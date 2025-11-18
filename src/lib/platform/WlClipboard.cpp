/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/WlClipboard.h"

#include "base/Log.h"

#include <chrono>
#include <common/Settings.h>
#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>

#include <QDateTime>
#include <QProcess>
#include <QStandardPaths>

namespace {

inline static const auto s_copyApp = QStringLiteral("wl-copy");
inline static const auto s_pasteApp = QStringLiteral("wl-paste");

// wl-clipboard args
inline static const auto s_listTypes = QStringLiteral("--list-types");
inline static const auto s_isPrimary = QStringLiteral("--primary");
inline static const auto s_noNewLine = QStringLiteral("-n");
inline static const auto s_readType = QStringLiteral("-t%1");

// MIME types for different clipboard formats
inline static const auto s_mimeTypeText = QStringLiteral("text/plain;charset=utf-8");
inline static const auto s_mimeTypeHtml = QStringLiteral("text/html");
inline static const auto s_mimeTypeBmp = QStringLiteral("image/bmp");

// Additional HTML MIME type variants
const char *const s_mimeTypeHtmlUtf8 = "text/html;charset=UTF-8";
const char *const s_mimeTypeHtmlWindows = "HTML Format";

// Command timeout (milliseconds)
const int kCommandTimeout = 5000;
const int kCacheValidityMs = 100;
const int kMonitorIntervalMs = 1000;
const int kMaxConsecutiveErrors = 5;

// Helper function to wait for process with timeout and proper cleanup
bool waitpidWithTimeout(pid_t pid, int *status, int timeout_ms)
{
  auto start = std::chrono::steady_clock::now();

  while (true) {
    int result = waitpid(pid, status, WNOHANG);
    if (result == pid) {
      return true; // Process finished
    } else if (result == -1) {
      if (errno == EINTR) {
        continue; // Interrupted, try again
      }
      return false; // Error
    }

    // Check timeout
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
    if (elapsed >= timeout_ms) {
      // Timeout - terminate the process
      kill(pid, SIGTERM);
      usleep(100000); // Give it 100ms to terminate gracefully

      result = waitpid(pid, status, WNOHANG);
      if (result != pid) {
        // Still not dead, force kill
        kill(pid, SIGKILL);
        waitpid(pid, status, 0); // This should not block
      }
      return false; // Timed out
    }

    usleep(10000); // Sleep 10ms before checking again
  }
}

// RAII class for managing processes with exception safety
class ProcessGuard
{
private:
  pid_t m_pid;
  bool m_released;

public:
  explicit ProcessGuard(pid_t pid) : m_pid(pid), m_released(false)
  {
  }

  ~ProcessGuard()
  {
    if (!m_released && m_pid > 0) {
      // Force cleanup if not properly released
      kill(m_pid, SIGTERM);
      usleep(100000);
      kill(m_pid, SIGKILL);
      int status;
      waitpid(m_pid, &status, 0);
    }
  }

  // Non-copyable, non-movable for simplicity
  ProcessGuard(const ProcessGuard &) = delete;
  ProcessGuard &operator=(const ProcessGuard &) = delete;
  ProcessGuard(ProcessGuard &&) = delete;
  ProcessGuard &operator=(ProcessGuard &&) = delete;

  pid_t get() const
  {
    return m_pid;
  }

  void release()
  {
    m_released = true;
  }
};

// RAII class for file descriptors
class FdGuard
{
private:
  int m_fd;

public:
  explicit FdGuard(int fd) : m_fd(fd)
  {
  }

  ~FdGuard()
  {
    if (m_fd >= 0) {
      ::close(m_fd);
    }
  }

  // Non-copyable, non-movable for simplicity
  FdGuard(const FdGuard &) = delete;
  FdGuard &operator=(const FdGuard &) = delete;
  FdGuard(FdGuard &&) = delete;
  FdGuard &operator=(FdGuard &&) = delete;

  int get() const
  {
    return m_fd;
  }

  void close()
  {
    if (m_fd >= 0) {
      ::close(m_fd);
      m_fd = -1;
    }
  }
};

} // namespace

WlClipboard::WlClipboard(ClipboardID id) : m_id(id), m_useClipboard(id == kClipboardClipboard)
{
  // Initialize cached data
  for (int i = 0; i < static_cast<int>(Format::TotalFormats); ++i) {
    m_cachedAvailable[i] = false;
  }
}

WlClipboard::~WlClipboard()
{
  stopMonitoring();
}

ClipboardID WlClipboard::getID() const
{
  return m_id;
}

bool WlClipboard::isAvailable()
{
  return !QStandardPaths::findExecutable(s_copyApp).isEmpty() && !QStandardPaths::findExecutable(s_pasteApp).isEmpty();
}

bool WlClipboard::isEnabled()
{
  return Settings::value(Settings::Core::useWlClipboard).toBool();
}

void WlClipboard::startMonitoring()
{
  if (m_monitoring) {
    return;
  }
  m_stopMonitoring = false;
  m_monitoring = true;
  m_monitorThread = std::make_unique<std::thread>(&WlClipboard::monitorClipboard, this);
}

void WlClipboard::stopMonitoring()
{
  if (!m_monitoring) {
    return;
  }

  m_stopMonitoring = true;
  m_monitoring = false;

  if (m_monitorThread && m_monitorThread->joinable()) {
    m_monitorThread->join();
  }
  m_monitorThread.reset();
}

bool WlClipboard::hasChanged() const
{
  return m_hasChanged.load();
}

bool WlClipboard::empty()
{
  if (!m_open) {
    return false;
  }

  std::vector<const char *> args;
  if (m_useClipboard) {
    args = {"wl-copy", nullptr};
  } else {
    args = {"wl-copy", "-p", nullptr};
  }

  bool success = executeCommandWithInput(args, "");

  if (success) {
    // Update ownership and cache only if command succeeded
    std::scoped_lock<std::mutex> lock(m_cacheMutex);
    updateOwnership(true);
    invalidateCache();
  }

  return success;
}

void WlClipboard::add(Format format, const std::string &data)
{
  if (!m_open) {
    return;
  }

  if (format == Format::HTML) {
    return;
  }

  auto mimeType = formatToMimeType(format).toStdString();
  if (mimeType.empty()) {
    LOG_WARN("unsupported clipboard format: %d", format);
    return;
  }

  std::vector<const char *> args;
  if (m_useClipboard) {
    args = {"wl-copy", "-t", mimeType.c_str(), nullptr};
  } else {
    args = {"wl-copy", "-t", mimeType.c_str(), "-p", nullptr};
  }

  bool success = executeCommandWithInput(args, data);
  if (success) {
    std::scoped_lock<std::mutex> lock(m_cacheMutex);
    updateOwnership(true);
    invalidateCache();
  } else {
    LOG_WARN("failed to set clipboard data");
  }
}

bool WlClipboard::open(Time time) const
{
  if (m_open) {
    LOG_DEBUG("failed to open clipboard: already opened");
    return false;
  }

  m_open = true;
  m_time = time;

  return true;
}

void WlClipboard::close() const
{
  if (!m_open) {
    return;
  }

  LOG_DEBUG("close clipboard");

  m_open = false;
  const_cast<WlClipboard *>(this)->invalidateCache();
}

IClipboard::Time WlClipboard::getTime() const
{
  return m_time;
}

bool WlClipboard::has(Format format) const
{
  if (!m_open) {
    return false;
  }

  std::scoped_lock<std::mutex> lock(m_cacheMutex);

  // Check cache validity
  Time currentTime = getCurrentTime();
  if (m_cached && (currentTime - m_cachedTime) < kCacheValidityMs) {
    return m_cachedAvailable[static_cast<int>(format)];
  }

  // Update cache by checking available MIME types
  const auto availableTypes = const_cast<WlClipboard *>(this)->getAvailableMimeTypes();

  if (availableTypes.isEmpty()) {
    // No types available - mark all formats as unavailable
    for (int i = 0; i < static_cast<int>(Format::TotalFormats); ++i) {
      m_cachedAvailable[i] = false;
      m_cachedData[i].clear();
    }
  } else {
    // Check each format against available types
    for (int i = 0; i < static_cast<int>(Format::TotalFormats); ++i) {
      Format currentFormat = static_cast<Format>(i);
      const auto mimeType = formatToMimeType(currentFormat);

      m_cachedAvailable[i] = false;
      if (!mimeType.isEmpty()) {
        for (const auto &available : availableTypes) {
          if (available == mimeType || (currentFormat == Format::Text && available == "text/plain") ||
              (currentFormat == Format::HTML && available.startsWith("text/html"))) {
            m_cachedAvailable[i] = true;
            break;
          }
        }
      }
    }
  }

  m_cached = true;
  m_cachedTime = currentTime;

  return m_cachedAvailable[static_cast<int>(format)];
}

std::string WlClipboard::get(Format format) const
{
  if (!m_open) {
    return std::string();
  }

  std::scoped_lock<std::mutex> lock(m_cacheMutex);

  // Return cached data if available and valid
  if (m_cached && m_cachedAvailable[static_cast<int>(format)] && !m_cachedData[static_cast<int>(format)].empty()) {
    return m_cachedData[static_cast<int>(format)];
  }

  auto mimeType = formatToMimeType(format);
  if (mimeType.isEmpty()) {
    return std::string();
  }

  QProcess cmd;
  cmd.setProgram(s_pasteApp);

  QStringList args = {s_noNewLine, s_readType.arg(mimeType)};
  if (!m_useClipboard)
    args.append(s_isPrimary);

  cmd.setArguments(args);
  cmd.start();
  cmd.waitForFinished();

  auto data = cmd.readAll().toStdString();

  // Update cache
  m_cachedData[static_cast<int>(format)] = data;
  m_cachedAvailable[static_cast<int>(format)] = !data.empty();
  m_cached = true;
  m_cachedTime = getCurrentTime();

  return data;
}

bool WlClipboard::executeCommandWithInput(const std::vector<const char *> &args, const std::string &input) const
{
  int pipefd[2];
  if (pipe(pipefd) == -1) {
    LOG_WARN("failed to create pipe");
    return false;
  }

  FdGuard readFd(pipefd[0]);
  FdGuard writeFd(pipefd[1]);

  // Set FD_CLOEXEC on pipe file descriptors
  fcntl(readFd.get(), F_SETFD, FD_CLOEXEC);
  fcntl(writeFd.get(), F_SETFD, FD_CLOEXEC);

  // Set up file actions for posix_spawn
  posix_spawn_file_actions_t fileActions;
  posix_spawn_file_actions_init(&fileActions);

  // Redirect stdin from pipe read end
  posix_spawn_file_actions_adddup2(&fileActions, readFd.get(), STDIN_FILENO);

  // Redirect stdout and stderr to /dev/null
  posix_spawn_file_actions_addopen(&fileActions, STDOUT_FILENO, "/dev/null", O_WRONLY, 0);
  posix_spawn_file_actions_addopen(&fileActions, STDERR_FILENO, "/dev/null", O_WRONLY, 0);

  // Close pipe file descriptors in child
  posix_spawn_file_actions_addclose(&fileActions, readFd.get());
  posix_spawn_file_actions_addclose(&fileActions, writeFd.get());

  extern char **environ;
  pid_t pid;
  int spawnResult = posix_spawnp(&pid, args[0], &fileActions, nullptr, const_cast<char *const *>(args.data()), environ);

  posix_spawn_file_actions_destroy(&fileActions);

  if (spawnResult != 0) {
    LOG_WARN("failed to spawn process: %s", strerror(spawnResult));
    return false;
  }

  ProcessGuard processGuard(pid);

  // Parent process - close read end
  readFd.close();

  bool writeSuccess = true;
  if (!input.empty()) {
    // Write with timeout using poll
    struct pollfd pfd;
    pfd.fd = writeFd.get();
    pfd.events = POLLOUT;

    const char *data = input.c_str();
    size_t totalWritten = 0;
    size_t dataSize = input.length();
    auto start = std::chrono::steady_clock::now();

    while (totalWritten < dataSize) {
      auto now = std::chrono::steady_clock::now();
      auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
      int remainingTimeout = kCommandTimeout - elapsed;

      if (remainingTimeout <= 0) {
        writeSuccess = false;
        break;
      }

      int pollResult = poll(&pfd, 1, remainingTimeout);
      if (pollResult == 0) {
        writeSuccess = false;
        break; // Timeout
      } else if (pollResult < 0) {
        if (errno == EINTR) {
          continue;
        }
        writeSuccess = false;
        break;
      }

      if (pfd.revents & POLLOUT) {
        ssize_t written = write(writeFd.get(), data + totalWritten, dataSize - totalWritten);
        if (written > 0) {
          totalWritten += written;
        } else if (written < 0 && errno != EINTR && errno != EAGAIN) {
          writeSuccess = false;
          break;
        }
      }

      if (pfd.revents & (POLLHUP | POLLERR)) {
        writeSuccess = false;
        break;
      }
    }

    if (!writeSuccess) {
      LOG_WARN("failed to write all input data");
      return false;
    }
  }

  writeFd.close();

  int status;
  bool processFinished = waitpidWithTimeout(pid, &status, kCommandTimeout);
  if (processFinished) {
    processGuard.release();
    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
  } else {
    LOG_WARN("process did not terminate properly");
    return false;
  }
}

QString WlClipboard::formatToMimeType(Format format) const
{
  switch (format) {
  case Format::Text:
    return s_mimeTypeText;
  case Format::HTML:
    return s_mimeTypeHtml;
  case Format::Bitmap:
    return s_mimeTypeBmp;
  default:
    return {};
  }
}

IClipboard::Format WlClipboard::mimeTypeToFormat(const QString &mimeType) const
{
  if (mimeType == s_mimeTypeText || mimeType == QStringLiteral("text/plain")) {
    return Format::Text;
  }
  if (mimeType == s_mimeTypeHtml || mimeType == s_mimeTypeHtmlUtf8 || mimeType == s_mimeTypeHtmlWindows ||
      mimeType.contains("text/html")) {
    return Format::HTML;
  }
  if (mimeType == s_mimeTypeBmp) {
    return Format::Bitmap;
  }
  return Format::Text; // Default fallback
}

QStringList WlClipboard::getAvailableMimeTypes() const
{
  QProcess cmd;
  cmd.setProgram(s_pasteApp);

  QStringList args = {s_listTypes};
  if (!m_useClipboard)
    args.append(s_isPrimary);

  cmd.setArguments(args);
  cmd.start();
  cmd.waitForFinished();

  const static QChar newLine = QLatin1Char('\n');
  return QString::fromLocal8Bit(cmd.readAll()).split(newLine);
}

void WlClipboard::monitorClipboard()
{
  QStringList lastTypes;
  int consecutiveErrors = 0;

  while (!m_stopMonitoring) {
    std::this_thread::sleep_for(std::chrono::milliseconds(kMonitorIntervalMs));
    try {
      // Check if clipboard content has changed by comparing available types
      const auto currentTypes = getAvailableMimeTypes();

      // Reset error counter on successful operation
      consecutiveErrors = 0;

      if (currentTypes != lastTypes) {
        m_hasChanged = true;
        lastTypes = currentTypes;

        // Clear cache when clipboard changes
        std::scoped_lock<std::mutex> lock(m_cacheMutex);
        invalidateCache();
        const_cast<WlClipboard *>(this)->updateOwnership(false);
      }
    } catch (const std::exception &e) {
      LOG_WARN("clipboard monitoring error: %s", e.what());
      if (++consecutiveErrors >= kMaxConsecutiveErrors) {
        LOG_ERR("too many consecutive errors in clipboard monitoring, stopping");
        break;
      }
    } catch (...) {
      LOG_WARN("clipboard monitoring unknown error");
      if (++consecutiveErrors >= kMaxConsecutiveErrors) {
        LOG_ERR("too many consecutive errors in clipboard monitoring, stopping");
        break;
      }
    }
  }
}

IClipboard::Time WlClipboard::getCurrentTime() const
{
  return static_cast<Time>(QDateTime::currentMSecsSinceEpoch());
}

bool WlClipboard::isOwned() const
{
  return m_owned;
}

void WlClipboard::resetChanged()
{
  m_hasChanged = false;

  // Clear cache when resetting change flag to force fresh data retrieval
  std::scoped_lock<std::mutex> lock(m_cacheMutex);
  invalidateCache();
}

void WlClipboard::updateOwnership(bool owned)
{
  m_owned = owned;
}

void WlClipboard::invalidateCache()
{
  m_cached = false;
  m_cachedTime = 0;
  for (int i = 0; i < static_cast<int>(Format::TotalFormats); ++i) {
    m_cachedData[i].clear();
    m_cachedAvailable[i] = false;
  }
}
