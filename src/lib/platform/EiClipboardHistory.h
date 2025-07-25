/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "deskflow/IClipboard.h"
#include "platform/Wayland.h"

#include <chrono>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace deskflow {

//! Clipboard history entry
struct ClipboardHistoryEntry
{
    //! Clipboard format
    IClipboard::EFormat format;
    
    //! Clipboard data
    std::string data;
    
    //! Data hash for deduplication
    std::string hash;
    
    //! Timestamp when data was added
    std::chrono::steady_clock::time_point timestamp;
    
    //! Size of the data in bytes
    size_t size;
    
    //! Whether this entry contains sensitive data
    bool containsSensitiveData;
    
    //! Source of the clipboard data (local, remote, etc.)
    std::string source;

    ClipboardHistoryEntry(
        IClipboard::EFormat fmt,
        const std::string& d,
        const std::string& h,
        bool sensitive = false,
        const std::string& src = "unknown")
        : format(fmt),
          data(d),
          hash(h),
          timestamp(std::chrono::steady_clock::now()),
          size(d.size()),
          containsSensitiveData(sensitive),
          source(src)
    {
    }
};

//! Clipboard history manager
/*!
This class manages clipboard history, providing versioning, deduplication,
and efficient storage of clipboard entries. It supports configurable
history size limits and automatic cleanup of old entries.
*/
class EiClipboardHistory
{
public:
    //! History configuration
    struct Config {
        size_t maxEntries = 100;                    // Maximum number of history entries
        size_t maxTotalSize = 50 * 1024 * 1024;     // Maximum total size (50MB)
        std::chrono::minutes maxAge{60};            // Maximum age of entries (1 hour)
        bool enableSensitiveData = false;          // Whether to store sensitive data
        bool enableDeduplication = true;           // Whether to deduplicate entries
    };

    explicit EiClipboardHistory(const Config& config = Config{});
    ~EiClipboardHistory();

    //! Add entry to history
    void addEntry(
        IClipboard::EFormat format,
        const std::string& data,
        const std::string& hash,
        bool containsSensitiveData = false,
        const std::string& source = "local");

    //! Get history entries (most recent first)
    std::vector<std::shared_ptr<ClipboardHistoryEntry>> getHistory() const;

    //! Get history entries for specific format
    std::vector<std::shared_ptr<ClipboardHistoryEntry>> getHistory(IClipboard::EFormat format) const;

    //! Get most recent entry
    std::shared_ptr<ClipboardHistoryEntry> getMostRecent() const;

    //! Get most recent entry for specific format
    std::shared_ptr<ClipboardHistoryEntry> getMostRecent(IClipboard::EFormat format) const;

    //! Get entry by index (0 = most recent)
    std::shared_ptr<ClipboardHistoryEntry> getEntry(size_t index) const;

    //! Find entry by hash
    std::shared_ptr<ClipboardHistoryEntry> findByHash(const std::string& hash) const;

    //! Check if hash exists in history
    bool hasHash(const std::string& hash) const;

    //! Clear all history
    void clear();

    //! Clear entries older than specified age
    void clearOld(std::chrono::minutes maxAge);

    //! Clear entries for specific format
    void clear(IClipboard::EFormat format);

    //! Get current history size (number of entries)
    size_t size() const;

    //! Get total data size in bytes
    size_t totalSize() const;

    //! Get configuration
    const Config& getConfig() const;

    //! Update configuration
    void setConfig(const Config& config);

    //! Get history statistics
    struct Statistics {
        size_t totalEntries;
        size_t totalSize;
        size_t entriesByFormat[IClipboard::kNumFormats];
        size_t sensitiveEntries;
        std::chrono::steady_clock::time_point oldestEntry;
        std::chrono::steady_clock::time_point newestEntry;
    };
    
    Statistics getStatistics() const;

private:
    //! Cleanup old entries based on configuration
    void cleanup();

    //! Remove duplicate entries
    void deduplicate();

    //! Check if entry should be stored based on configuration
    bool shouldStore(const ClipboardHistoryEntry& entry) const;

    mutable std::mutex m_mutex;
    std::deque<std::shared_ptr<ClipboardHistoryEntry>> m_history;
    Config m_config;
    size_t m_totalSize;
};

} // namespace deskflow
