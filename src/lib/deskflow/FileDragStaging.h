/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "deskflow/FileDragTypes.h"

#include <chrono>
#include <condition_variable>
#include <filesystem>
#include <fstream>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace deskflow::filedrag {

/**
 * @brief Receives the files of one drag into a private staging folder
 *
 * The staging folder is "<root>/drag-<id>-<n>", where root is normally
 * defaultRoot() (a "Deskflow" folder in the system temp folder). The item list is
 * validated again, every name is sanitized for the local file system and made unique,
 * and free disk space is checked before anything is created.
 *
 * One thread writes (begin, fileStart, write, fileEnd); any number of threads may wait
 * for data with waitForData() and read the staged files with their own handles while
 * they are still arriving. cancel() wakes every waiting reader with Failed and deletes
 * the staged files; the destructor does the same, so a dropped connection or an
 * abandoned drag never leaves partial files behind.
 */
class FileDragStaging
{
public:
  //! A staged item
  struct Entry
  {
    std::string remotePath; ///< Path as sent by the peer
    std::string localPath;  ///< Sanitized, unique relative path ('/' separated) inside the staging folder
    uint64_t size = 0;
    bool isDirectory = false;
  };

  enum class WaitResult
  {
    Data,   ///< More bytes are available at the requested offset
    Eof,    ///< The file is complete and the offset is at (or past) its end
    Failed, ///< The drag was cancelled or failed
    Timeout ///< Nothing new arrived within the timeout
  };

  //! "<system temp>/Deskflow"
  static std::filesystem::path defaultRoot();

  /**
   * @brief Deletes staging folders left behind by an earlier run (crash, power loss)
   *
   * Only folders named "drag-*" directly in \p root are removed.
   */
  static void sweepStale(const std::filesystem::path &root);

  explicit FileDragStaging(std::filesystem::path root = defaultRoot());
  ~FileDragStaging();

  FileDragStaging(const FileDragStaging &) = delete;
  FileDragStaging &operator=(const FileDragStaging &) = delete;

  /**
   * @brief Validates the items, checks free space and creates the folders
   *
   * On failure nothing is left on disk and \p reason says why (Invalid or NoSpace or
   * WriteFailed).
   */
  bool begin(uint32_t dragId, const ItemList &items, CancelReason &reason, std::string *error = nullptr);

  //! Starts writing file \p index; \p size must match the item list
  bool fileStart(uint32_t index, uint64_t size);
  //! Appends to the file that is being written; never past its announced size
  bool write(uint32_t index, const std::string &data);
  //! Finishes file \p index; it must have exactly its announced size
  bool fileEnd(uint32_t index);

  /**
   * @brief Fails the drag: wakes readers and deletes every staged file
   *
   * Safe to call more than once and from any thread.
   */
  void cancel();

  /**
   * @brief Deletes the staging folder after a successful drop
   *
   * Returns false if something is still open; call again later (the destructor and
   * sweepStale() retry).
   */
  bool removeStaged();

  /**
   * @brief Waits until file \p index has data at \p offset (thread-safe)
   *
   * \p available is set to the number of bytes staged so far.
   */
  WaitResult waitForData(uint32_t index, uint64_t offset, uint64_t &available, std::chrono::milliseconds timeout) const;

  /**
   * @brief Waits until every file of top-level item \p top has arrived (thread-safe)
   *
   * \p top is a top-level index (see topLevelIndices()); for a folder this covers every
   * file inside it. Returns Eof when all of them are complete (at once for an empty
   * folder), Failed if the drag was cancelled or \p top is not a top-level item, and
   * Timeout if something is still arriving after \p timeout.
   */
  WaitResult waitForItem(uint32_t top, std::chrono::milliseconds timeout) const;

  //! Indices of the top-level items (those directly in the staging folder), in order
  std::vector<uint32_t> topLevelIndices() const;

  //! Moves the staged items into \p targetDir, never overwriting (see moveNoReplace)
  bool moveInto(
      const std::filesystem::path &targetDir, std::vector<std::filesystem::path> *placed = nullptr,
      std::string *error = nullptr
  );

  uint32_t dragId() const
  {
    return m_dragId;
  }
  bool isComplete() const;
  bool isCancelled() const;
  uint64_t totalBytes() const
  {
    return m_totalBytes;
  }
  uint64_t receivedBytes() const;
  const std::vector<Entry> &entries() const
  {
    return m_entries;
  }
  std::filesystem::path stagingDir() const
  {
    return m_dir;
  }
  std::filesystem::path pathOf(uint32_t index) const;

  /**
   * @brief Moves \p source to \p destinationDir under a free name
   *
   * Picks \p name, or "name (2).ext", "name (3).ext", ... and never replaces an existing
   * file or folder. Works across volumes (copy then delete). \p placed receives the
   * final path.
   */
  static bool moveNoReplace(
      const std::filesystem::path &source, const std::filesystem::path &destinationDir, const std::string &name,
      bool isDirectory, std::filesystem::path &placed, std::string *error = nullptr
  );

  //! Free bytes on the volume of \p path, or UINT64_MAX if unknown
  static uint64_t freeSpace(const std::filesystem::path &path);

  //! Lets tests pretend the disk is (nearly) full
  void setFreeSpaceOverride(uint64_t bytes)
  {
    m_freeSpaceOverride = bytes;
  }

private:
  struct FileState
  {
    uint64_t written = 0;
    bool started = false;
    bool complete = false;
  };

  void failLocked();
  void removeAllLocked();

  std::filesystem::path m_root;
  std::filesystem::path m_dir;
  uint32_t m_dragId = 0;
  std::vector<Entry> m_entries;
  std::vector<FileState> m_files;
  uint64_t m_totalBytes = 0;
  uint64_t m_freeSpaceOverride = UINT64_MAX;

  mutable std::mutex m_mutex;
  mutable std::condition_variable m_cond;
  bool m_cancelled = false;
  bool m_begun = false;
  bool m_moved = false;
  int64_t m_writing = -1;
  std::ofstream m_out;
};

} // namespace deskflow::filedrag
