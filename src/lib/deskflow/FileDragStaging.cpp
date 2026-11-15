/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "deskflow/FileDragStaging.h"

#include "deskflow/FileDragCodec.h"

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <map>
#include <set>
#include <system_error>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#else
#include <signal.h>
#include <unistd.h>
#endif

#if defined(__APPLE__)
#include <stdio.h> // renamex_np
#endif

namespace fs = std::filesystem;

namespace deskflow::filedrag {

namespace {

uint32_t currentProcessId()
{
#if defined(_WIN32)
  return GetCurrentProcessId();
#else
  return static_cast<uint32_t>(getpid());
#endif
}

bool isProcessAlive(uint32_t pid)
{
  if (pid == 0) {
    return false;
  }
  if (pid == currentProcessId()) {
    return true;
  }
#if defined(_WIN32)
  HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
  if (process == nullptr) {
    // access denied means it exists but belongs to someone else
    return GetLastError() == ERROR_ACCESS_DENIED;
  }
  DWORD code = 0;
  const bool alive = GetExitCodeProcess(process, &code) && code == STILL_ACTIVE;
  CloseHandle(process);
  return alive;
#else
  return kill(static_cast<pid_t>(pid), 0) == 0 || errno == EPERM;
#endif
}

fs::path fromUtf8(const std::string &s)
{
  return fs::path(std::u8string(s.begin(), s.end()));
}

std::string toUtf8(const fs::path &p)
{
  const auto u8 = p.u8string();
  return std::string(u8.begin(), u8.end());
}

void setError(std::string *error, const std::string &message)
{
  if (error != nullptr) {
    *error = message;
  }
}

enum class MoveResult
{
  Moved,
  Exists,
  CrossDevice,
  Failed
};

MoveResult renameNoReplace(const fs::path &source, const fs::path &target)
{
#if defined(_WIN32)
  // without MOVEFILE_REPLACE_EXISTING this fails if the target exists
  if (MoveFileExW(source.c_str(), target.c_str(), 0)) {
    return MoveResult::Moved;
  }
  switch (GetLastError()) {
  case ERROR_ALREADY_EXISTS:
  case ERROR_FILE_EXISTS:
    return MoveResult::Exists;
  case ERROR_NOT_SAME_DEVICE:
    return MoveResult::CrossDevice;
  default:
    return MoveResult::Failed;
  }
#elif defined(__APPLE__)
  // RENAME_EXCL fails with EEXIST instead of replacing, in one step
  if (renamex_np(source.c_str(), target.c_str(), RENAME_EXCL) == 0) {
    return MoveResult::Moved;
  }
  switch (errno) {
  case EEXIST:
  case ENOTEMPTY:
    return MoveResult::Exists;
  case EXDEV:
    return MoveResult::CrossDevice;
  default:
    return MoveResult::Failed;
  }
#else
  // POSIX rename() replaces silently, so check first. The window between the check and
  // the rename is tiny and only matters if another program creates the same name then.
  std::error_code ec;
  if (fs::exists(target, ec) || fs::is_symlink(target, ec)) {
    return MoveResult::Exists;
  }
  fs::rename(source, target, ec);
  if (!ec) {
    return MoveResult::Moved;
  }
  if (ec == std::errc::cross_device_link) {
    return MoveResult::CrossDevice;
  }
  if (ec == std::errc::file_exists || ec == std::errc::directory_not_empty) {
    return MoveResult::Exists;
  }
  return MoveResult::Failed;
#endif
}

MoveResult copyNoReplace(const fs::path &source, const fs::path &target, bool isDirectory)
{
  std::error_code ec;
  if (isDirectory) {
    // create_directory() returns false if it exists, which reserves the name for us
    if (!fs::create_directory(target, ec)) {
      return ec ? MoveResult::Failed : MoveResult::Exists;
    }
    fs::copy(source, target, fs::copy_options::recursive, ec);
    if (ec) {
      fs::remove_all(target, ec);
      return MoveResult::Failed;
    }
    fs::remove_all(source, ec);
    return MoveResult::Moved;
  }

  // copy_options::none fails if the target exists
  if (!fs::copy_file(source, target, fs::copy_options::none, ec)) {
    if (ec == std::errc::file_exists) {
      return MoveResult::Exists;
    }
    return MoveResult::Failed;
  }
  fs::remove(source, ec);
  return MoveResult::Moved;
}

std::atomic<uint32_t> g_stagingCounter{0};

} // namespace

fs::path FileDragStaging::defaultRoot()
{
  std::error_code ec;
  auto temp = fs::temp_directory_path(ec);
  if (ec) {
    temp = fs::current_path(ec);
  }
  return temp / "Deskflow";
}

void FileDragStaging::sweepStale(const fs::path &root)
{
  std::error_code ec;
  if (!fs::is_directory(root, ec)) {
    return;
  }
  for (const auto &entry : fs::directory_iterator(root, ec)) {
    const auto name = toUtf8(entry.path().filename());
    if (!name.starts_with("drag-") || !entry.is_directory(ec)) {
      continue;
    }
    // drag-<pid>-<id>-<n>: keep folders of processes that are still running
    uint32_t pid = 0;
    try {
      pid = static_cast<uint32_t>(std::stoul(name.substr(5)));
    } catch (...) {
      pid = 0;
    }
    if (isProcessAlive(pid)) {
      continue;
    }
    std::error_code removeError;
    fs::remove_all(entry.path(), removeError);
  }
}

uint64_t FileDragStaging::freeSpace(const fs::path &path)
{
  std::error_code ec;
  const auto info = fs::space(path, ec);
  if (ec) {
    return UINT64_MAX;
  }
  return info.available;
}

FileDragStaging::FileDragStaging(fs::path root) : m_root(std::move(root))
{
}

FileDragStaging::~FileDragStaging()
{
  std::scoped_lock lock(m_mutex);
  if (m_out.is_open()) {
    m_out.close();
  }
  m_cancelled = true;
  m_cond.notify_all();
  removeAllLocked();
}

bool FileDragStaging::begin(uint32_t dragId, const ItemList &items, CancelReason &reason, std::string *error)
{
  std::scoped_lock lock(m_mutex);
  if (m_begun) {
    reason = CancelReason::Busy;
    setError(error, "staging already in use");
    return false;
  }

  if (!validateItems(items, error)) {
    reason = CancelReason::Invalid;
    return false;
  }

  uint64_t total = 0;
  totalSize(items, total);

  std::error_code ec;
  fs::create_directories(m_root, ec);
  if (ec) {
    reason = CancelReason::WriteFailed;
    setError(error, "cannot create staging root: " + ec.message());
    return false;
  }

  const uint64_t available = m_freeSpaceOverride != UINT64_MAX ? m_freeSpaceOverride : freeSpace(m_root);
  if (available != UINT64_MAX && (available < kStagingSpareBytes || available - kStagingSpareBytes < total)) {
    reason = CancelReason::NoSpace;
    setError(error, "not enough free space for " + std::to_string(total) + " bytes");
    return false;
  }

  for (int attempt = 0; attempt < 100; ++attempt) {
    const auto name = "drag-" + std::to_string(currentProcessId()) + "-" + std::to_string(dragId) + "-" +
                      std::to_string(++g_stagingCounter);
    auto dir = m_root / name;
    if (fs::create_directory(dir, ec)) {
      m_dir = dir;
      break;
    }
    if (ec) {
      reason = CancelReason::WriteFailed;
      setError(error, "cannot create staging folder: " + ec.message());
      return false;
    }
  }
  if (m_dir.empty()) {
    reason = CancelReason::WriteFailed;
    setError(error, "no free staging folder name");
    return false;
  }

  m_begun = true;
  m_dragId = dragId;
  m_totalBytes = total;
  m_entries.clear();
  m_files.assign(items.size(), FileState{});

  // remote folder path -> local folder path, and the names used in each local folder
  std::map<std::string, std::string> localDirs;
  std::map<std::string, std::set<std::string>> usedNames;

  for (const auto &item : items) {
    const auto slash = item.path.rfind('/');
    const std::string remoteParent = slash == std::string::npos ? std::string() : item.path.substr(0, slash);
    const std::string remoteName = slash == std::string::npos ? item.path : item.path.substr(slash + 1);
    const std::string localParent = remoteParent.empty() ? std::string() : localDirs.at(remoteParent);
    const fs::path localParentPath = localParent.empty() ? m_dir : m_dir / fromUtf8(localParent);

    auto &used = usedNames[localParent];
    const auto name = uniqueName(sanitizeComponent(remoteName), item.isDirectory, [&](const std::string &candidate) {
      std::error_code existsError;
      return used.contains(nameKey(candidate)) || fs::exists(localParentPath / fromUtf8(candidate), existsError);
    });
    used.insert(nameKey(name));

    Entry entry;
    entry.remotePath = item.path;
    entry.localPath = localParent.empty() ? name : localParent + "/" + name;
    entry.size = item.size;
    entry.isDirectory = item.isDirectory;

    const auto path = m_dir / fromUtf8(entry.localPath);
    if (item.isDirectory) {
      localDirs[item.path] = entry.localPath;
      if (!fs::create_directory(path, ec)) {
        reason = CancelReason::WriteFailed;
        setError(error, "cannot create staged folder");
        failLocked();
        return false;
      }
    } else {
      // create every file up front so readers can open it at any time
      std::ofstream create(path, std::ios::binary | std::ios::trunc);
      if (!create) {
        reason = CancelReason::WriteFailed;
        setError(error, "cannot create staged file");
        failLocked();
        return false;
      }
    }
    m_entries.push_back(std::move(entry));
  }
  return true;
}

bool FileDragStaging::fileStart(uint32_t index, uint64_t size)
{
  std::scoped_lock lock(m_mutex);
  if (!m_begun || m_cancelled || index >= m_entries.size() || m_writing >= 0) {
    return false;
  }
  const auto &entry = m_entries[index];
  auto &state = m_files[index];
  if (entry.isDirectory || state.started || size != entry.size) {
    return false;
  }
  m_out.open(m_dir / fromUtf8(entry.localPath), std::ios::binary | std::ios::trunc);
  if (!m_out) {
    return false;
  }
  state.started = true;
  m_writing = index;
  return true;
}

bool FileDragStaging::write(uint32_t index, const std::string &data)
{
  std::unique_lock lock(m_mutex);
  if (m_cancelled || static_cast<int64_t>(index) != m_writing) {
    return false;
  }
  auto &state = m_files[index];
  const auto &entry = m_entries[index];
  if (data.size() > entry.size - state.written) {
    return false;
  }
  m_out.write(data.data(), static_cast<std::streamsize>(data.size()));
  // readers use their own handles, so the bytes must reach the file now
  m_out.flush();
  if (!m_out) {
    return false;
  }
  state.written += data.size();
  m_cond.notify_all();
  return true;
}

bool FileDragStaging::fileEnd(uint32_t index)
{
  std::scoped_lock lock(m_mutex);
  if (m_cancelled || static_cast<int64_t>(index) != m_writing) {
    return false;
  }
  auto &state = m_files[index];
  if (state.written != m_entries[index].size) {
    return false;
  }
  m_out.close();
  if (m_out.fail()) {
    return false;
  }
  state.complete = true;
  m_writing = -1;
  m_cond.notify_all();
  return true;
}

void FileDragStaging::cancel()
{
  std::scoped_lock lock(m_mutex);
  failLocked();
}

void FileDragStaging::failLocked()
{
  if (m_out.is_open()) {
    m_out.close();
  }
  m_writing = -1;
  m_cancelled = true;
  m_cond.notify_all();
  removeAllLocked();
}

void FileDragStaging::removeAllLocked()
{
  if (m_dir.empty()) {
    return;
  }
  std::error_code ec;
  fs::remove_all(m_dir, ec);
}

bool FileDragStaging::removeStaged()
{
  std::scoped_lock lock(m_mutex);
  if (m_out.is_open()) {
    m_out.close();
  }
  m_writing = -1;
  removeAllLocked();
  std::error_code ec;
  return m_dir.empty() || !fs::exists(m_dir, ec);
}

FileDragStaging::WaitResult FileDragStaging::waitForData(
    uint32_t index, uint64_t offset, uint64_t &available, std::chrono::milliseconds timeout
) const
{
  std::unique_lock lock(m_mutex);
  if (index >= m_entries.size() || m_entries[index].isDirectory) {
    available = 0;
    return WaitResult::Failed;
  }
  const auto deadline = std::chrono::steady_clock::now() + timeout;
  while (true) {
    const auto &state = m_files[index];
    available = state.written;
    if (m_cancelled) {
      return WaitResult::Failed;
    }
    if (offset < state.written) {
      return WaitResult::Data;
    }
    if (state.complete) {
      return WaitResult::Eof;
    }
    if (m_cond.wait_until(lock, deadline) == std::cv_status::timeout) {
      available = m_files[index].written;
      if (m_cancelled) {
        return WaitResult::Failed;
      }
      return offset < available ? WaitResult::Data : WaitResult::Timeout;
    }
  }
}

std::vector<uint32_t> FileDragStaging::topLevelIndices() const
{
  std::scoped_lock lock(m_mutex);
  std::vector<uint32_t> tops;
  for (size_t i = 0; i < m_entries.size(); ++i) {
    if (m_entries[i].localPath.find('/') == std::string::npos) {
      tops.push_back(static_cast<uint32_t>(i));
    }
  }
  return tops;
}

FileDragStaging::WaitResult FileDragStaging::waitForItem(uint32_t top, std::chrono::milliseconds timeout) const
{
  std::unique_lock lock(m_mutex);
  if (!m_begun || top >= m_entries.size() || m_entries[top].localPath.find('/') != std::string::npos) {
    return WaitResult::Failed;
  }

  // the files of this item: itself, or everything below the folder
  std::vector<size_t> files;
  const auto &topEntry = m_entries[top];
  if (!topEntry.isDirectory) {
    files.push_back(top);
  } else {
    const std::string prefix = topEntry.localPath + "/";
    for (size_t i = 0; i < m_entries.size(); ++i) {
      if (!m_entries[i].isDirectory && m_entries[i].localPath.starts_with(prefix)) {
        files.push_back(i);
      }
    }
  }

  const auto allComplete = [this, &files] {
    return std::all_of(files.begin(), files.end(), [this](size_t i) { return m_files[i].complete; });
  };
  const auto deadline = std::chrono::steady_clock::now() + timeout;
  while (true) {
    if (m_cancelled) {
      return WaitResult::Failed;
    }
    if (allComplete()) {
      return WaitResult::Eof;
    }
    if (m_cond.wait_until(lock, deadline) == std::cv_status::timeout) {
      if (m_cancelled) {
        return WaitResult::Failed;
      }
      return allComplete() ? WaitResult::Eof : WaitResult::Timeout;
    }
  }
}

bool FileDragStaging::isComplete() const
{
  std::scoped_lock lock(m_mutex);
  if (!m_begun || m_cancelled) {
    return false;
  }
  for (size_t i = 0; i < m_entries.size(); ++i) {
    if (!m_entries[i].isDirectory && !m_files[i].complete) {
      return false;
    }
  }
  return true;
}

bool FileDragStaging::isCancelled() const
{
  std::scoped_lock lock(m_mutex);
  return m_cancelled;
}

uint64_t FileDragStaging::receivedBytes() const
{
  std::scoped_lock lock(m_mutex);
  uint64_t total = 0;
  for (const auto &state : m_files) {
    total += state.written;
  }
  return total;
}

fs::path FileDragStaging::pathOf(uint32_t index) const
{
  if (index >= m_entries.size()) {
    return {};
  }
  return m_dir / fromUtf8(m_entries[index].localPath);
}

bool FileDragStaging::moveNoReplace(
    const fs::path &source, const fs::path &destinationDir, const std::string &name, bool isDirectory, fs::path &placed,
    std::string *error
)
{
  std::set<std::string> tried;
  for (int attempt = 0; attempt < 1000; ++attempt) {
    const auto candidate = uniqueName(name, isDirectory, [&](const std::string &n) {
      std::error_code ec;
      return tried.contains(nameKey(n)) || fs::exists(destinationDir / fromUtf8(n), ec);
    });
    tried.insert(nameKey(candidate));
    const auto target = destinationDir / fromUtf8(candidate);

    auto result = renameNoReplace(source, target);
    if (result == MoveResult::CrossDevice || (result == MoveResult::Failed && isDirectory)) {
      result = copyNoReplace(source, target, isDirectory);
    }
    switch (result) {
    case MoveResult::Moved:
      placed = target;
      return true;
    case MoveResult::Exists:
      // someone took the name between our check and the move: try the next one
      continue;
    case MoveResult::CrossDevice:
    case MoveResult::Failed:
      setError(error, "cannot move " + toUtf8(source.filename()));
      return false;
    }
  }
  setError(error, "no free name for " + name);
  return false;
}

bool FileDragStaging::moveInto(const fs::path &targetDir, std::vector<fs::path> *placed, std::string *error)
{
  if (!isComplete()) {
    setError(error, "drag is not complete");
    return false;
  }
  std::scoped_lock lock(m_mutex);
  if (m_moved) {
    setError(error, "already moved");
    return false;
  }
  std::error_code ec;
  if (!fs::is_directory(targetDir, ec)) {
    setError(error, "target is not a folder");
    return false;
  }
  for (const auto &entry : m_entries) {
    if (entry.localPath.find('/') != std::string::npos) {
      continue; // moved with its top-level folder
    }
    fs::path finalPath;
    if (!moveNoReplace(
            m_dir / fromUtf8(entry.localPath), targetDir, entry.localPath, entry.isDirectory, finalPath, error
        )) {
      return false;
    }
    if (placed != nullptr) {
      placed->push_back(finalPath);
    }
  }
  m_moved = true;
  return true;
}

} // namespace deskflow::filedrag
