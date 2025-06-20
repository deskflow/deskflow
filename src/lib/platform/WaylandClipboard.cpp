/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/WaylandClipboard.h"

#include "base/Log.h"

#include <chrono>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <poll.h>
#include <errno.h>

namespace {

// MIME types for different clipboard formats
const char* const kMimeTypeText = "text/plain;charset=utf-8";
const char* const kMimeTypeHtml = "text/html";
const char* const kMimeTypeBmp = "image/bmp";

// Additional HTML MIME type variants
const char* const kMimeTypeHtmlUtf8 = "text/html;charset=UTF-8";
const char* const kMimeTypeHtmlWindows = "HTML Format";

// Command timeout (milliseconds)
const int kCommandTimeout = 5000;
const int kCacheValidityMs = 100;
const int kMonitorIntervalMs = 1000;
const int kMaxConsecutiveErrors = 5;

// Helper function to wait for process with timeout and proper cleanup
bool waitpidWithTimeout(pid_t pid, int* status, int timeout_ms) {
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
class ProcessGuard {
private:
    pid_t m_pid;
    bool m_released;

public:
    explicit ProcessGuard(pid_t pid) : m_pid(pid), m_released(false) {}
    
    ~ProcessGuard() {
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
    ProcessGuard(const ProcessGuard&) = delete;
    ProcessGuard& operator=(const ProcessGuard&) = delete;
    ProcessGuard(ProcessGuard&&) = delete;
    ProcessGuard& operator=(ProcessGuard&&) = delete;
    
    pid_t get() const { return m_pid; }
    
    void release() { m_released = true; }
};

// RAII class for file descriptors
class FdGuard {
private:
    int m_fd;

public:
    explicit FdGuard(int fd) : m_fd(fd) {}
    
    ~FdGuard() {
        if (m_fd >= 0) {
            ::close(m_fd);
        }
    }
    
    // Non-copyable, non-movable for simplicity
    FdGuard(const FdGuard&) = delete;
    FdGuard& operator=(const FdGuard&) = delete;
    FdGuard(FdGuard&&) = delete;
    FdGuard& operator=(FdGuard&&) = delete;
    
    int get() const { return m_fd; }
    
    void close() {
        if (m_fd >= 0) {
            ::close(m_fd);
            m_fd = -1;
        }
    }
};

} // namespace

WaylandClipboard::WaylandClipboard(ClipboardID id)
    : m_id(id),
      m_useClipboard(id == kClipboardClipboard)
{
    // Initialize cached data
    for (int i = 0; i < kNumFormats; ++i) {
        m_cachedAvailable[i] = false;
    }
}

WaylandClipboard::~WaylandClipboard()
{
    stopMonitoring();
}

ClipboardID WaylandClipboard::getID() const
{
    return m_id;
}

bool WaylandClipboard::isAvailable()
{
    return checkCommandExists("wl-paste") && checkCommandExists("wl-copy");
}

bool WaylandClipboard::checkCommandExists(const char* command)
{
    std::vector<const char*> args = {command, "--help", nullptr};

    // Set up file actions for posix_spawn
    posix_spawn_file_actions_t file_actions;
    posix_spawn_file_actions_init(&file_actions);
    
    // Redirect stdout and stderr to /dev/null
    posix_spawn_file_actions_addopen(&file_actions, STDOUT_FILENO, "/dev/null", O_WRONLY, 0);
    posix_spawn_file_actions_addopen(&file_actions, STDERR_FILENO, "/dev/null", O_WRONLY, 0);

    extern char **environ;
    pid_t pid;
    int spawn_result = posix_spawnp(&pid, command, &file_actions, nullptr, 
                                    const_cast<char* const*>(args.data()), environ);
    
    posix_spawn_file_actions_destroy(&file_actions);
    
    if (spawn_result != 0) {
        return false;
    }

    ProcessGuard process_guard(pid);

    int status;
    bool success = waitpidWithTimeout(pid, &status, kCommandTimeout);
    if (success) {
        process_guard.release();
        return WIFEXITED(status) && WEXITSTATUS(status) == 0;
    } else {
        return false;
    }
}

void WaylandClipboard::startMonitoring()
{
    if (m_monitoring) {
        return;
    }
    m_stopMonitoring = false;
    m_monitoring = true;
    m_monitorThread = std::make_unique<std::thread>(&WaylandClipboard::monitorClipboard, this);
}

void WaylandClipboard::stopMonitoring()
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

bool WaylandClipboard::hasChanged() const
{
    return m_hasChanged.load();
}

bool WaylandClipboard::empty()
{
    if (!m_open) {
        return false;
    }

    std::vector<const char*> args;
    if (m_useClipboard) {
        args = {"wl-copy", nullptr};
    } else {
        args = {"wl-copy", "-p", nullptr};
    }
    
    bool success = executeCommandWithInput(args, "");
    
    if (success) {
        // Update ownership and cache only if command succeeded
        std::lock_guard<std::mutex> lock(m_cacheMutex);
        updateOwnership(true);
        invalidateCache();
    }
    
    return success;
}

void WaylandClipboard::add(EFormat format, const std::string &data)
{
    if (!m_open) {
        return;
    }

    if (format == kHTML) {
        return;
    }

    std::string mimeType = formatToMimeType(format);
    if (mimeType.empty()) {
        LOG_WARN("unsupported clipboard format: %d", format);
        return;
    }

    std::vector<const char*> args;
    if (m_useClipboard) {
        args = {"wl-copy", "-t", mimeType.c_str(), nullptr};
    } else {
        args = {"wl-copy", "-t", mimeType.c_str(), "-p", nullptr};
    }

    bool success = executeCommandWithInput(args, data);
    if (success) {
        std::lock_guard<std::mutex> lock(m_cacheMutex);
        updateOwnership(true);
        invalidateCache();
    } else {
        LOG_WARN("failed to set clipboard data");
    }
}

bool WaylandClipboard::open(Time time) const
{
    if (m_open) {
        LOG_DEBUG("failed to open clipboard: already opened");
        return false;
    }

    m_open = true;
    m_time = time;
    
    return true;
}

void WaylandClipboard::close() const
{
    if (!m_open) {
        return;
    }

    LOG_DEBUG("close clipboard");
    
    m_open = false;
    const_cast<WaylandClipboard*>(this)->invalidateCache();
}

IClipboard::Time WaylandClipboard::getTime() const
{
    return m_time;
}

bool WaylandClipboard::has(EFormat format) const
{
    if (!m_open) {
        return false;
    }

    std::lock_guard<std::mutex> lock(m_cacheMutex);
    
    // Check cache validity
    Time currentTime = getCurrentTime();
    if (m_cached && (currentTime - m_cachedTime) < kCacheValidityMs) {
        return m_cachedAvailable[format];
    }
    
    // Update cache by checking available MIME types
    std::vector<std::string> availableTypes = const_cast<WaylandClipboard*>(this)->getAvailableMimeTypes();
    
    if (availableTypes.empty()) {
        // No types available - mark all formats as unavailable
        for (int i = 0; i < kNumFormats; ++i) {
            m_cachedAvailable[i] = false;
            m_cachedData[i].clear();
        }
    } else {
        // Check each format against available types
        for (int i = 0; i < kNumFormats; ++i) {
            EFormat currentFormat = static_cast<EFormat>(i);
            std::string mimeType = formatToMimeType(currentFormat);
            
            m_cachedAvailable[i] = false;
            if (!mimeType.empty()) {
                for (const std::string& available : availableTypes) {
                    if (available == mimeType || 
                        (currentFormat == kText && available == "text/plain") ||
                        (currentFormat == kHTML && available.find("text/html") == 0)) {
                        m_cachedAvailable[i] = true;
                        break;
                    }
                }
            }
        }
    }
    
    m_cached = true;
    m_cachedTime = currentTime;

    return m_cachedAvailable[format];
}

std::string WaylandClipboard::get(EFormat format) const
{
    if (!m_open) {
        return std::string();
    }

    std::lock_guard<std::mutex> lock(m_cacheMutex);
    
    // Return cached data if available and valid
    if (m_cached && m_cachedAvailable[format] && !m_cachedData[format].empty()) {
        return m_cachedData[format];
    }
    
    std::string mimeType = formatToMimeType(format);
    if (mimeType.empty()) {
        return std::string();
    }

    std::vector<const char*> args;
    if (m_useClipboard) {
        args = {"wl-paste", "-t", mimeType.c_str(), nullptr};
    } else {
        args = {"wl-paste", "-t", mimeType.c_str(), "-p", nullptr};
    }

    std::string data = const_cast<WaylandClipboard*>(this)->executeCommand(args);
    
    // Update cache
    m_cachedData[format] = data;
    m_cachedAvailable[format] = !data.empty();
    m_cached = true;
    m_cachedTime = getCurrentTime();
    
    return data;
}

std::string WaylandClipboard::executeCommand(const std::vector<const char*> &args) const
{
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        LOG_WARN("failed to create pipe");
        return std::string();
    }

    FdGuard read_fd(pipefd[0]);
    FdGuard write_fd(pipefd[1]);

    // Set FD_CLOEXEC on pipe file descriptors
    fcntl(read_fd.get(), F_SETFD, FD_CLOEXEC);
    fcntl(write_fd.get(), F_SETFD, FD_CLOEXEC);

    // Set up file actions for posix_spawn
    posix_spawn_file_actions_t file_actions;
    posix_spawn_file_actions_init(&file_actions);
    
    // Redirect stdout to pipe write end
    posix_spawn_file_actions_adddup2(&file_actions, write_fd.get(), STDOUT_FILENO);
    
    // Redirect stderr to /dev/null
    posix_spawn_file_actions_addopen(&file_actions, STDERR_FILENO, "/dev/null", O_WRONLY, 0);
    
    // Close pipe file descriptors in child
    posix_spawn_file_actions_addclose(&file_actions, read_fd.get());
    posix_spawn_file_actions_addclose(&file_actions, write_fd.get());

    extern char **environ;
    pid_t pid;
    int spawn_result = posix_spawnp(&pid, args[0], &file_actions, nullptr, 
                                    const_cast<char* const*>(args.data()), environ);
    
    posix_spawn_file_actions_destroy(&file_actions);
    
    if (spawn_result != 0) {
        LOG_WARN("failed to spawn process: %s", strerror(spawn_result));
        return std::string();
    }

    ProcessGuard process_guard(pid);

    // Parent process - close write end
    write_fd.close();

    std::string result;
    char buffer[4096];
    
    // Use poll to read with timeout
    struct pollfd pfd;
    pfd.fd = read_fd.get();
    pfd.events = POLLIN;
    
    auto start = std::chrono::steady_clock::now();
    while (true) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
        int remaining_timeout = kCommandTimeout - elapsed;
        
        if (remaining_timeout <= 0) {
            break; // Timeout
        }
        
        int poll_result = poll(&pfd, 1, remaining_timeout);
        if (poll_result == 0) {
            break; // Timeout
        } else if (poll_result < 0) {
            if (errno == EINTR) {
                continue; // Interrupted, try again
            }
            break; // Error
        }
        
        if (pfd.revents & POLLIN) {
            ssize_t bytesRead = read(read_fd.get(), buffer, sizeof(buffer));
            if (bytesRead > 0) {
                result.append(buffer, bytesRead);
            } else if (bytesRead == 0) {
                break; // EOF
            } else if (errno != EINTR) {
                break; // Error
            }
        }
        
        if (pfd.revents & (POLLHUP | POLLERR)) {
            break; // Pipe closed or error
        }
    }

    read_fd.close();

    int status;
    bool process_finished = waitpidWithTimeout(pid, &status, 1000); // Give 1 more second
    if (process_finished) {
        process_guard.release();
        
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            // Remove trailing newline if present
            if (!result.empty() && result.back() == '\n') {
                result.pop_back();
            }
            return result;
        } else {
            LOG_DEBUG1("command failed with status %d", WEXITSTATUS(status));
            return std::string();
        }
    } else {
        LOG_WARN("process did not terminate properly");
        return std::string();
    }
}

bool WaylandClipboard::executeCommandWithInput(const std::vector<const char*> &args, const std::string &input) const
{
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        LOG_WARN("failed to create pipe");
        return false;
    }

    FdGuard read_fd(pipefd[0]);
    FdGuard write_fd(pipefd[1]);

    // Set FD_CLOEXEC on pipe file descriptors
    fcntl(read_fd.get(), F_SETFD, FD_CLOEXEC);
    fcntl(write_fd.get(), F_SETFD, FD_CLOEXEC);

    // Set up file actions for posix_spawn
    posix_spawn_file_actions_t file_actions;
    posix_spawn_file_actions_init(&file_actions);
    
    // Redirect stdin from pipe read end
    posix_spawn_file_actions_adddup2(&file_actions, read_fd.get(), STDIN_FILENO);
    
    // Redirect stdout and stderr to /dev/null
    posix_spawn_file_actions_addopen(&file_actions, STDOUT_FILENO, "/dev/null", O_WRONLY, 0);
    posix_spawn_file_actions_addopen(&file_actions, STDERR_FILENO, "/dev/null", O_WRONLY, 0);
    
    // Close pipe file descriptors in child
    posix_spawn_file_actions_addclose(&file_actions, read_fd.get());
    posix_spawn_file_actions_addclose(&file_actions, write_fd.get());

    extern char **environ;
    pid_t pid;
    int spawn_result = posix_spawnp(&pid, args[0], &file_actions, nullptr, 
                                    const_cast<char* const*>(args.data()), environ);
    
    posix_spawn_file_actions_destroy(&file_actions);
    
    if (spawn_result != 0) {
        LOG_WARN("failed to spawn process: %s", strerror(spawn_result));
        return false;
    }

    ProcessGuard process_guard(pid);

    // Parent process - close read end
    read_fd.close();

    bool write_success = true;
    if (!input.empty()) {
        // Write with timeout using poll
        struct pollfd pfd;
        pfd.fd = write_fd.get();
        pfd.events = POLLOUT;
        
        const char* data = input.c_str();
        size_t total_written = 0;
        size_t data_size = input.length();
        auto start = std::chrono::steady_clock::now();
        
        while (total_written < data_size) {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
            int remaining_timeout = kCommandTimeout - elapsed;
            
            if (remaining_timeout <= 0) {
                write_success = false;
                break;
            }
            
            int poll_result = poll(&pfd, 1, remaining_timeout);
            if (poll_result == 0) {
                write_success = false;
                break; // Timeout
            } else if (poll_result < 0) {
                if (errno == EINTR) {
                    continue;
                }
                write_success = false;
                break;
            }
            
            if (pfd.revents & POLLOUT) {
                ssize_t written = write(write_fd.get(), data + total_written, data_size - total_written);
                if (written > 0) {
                    total_written += written;
                } else if (written < 0 && errno != EINTR && errno != EAGAIN) {
                    write_success = false;
                    break;
                }
            }
            
            if (pfd.revents & (POLLHUP | POLLERR)) {
                write_success = false;
                break;
            }
        }
        
        if (!write_success) {
            LOG_WARN("failed to write all input data");
            return false;
        }
    }

    write_fd.close();

    int status;
    bool process_finished = waitpidWithTimeout(pid, &status, kCommandTimeout);
    if (process_finished) {
        process_guard.release();
        return WIFEXITED(status) && WEXITSTATUS(status) == 0;
    } else {
        LOG_WARN("process did not terminate properly");
        return false;
    }
}

std::string WaylandClipboard::formatToMimeType(EFormat format) const
{
    switch (format) {
        case kText:
            return kMimeTypeText;
        case kHTML:
            return kMimeTypeHtml;
        case kBitmap:
            return kMimeTypeBmp;
        default:
            return std::string();
    }
}

IClipboard::EFormat WaylandClipboard::mimeTypeToFormat(const std::string &mimeType) const
{
    if (mimeType == kMimeTypeText || mimeType == "text/plain") {
        return kText;
    }
    if (mimeType == kMimeTypeHtml || mimeType == kMimeTypeHtmlUtf8 ||
        mimeType == kMimeTypeHtmlWindows || mimeType.find("text/html") == 0) {
        return kHTML;
    }
    if (mimeType == kMimeTypeBmp || mimeType == "image/bmp") {
        return kBitmap;
    }
    return kText; // Default fallback
}

std::vector<std::string> WaylandClipboard::getAvailableMimeTypes() const
{
    std::vector<const char*> args;
    if (m_useClipboard) {
        args = {"wl-paste", "--list-types", nullptr};
    } else {
        args = {"wl-paste", "--list-types", "-p", nullptr};
    }
    
    std::string result = executeCommand(args);
    std::vector<std::string> types;
    
    if (!result.empty()) {
        std::istringstream iss(result);
        std::string type;
        while (std::getline(iss, type)) {
            if (!type.empty()) {
                types.push_back(type);
            }
        }
    }
    
    return types;
}

std::string WaylandClipboard::getClipboardData(const std::string &mimeType) const
{
    std::vector<const char*> args;
    if (m_useClipboard) {
        args = {"wl-paste", "-t", mimeType.c_str(), nullptr};
    } else {
        args = {"wl-paste", "-t", mimeType.c_str(), "-p", nullptr};
    }
    return executeCommand(args);
}

bool WaylandClipboard::setClipboardData(const std::string &mimeType, const std::string &data) const
{
    std::vector<const char*> args;
    if (m_useClipboard) {
        args = {"wl-copy", "-t", mimeType.c_str(), nullptr};
    } else {
        args = {"wl-copy", "-t", mimeType.c_str(), "-p", nullptr};
    }
    return executeCommandWithInput(args, data);
}

void WaylandClipboard::monitorClipboard()
{
    std::vector<std::string> lastTypes;
    int consecutiveErrors = 0;
    
    while (!m_stopMonitoring) {
        std::this_thread::sleep_for(std::chrono::milliseconds(kMonitorIntervalMs));
        try {
            // Check if clipboard content has changed by comparing available types
            std::vector<std::string> currentTypes = getAvailableMimeTypes();

            // Reset error counter on successful operation
            consecutiveErrors = 0;

            if (currentTypes != lastTypes) {
                m_hasChanged = true;
                lastTypes = currentTypes;

                // Clear cache when clipboard changes
                std::lock_guard<std::mutex> lock(m_cacheMutex);
                invalidateCache();
                const_cast<WaylandClipboard*>(this)->updateOwnership(false);
            }
        } catch (const std::exception& e) {
            LOG_WARN("Exception in clipboard monitoring: %s", e.what());
            if (++consecutiveErrors >= kMaxConsecutiveErrors) {
                LOG_ERR("Too many consecutive errors in clipboard monitoring, stopping");
                break;
            }
        } catch (...) {
            LOG_WARN("Unknown exception in clipboard monitoring");
            if (++consecutiveErrors >= kMaxConsecutiveErrors) {
                LOG_ERR("Too many consecutive errors in clipboard monitoring, stopping");
                break;
            }
        }
    }
}

IClipboard::Time WaylandClipboard::getCurrentTime() const
{
    auto now = std::chrono::steady_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
    return static_cast<Time>(ms.count());
}

bool WaylandClipboard::isOwned() const
{
    return m_owned;
}

void WaylandClipboard::resetChanged()
{
    m_hasChanged = false;

    // Clear cache when resetting change flag to force fresh data retrieval
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    invalidateCache();
}

void WaylandClipboard::updateOwnership(bool owned)
{
    m_owned = owned;
}

void WaylandClipboard::invalidateCache()
{
    m_cached = false;
    m_cachedTime = 0;
    for (int i = 0; i < kNumFormats; ++i) {
        m_cachedData[i].clear();
        m_cachedAvailable[i] = false;
    }
}
