/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/EiClipboardHistory.h"
#include "base/Log.h"

#include <algorithm>

namespace deskflow {

EiClipboardHistory::EiClipboardHistory(const Config &config) : m_config(config), m_totalSize(0)
{
  LOG_DEBUG(
      "clipboard history initialized with max %zu entries, %zu bytes", m_config.maxEntries, m_config.maxTotalSize
  );
}

EiClipboardHistory::~EiClipboardHistory()
{
  clear();
}

void EiClipboardHistory::addEntry(
    IClipboard::EFormat format, const std::string &data, const std::string &hash, bool containsSensitiveData,
    const std::string &source
)
{
  std::lock_guard<std::mutex> lock(m_mutex);

  // Create new entry
  auto entry = std::make_shared<ClipboardHistoryEntry>(format, data, hash, containsSensitiveData, source);

  // Check if we should store this entry
  if (!shouldStore(*entry)) {
    LOG_DEBUG("skipping clipboard history entry (sensitive data disabled)");
    return;
  }

  // Check for duplicates if deduplication is enabled
  if (m_config.enableDeduplication && hasHash(hash)) {
    LOG_DEBUG("skipping duplicate clipboard entry");
    return;
  }

  // Add to front of history
  m_history.push_front(entry);
  m_totalSize += entry->size;

  LOG_DEBUG("added clipboard history entry: format=%d, size=%zu, hash=%s", format, entry->size, hash.c_str());

  // Cleanup if necessary
  cleanup();
}

std::vector<std::shared_ptr<ClipboardHistoryEntry>> EiClipboardHistory::getHistory() const
{
  std::lock_guard<std::mutex> lock(m_mutex);
  return std::vector<std::shared_ptr<ClipboardHistoryEntry>>(m_history.begin(), m_history.end());
}

std::vector<std::shared_ptr<ClipboardHistoryEntry>> EiClipboardHistory::getHistory(IClipboard::EFormat format) const
{
  std::lock_guard<std::mutex> lock(m_mutex);

  std::vector<std::shared_ptr<ClipboardHistoryEntry>> result;
  for (const auto &entry : m_history) {
    if (entry->format == format) {
      result.push_back(entry);
    }
  }
  return result;
}

std::shared_ptr<ClipboardHistoryEntry> EiClipboardHistory::getMostRecent() const
{
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_history.empty() ? nullptr : m_history.front();
}

std::shared_ptr<ClipboardHistoryEntry> EiClipboardHistory::getMostRecent(IClipboard::EFormat format) const
{
  std::lock_guard<std::mutex> lock(m_mutex);

  for (const auto &entry : m_history) {
    if (entry->format == format) {
      return entry;
    }
  }
  return nullptr;
}

std::shared_ptr<ClipboardHistoryEntry> EiClipboardHistory::getEntry(size_t index) const
{
  std::lock_guard<std::mutex> lock(m_mutex);
  return (index < m_history.size()) ? m_history[index] : nullptr;
}

std::shared_ptr<ClipboardHistoryEntry> EiClipboardHistory::findByHash(const std::string &hash) const
{
  std::lock_guard<std::mutex> lock(m_mutex);

  for (const auto &entry : m_history) {
    if (entry->hash == hash) {
      return entry;
    }
  }
  return nullptr;
}

bool EiClipboardHistory::hasHash(const std::string &hash) const
{
  // Note: This method assumes the mutex is already locked by the caller
  for (const auto &entry : m_history) {
    if (entry->hash == hash) {
      return true;
    }
  }
  return false;
}

void EiClipboardHistory::clear()
{
  std::lock_guard<std::mutex> lock(m_mutex);

  LOG_DEBUG("clearing clipboard history (%zu entries)", m_history.size());
  m_history.clear();
  m_totalSize = 0;
}

void EiClipboardHistory::clearOld(std::chrono::minutes maxAge)
{
  std::lock_guard<std::mutex> lock(m_mutex);

  auto cutoff = std::chrono::steady_clock::now() - maxAge;
  size_t removedCount = 0;

  auto it = m_history.begin();
  while (it != m_history.end()) {
    if ((*it)->timestamp < cutoff) {
      m_totalSize -= (*it)->size;
      it = m_history.erase(it);
      ++removedCount;
    } else {
      ++it;
    }
  }

  if (removedCount > 0) {
    LOG_DEBUG("removed %zu old clipboard entries", removedCount);
  }
}

void EiClipboardHistory::clear(IClipboard::EFormat format)
{
  std::lock_guard<std::mutex> lock(m_mutex);

  size_t removedCount = 0;
  auto it = m_history.begin();
  while (it != m_history.end()) {
    if ((*it)->format == format) {
      m_totalSize -= (*it)->size;
      it = m_history.erase(it);
      ++removedCount;
    } else {
      ++it;
    }
  }

  if (removedCount > 0) {
    LOG_DEBUG("removed %zu clipboard entries for format %d", removedCount, format);
  }
}

size_t EiClipboardHistory::size() const
{
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_history.size();
}

size_t EiClipboardHistory::totalSize() const
{
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_totalSize;
}

const EiClipboardHistory::Config &EiClipboardHistory::getConfig() const
{
  return m_config;
}

void EiClipboardHistory::setConfig(const Config &config)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  m_config = config;
  cleanup(); // Apply new limits immediately
}

EiClipboardHistory::Statistics EiClipboardHistory::getStatistics() const
{
  std::lock_guard<std::mutex> lock(m_mutex);

  Statistics stats = {};
  stats.totalEntries = m_history.size();
  stats.totalSize = m_totalSize;

  if (!m_history.empty()) {
    stats.newestEntry = m_history.front()->timestamp;
    stats.oldestEntry = m_history.back()->timestamp;
  }

  for (const auto &entry : m_history) {
    if (entry->format >= 0 && entry->format < IClipboard::kNumFormats) {
      stats.entriesByFormat[entry->format]++;
    }
    if (entry->containsSensitiveData) {
      stats.sensitiveEntries++;
    }
  }

  return stats;
}

void EiClipboardHistory::cleanup()
{
  // Remove entries exceeding count limit
  while (m_history.size() > m_config.maxEntries) {
    auto &entry = m_history.back();
    m_totalSize -= entry->size;
    m_history.pop_back();
  }

  // Remove entries exceeding size limit
  while (m_totalSize > m_config.maxTotalSize && !m_history.empty()) {
    auto &entry = m_history.back();
    m_totalSize -= entry->size;
    m_history.pop_back();
  }

  // Remove entries exceeding age limit
  clearOld(m_config.maxAge);

  // Deduplicate if enabled
  if (m_config.enableDeduplication) {
    deduplicate();
  }
}

void EiClipboardHistory::deduplicate()
{
  std::vector<std::string> seenHashes;
  auto it = m_history.begin();

  while (it != m_history.end()) {
    const std::string &hash = (*it)->hash;

    if (std::find(seenHashes.begin(), seenHashes.end(), hash) != seenHashes.end()) {
      // Duplicate found, remove it
      m_totalSize -= (*it)->size;
      it = m_history.erase(it);
    } else {
      seenHashes.push_back(hash);
      ++it;
    }
  }
}

bool EiClipboardHistory::shouldStore(const ClipboardHistoryEntry &entry) const
{
  // Don't store sensitive data if disabled
  if (entry.containsSensitiveData && !m_config.enableSensitiveData) {
    return false;
  }

  // Don't store empty data
  if (entry.data.empty()) {
    return false;
  }

  return true;
}

} // namespace deskflow
