/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/WlClipboard.h"

#include "base/Log.h"

#include <chrono>
#include <common/Settings.h>
#include <fcntl.h>
#include <poll.h>
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
const int kCacheValidityMs = 100;
const int kMonitorIntervalMs = 1000;
const int kMaxConsecutiveErrors = 5;
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
  for (auto &cmd : m_runningWlCopies) {
    cmd->kill();
    cmd->waitForFinished(100);
  }
  m_runningWlCopies.clear();
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
  auto cmd = new QProcess(this);
  cmd->setProgram(s_copyApp);
  m_runningWlCopies.append(cmd);
  connect(cmd, &QProcess::finished, this, [&] { m_runningWlCopies.removeAll(cmd); });

  QStringList args = {s_noNewLine, ""};
  if (!m_useClipboard)
    args.prepend(s_isPrimary);

  cmd->setArguments(args);
  cmd->start();
  bool success = cmd->waitForStarted(100);

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

  auto mimeType = formatToMimeType(format);
  if (mimeType.isEmpty()) {
    LOG_WARN("unsupported clipboard format: %d", format);
    return;
  }

  auto cmd = new QProcess(this);
  cmd->setProgram(s_copyApp);

  m_runningWlCopies.append(cmd);
  connect(cmd, &QProcess::finished, this, [&] { m_runningWlCopies.removeAll(cmd); });

  QStringList args = {s_noNewLine, s_readType.arg(mimeType), QString::fromStdString(data)};
  if (!m_useClipboard)
    args.prepend(s_isPrimary);

  cmd->setArguments(args);
  cmd->start();

  if (cmd->waitForStarted(100)) {
    std::scoped_lock<std::mutex> lock(m_cacheMutex);
    updateOwnership(true);
    invalidateCache();
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

  if (const auto availableTypes = getAvailableMimeTypes(); availableTypes.isEmpty()) {
    // No types available - mark all formats as unavailable
    for (int i = 0; i < static_cast<int>(Format::TotalFormats); ++i) {
      m_cachedAvailable[i] = false;
      m_cachedData[i].clear();
    }
  } else {
    using enum IClipboard::Format;
    // Check each format against available types
    for (int i = 0; i < static_cast<int>(TotalFormats); ++i) {
      auto currentFormat = static_cast<Format>(i);
      const auto mimeType = formatToMimeType(currentFormat);

      m_cachedAvailable[i] = false;
      if (!mimeType.isEmpty()) {
        for (const auto &available : availableTypes) {
          if (available == mimeType || (currentFormat == Text && available == QStringLiteral("text/plain")) ||
              (currentFormat == HTML && available.startsWith(QStringLiteral("text/html")))) {
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

QString WlClipboard::formatToMimeType(Format format) const
{
  switch (format) {
    using enum IClipboard::Format;
  case Text:
    return s_mimeTypeText;
  case HTML:
    return s_mimeTypeHtml;
  case Bitmap:
    return s_mimeTypeBmp;
  default:
    return {};
  }
}

IClipboard::Format WlClipboard::mimeTypeToFormat(const QString &mimeType) const
{
  using enum IClipboard::Format;
  if (mimeType == s_mimeTypeText || mimeType == QStringLiteral("text/plain")) {
    return Text;
  }
  if (mimeType == s_mimeTypeHtml || mimeType == s_mimeTypeHtmlUtf8 || mimeType == s_mimeTypeHtmlWindows ||
      mimeType.contains("text/html")) {
    return HTML;
  }
  if (mimeType == s_mimeTypeBmp) {
    return Bitmap;
  }
  return Text; // Default fallback
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
        updateOwnership(false);
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
