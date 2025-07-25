/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/EiClipboardMetrics.h"
#include "base/Log.h"

#include <algorithm>
#include <iomanip>
#include <sstream>

namespace deskflow {

EiClipboardMetrics::EiClipboardMetrics(const Config &config)
    : m_config(config),
      m_statsValid(false),
      m_startTime(std::chrono::steady_clock::now())
{
  LOG_DEBUG("clipboard metrics initialized (enabled: %s)", m_config.enabled ? "true" : "false");
}

EiClipboardMetrics::~EiClipboardMetrics() = default;

void EiClipboardMetrics::recordOperation(const ClipboardOperationMetrics &metrics)
{
  if (!m_config.enabled) {
    return;
  }

  std::lock_guard<std::mutex> lock(m_mutex);

  // Add to metrics history
  m_metrics.push_back(metrics);

  // Update counters
  m_totalOperations++;
  if (metrics.success) {
    m_successfulOperations++;
  } else {
    m_failedOperations++;
    if (!metrics.error.empty()) {
      m_errorCounts[metrics.error]++;
    }
  }

  m_totalDataTransferred += metrics.dataSize;
  m_formatCounts[metrics.format]++;
  if (!metrics.mimeType.empty()) {
    m_mimeTypeCounts[metrics.mimeType]++;
  }

  // Invalidate cached stats
  m_statsValid = false;

  // Cleanup old metrics if needed
  if (m_metrics.size() > m_config.maxMetricsHistory) {
    size_t toRemove = m_metrics.size() - m_config.maxMetricsHistory;
    m_metrics.erase(m_metrics.begin(), m_metrics.begin() + toRemove);
  }

  LOG_DEBUG(
      "recorded clipboard operation: %d, format: %d, duration: %lld μs, size: %zu", static_cast<int>(metrics.operation),
      metrics.format, metrics.duration.count(), metrics.dataSize
  );
}

EiClipboardMetrics::OperationTimer::OperationTimer(
    EiClipboardMetrics *metrics, ClipboardOperationMetrics::Operation operation, IClipboard::EFormat format
)
    : m_metrics(metrics),
      m_operation(operation, format),
      m_startTime(std::chrono::steady_clock::now())
{
}

EiClipboardMetrics::OperationTimer::~OperationTimer()
{
  if (m_metrics) {
    auto endTime = std::chrono::steady_clock::now();
    m_operation.duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - m_startTime);
    m_metrics->recordOperation(m_operation);
  }
}

void EiClipboardMetrics::OperationTimer::setDataSize(size_t size)
{
  m_operation.dataSize = size;
}

void EiClipboardMetrics::OperationTimer::setMimeType(const std::string &mimeType)
{
  m_operation.mimeType = mimeType;
}

void EiClipboardMetrics::OperationTimer::setSuccess(bool success)
{
  m_operation.success = success;
}

void EiClipboardMetrics::OperationTimer::setError(const std::string &error)
{
  m_operation.error = error;
  m_operation.success = false;
}

std::unique_ptr<EiClipboardMetrics::OperationTimer>
EiClipboardMetrics::startOperation(ClipboardOperationMetrics::Operation operation, IClipboard::EFormat format)
{
  if (!m_config.enabled) {
    return nullptr;
  }

  return std::make_unique<OperationTimer>(this, operation, format);
}

PerformanceStats EiClipboardMetrics::getPerformanceStats() const
{
  std::lock_guard<std::mutex> lock(m_mutex);

  // Check if cached stats are still valid
  auto now = std::chrono::steady_clock::now();
  if (m_statsValid && (now - m_lastStatsUpdate) < std::chrono::seconds(5)) {
    return m_cachedStats;
  }

  m_cachedStats = calculateStats(m_metrics);
  m_lastStatsUpdate = now;
  m_statsValid = true;

  return m_cachedStats;
}

PerformanceStats EiClipboardMetrics::getPerformanceStats(ClipboardOperationMetrics::Operation operation) const
{
  std::lock_guard<std::mutex> lock(m_mutex);

  std::vector<ClipboardOperationMetrics> filtered;
  std::copy_if(
      m_metrics.begin(), m_metrics.end(), std::back_inserter(filtered),
      [operation](const ClipboardOperationMetrics &m) { return m.operation == operation; }
  );

  return calculateStats(filtered);
}

PerformanceStats EiClipboardMetrics::getPerformanceStats(IClipboard::EFormat format) const
{
  std::lock_guard<std::mutex> lock(m_mutex);

  std::vector<ClipboardOperationMetrics> filtered;
  std::copy_if(
      m_metrics.begin(), m_metrics.end(), std::back_inserter(filtered),
      [format](const ClipboardOperationMetrics &m) { return m.format == format; }
  );

  return calculateStats(filtered);
}

std::vector<ClipboardOperationMetrics> EiClipboardMetrics::getRecentMetrics(size_t count) const
{
  std::lock_guard<std::mutex> lock(m_mutex);

  if (m_metrics.size() <= count) {
    return m_metrics;
  }

  return std::vector<ClipboardOperationMetrics>(m_metrics.end() - count, m_metrics.end());
}

std::vector<ClipboardOperationMetrics> EiClipboardMetrics::getMetrics(
    std::chrono::steady_clock::time_point start, std::chrono::steady_clock::time_point end
) const
{
  std::lock_guard<std::mutex> lock(m_mutex);

  std::vector<ClipboardOperationMetrics> filtered;
  std::copy_if(
      m_metrics.begin(), m_metrics.end(), std::back_inserter(filtered),
      [start, end](const ClipboardOperationMetrics &m) { return m.timestamp >= start && m.timestamp <= end; }
  );

  return filtered;
}

std::map<std::string, size_t> EiClipboardMetrics::getErrorStatistics() const
{
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_errorCounts;
}

std::map<IClipboard::EFormat, size_t> EiClipboardMetrics::getFormatUsage() const
{
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_formatCounts;
}

std::map<std::string, size_t> EiClipboardMetrics::getMimeTypeUsage() const
{
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_mimeTypeCounts;
}

double EiClipboardMetrics::getOperationFrequency() const
{
  auto now = std::chrono::steady_clock::now();
  auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - m_startTime);

  if (elapsed.count() == 0) {
    return 0.0;
  }

  return static_cast<double>(m_totalOperations.load()) / elapsed.count();
}

double EiClipboardMetrics::getDataThroughput() const
{
  auto now = std::chrono::steady_clock::now();
  auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - m_startTime);

  if (elapsed.count() == 0) {
    return 0.0;
  }

  return static_cast<double>(m_totalDataTransferred.load()) / elapsed.count();
}

void EiClipboardMetrics::clearMetrics()
{
  std::lock_guard<std::mutex> lock(m_mutex);

  m_metrics.clear();
  m_errorCounts.clear();
  m_formatCounts.clear();
  m_mimeTypeCounts.clear();

  m_totalOperations = 0;
  m_successfulOperations = 0;
  m_failedOperations = 0;
  m_totalDataTransferred = 0;

  m_statsValid = false;
  m_startTime = std::chrono::steady_clock::now();

  LOG_DEBUG("clipboard metrics cleared");
}

void EiClipboardMetrics::cleanupOldMetrics()
{
  if (!m_config.enabled) {
    return;
  }

  std::lock_guard<std::mutex> lock(m_mutex);

  auto cutoff = std::chrono::steady_clock::now() - m_config.metricsRetention;

  auto it = std::remove_if(m_metrics.begin(), m_metrics.end(), [cutoff](const ClipboardOperationMetrics &m) {
    return m.timestamp < cutoff;
  });

  size_t removed = std::distance(it, m_metrics.end());
  m_metrics.erase(it, m_metrics.end());

  if (removed > 0) {
    LOG_DEBUG("cleaned up %zu old clipboard metrics", removed);
    m_statsValid = false;
  }
}

const EiClipboardMetrics::Config &EiClipboardMetrics::getConfig() const
{
  return m_config;
}

void EiClipboardMetrics::setConfig(const Config &config)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  m_config = config;

  if (!config.enabled) {
    clearMetrics();
  }
}

bool EiClipboardMetrics::isEnabled() const
{
  return m_config.enabled;
}

void EiClipboardMetrics::setEnabled(bool enabled)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  m_config.enabled = enabled;

  if (!enabled) {
    clearMetrics();
  }
}

PerformanceStats EiClipboardMetrics::calculateStats(const std::vector<ClipboardOperationMetrics> &metrics) const
{
  PerformanceStats stats;

  if (metrics.empty()) {
    return stats;
  }

  stats.totalOperations = metrics.size();

  std::chrono::microseconds totalTime{0};
  size_t totalData = 0;

  for (const auto &metric : metrics) {
    if (metric.success) {
      stats.successfulOperations++;
    } else {
      stats.failedOperations++;
    }

    totalTime += metric.duration;
    totalData += metric.dataSize;

    if (metric.duration > stats.maxOperationTime) {
      stats.maxOperationTime = metric.duration;
    }
    if (metric.duration < stats.minOperationTime) {
      stats.minOperationTime = metric.duration;
    }

    if (metric.dataSize > stats.maxDataSize) {
      stats.maxDataSize = metric.dataSize;
    }

    stats.formatUsage[metric.format]++;
    if (!metric.mimeType.empty()) {
      stats.mimeTypeUsage[metric.mimeType]++;
    }
  }

  stats.averageOperationTime = totalTime / stats.totalOperations;
  stats.successRate = static_cast<double>(stats.successfulOperations) / stats.totalOperations;
  stats.totalDataTransferred = totalData;
  stats.averageDataSize = stats.totalOperations > 0 ? totalData / stats.totalOperations : 0;

  return stats;
}

std::string EiClipboardMetrics::exportMetricsJson() const
{
  // TODO: Implement JSON export
  return "{}";
}

std::string EiClipboardMetrics::generatePerformanceReport() const
{
  auto stats = getPerformanceStats();

  std::stringstream report;
  report << "=== Clipboard Performance Report ===\n";
  report << "Total Operations: " << stats.totalOperations << "\n";
  report << "Success Rate: " << std::fixed << std::setprecision(2) << (stats.successRate * 100) << "%\n";
  report << "Average Operation Time: " << stats.averageOperationTime.count() << " μs\n";
  report << "Max Operation Time: " << stats.maxOperationTime.count() << " μs\n";
  report << "Total Data Transferred: " << stats.totalDataTransferred << " bytes\n";
  report << "Average Data Size: " << stats.averageDataSize << " bytes\n";
  report << "Operation Frequency: " << std::fixed << std::setprecision(2) << getOperationFrequency() << " ops/sec\n";
  report << "Data Throughput: " << std::fixed << std::setprecision(2) << getDataThroughput() << " bytes/sec\n";

  return report.str();
}

} // namespace deskflow
