/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "deskflow/IClipboard.h"

#include <atomic>
#include <chrono>
#include <map>
#include <mutex>
#include <string>
#include <vector>

namespace deskflow {

//! Clipboard operation metrics
struct ClipboardOperationMetrics
{
  //! Operation type
  enum class Operation
  {
    Open,
    Close,
    Add,
    Get,
    Has,
    Empty,
    PortalSet,
    PortalGet,
    FormatNegotiation,
    DataValidation,
    DataSanitization
  };

  Operation operation;
  IClipboard::EFormat format;
  std::chrono::steady_clock::time_point timestamp;
  std::chrono::microseconds duration;
  size_t dataSize;
  bool success;
  std::string error;
  std::string mimeType;

  ClipboardOperationMetrics(
      Operation op, IClipboard::EFormat fmt = IClipboard::kText,
      std::chrono::microseconds dur = std::chrono::microseconds{0}, size_t size = 0, bool succ = true,
      const std::string &err = "", const std::string &mime = ""
  )
      : operation(op),
        format(fmt),
        timestamp(std::chrono::steady_clock::now()),
        duration(dur),
        dataSize(size),
        success(succ),
        error(err),
        mimeType(mime)
  {
  }
};

//! Performance statistics
struct PerformanceStats
{
  std::chrono::microseconds averageOperationTime{0};
  std::chrono::microseconds maxOperationTime{0};
  std::chrono::microseconds minOperationTime{std::chrono::microseconds::max()};
  size_t totalOperations = 0;
  size_t successfulOperations = 0;
  size_t failedOperations = 0;
  double successRate = 0.0;

  // Data transfer statistics
  size_t totalDataTransferred = 0;
  size_t averageDataSize = 0;
  size_t maxDataSize = 0;

  // Format usage statistics
  std::map<IClipboard::EFormat, size_t> formatUsage;
  std::map<std::string, size_t> mimeTypeUsage;
};

//! Clipboard metrics collector and analyzer
/*!
This class collects performance metrics, usage analytics, and diagnostic
information for clipboard operations. It provides insights into clipboard
performance, format usage patterns, and potential issues.
*/
class EiClipboardMetrics
{
public:
  //! Metrics configuration
  struct Config
  {
    bool enabled;                          // Enable metrics collection
    size_t maxMetricsHistory;              // Maximum metrics entries to keep
    std::chrono::minutes metricsRetention; // How long to keep metrics
    bool collectDetailedMetrics;           // Collect detailed operation metrics
    bool collectPerformanceStats;          // Collect performance statistics
    bool collectUsageAnalytics;            // Collect usage analytics

    Config()
        : enabled(true),
          maxMetricsHistory(10000),
          metricsRetention(60),
          collectDetailedMetrics(true),
          collectPerformanceStats(true),
          collectUsageAnalytics(true)
    {
    }
  };

  explicit EiClipboardMetrics(const Config &config = Config{});
  ~EiClipboardMetrics();

  //! Record clipboard operation
  void recordOperation(const ClipboardOperationMetrics &metrics);

  //! Start timing an operation
  class OperationTimer
  {
  public:
    OperationTimer(
        EiClipboardMetrics *metrics, ClipboardOperationMetrics::Operation operation,
        IClipboard::EFormat format = IClipboard::kText
    );
    ~OperationTimer();

    void setDataSize(size_t size);
    void setMimeType(const std::string &mimeType);
    void setSuccess(bool success);
    void setError(const std::string &error);

  private:
    EiClipboardMetrics *m_metrics;
    ClipboardOperationMetrics m_operation;
    std::chrono::steady_clock::time_point m_startTime;
  };

  //! Create operation timer
  std::unique_ptr<OperationTimer>
  startOperation(ClipboardOperationMetrics::Operation operation, IClipboard::EFormat format = IClipboard::kText);

  //! Get performance statistics
  PerformanceStats getPerformanceStats() const;

  //! Get performance statistics for specific operation
  PerformanceStats getPerformanceStats(ClipboardOperationMetrics::Operation operation) const;

  //! Get performance statistics for specific format
  PerformanceStats getPerformanceStats(IClipboard::EFormat format) const;

  //! Get recent metrics (last N operations)
  std::vector<ClipboardOperationMetrics> getRecentMetrics(size_t count = 100) const;

  //! Get metrics for time range
  std::vector<ClipboardOperationMetrics>
  getMetrics(std::chrono::steady_clock::time_point start, std::chrono::steady_clock::time_point end) const;

  //! Get error statistics
  std::map<std::string, size_t> getErrorStatistics() const;

  //! Get format usage statistics
  std::map<IClipboard::EFormat, size_t> getFormatUsage() const;

  //! Get MIME type usage statistics
  std::map<std::string, size_t> getMimeTypeUsage() const;

  //! Get operation frequency (operations per second)
  double getOperationFrequency() const;

  //! Get data throughput (bytes per second)
  double getDataThroughput() const;

  //! Clear all metrics
  void clearMetrics();

  //! Clear old metrics based on retention policy
  void cleanupOldMetrics();

  //! Get configuration
  const Config &getConfig() const;

  //! Update configuration
  void setConfig(const Config &config);

  //! Export metrics to JSON string
  std::string exportMetricsJson() const;

  //! Export performance report
  std::string generatePerformanceReport() const;

  //! Check if metrics collection is enabled
  bool isEnabled() const;

  //! Enable/disable metrics collection
  void setEnabled(bool enabled);

private:
  //! Update performance statistics
  void updatePerformanceStats(const ClipboardOperationMetrics &metrics);

  //! Calculate statistics from metrics
  PerformanceStats calculateStats(const std::vector<ClipboardOperationMetrics> &metrics) const;

  mutable std::mutex m_mutex;
  Config m_config;

  // Metrics storage
  std::vector<ClipboardOperationMetrics> m_metrics;

  // Cached statistics
  mutable PerformanceStats m_cachedStats;
  mutable std::chrono::steady_clock::time_point m_lastStatsUpdate;
  mutable bool m_statsValid;

  // Operation counters
  std::atomic<size_t> m_totalOperations{0};
  std::atomic<size_t> m_successfulOperations{0};
  std::atomic<size_t> m_failedOperations{0};
  std::atomic<size_t> m_totalDataTransferred{0};

  // Error tracking
  std::map<std::string, size_t> m_errorCounts;

  // Usage tracking
  std::map<IClipboard::EFormat, size_t> m_formatCounts;
  std::map<std::string, size_t> m_mimeTypeCounts;

  // Timing
  std::chrono::steady_clock::time_point m_startTime;
};

} // namespace deskflow
