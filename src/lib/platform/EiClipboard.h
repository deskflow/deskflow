/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "deskflow/IClipboard.h"
#include "platform/Wayland.h"

#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace deskflow {

class EiClipboardMonitor;
class PortalClipboard;
class EiClipboardNegotiator;
class EiClipboardSync;

//! Wayland clipboard implementation using XDG Desktop Portal
/*!
This class implements clipboard functionality for Wayland using the XDG Desktop Portal
system. It provides a bridge between Deskflow's IClipboard interface and the portal
clipboard API when available.
*/
class EiClipboard : public IClipboard
{
public:
  EiClipboard();
  ~EiClipboard() override;

  //! Check if portal clipboard is available
  /*!
  Returns true if the XDG Desktop Portal clipboard interface is available
  and functional on the current system.
  */
  bool isPortalAvailable() const;

  //! Start clipboard change monitoring
  bool startMonitoring();

  //! Stop clipboard change monitoring
  void stopMonitoring();

  //! Check if monitoring is active
  bool isMonitoring() const;

  //! Set maximum clipboard data size (in bytes)
  void setMaxDataSize(size_t maxSize);

  //! Get maximum clipboard data size
  size_t getMaxDataSize() const;

  //! Clear clipboard cache
  void clearCache();

  //! Get format negotiator
  std::shared_ptr<EiClipboardNegotiator> getNegotiator() const;

  //! Select best format for available options
  std::string selectBestMimeType(EFormat format, const std::vector<std::string> &availableTypes) const;

  //! Get synchronization system
  std::shared_ptr<EiClipboardSync> getSync() const;

  //! Enable/disable bandwidth optimization
  void setSyncOptimizationEnabled(bool enabled);

  // IClipboard interface
  bool empty() override;
  void add(EFormat format, const std::string &data) override;
  bool open(Time time) const override;
  void close() const override;
  Time getTime() const override;
  bool has(EFormat format) const override;
  std::string get(EFormat format) const override;

private:
  //! Initialize the portal connection
  void initPortal();

  //! Convert Deskflow format to MIME type
  std::string formatToMimeType(EFormat format) const;

  //! Convert Deskflow format to list of supported MIME types
  std::vector<std::string> formatToMimeTypes(EFormat format) const;

  //! Convert MIME type to Deskflow format
  EFormat mimeTypeToFormat(const std::string &mimeType) const;

  //! Check if MIME type is supported
  bool isSupportedMimeType(const std::string &mimeType) const;

  //! Wait for asynchronous portal operation to complete
  void waitForOperation() const;

  //! Check if portal D-Bus service is running
  bool checkPortalService() const;

  //! Check if clipboard interface is available
  bool checkClipboardInterface() const;

  //! Validate clipboard data for security and format compliance
  bool validateClipboardData(EFormat format, const std::string &data) const;

  //! Sanitize clipboard data to remove potentially harmful content
  std::string sanitizeClipboardData(EFormat format, const std::string &data) const;

  //! Check if data contains potentially sensitive information
  bool containsSensitiveData(const std::string &data) const;

  //! Calculate data hash for change detection
  std::string calculateDataHash(const std::string &data) const;

  // Portal state
  mutable std::mutex m_mutex;
  mutable std::condition_variable m_cv;
  mutable bool m_operationComplete;
  mutable bool m_operationSuccess;
  mutable std::string m_operationError;

  // Clipboard state
  mutable bool m_open;
  mutable Time m_time;
  bool m_added[kNumFormats];
  std::string m_data[kNumFormats];
  mutable bool m_cacheValid[kNumFormats];
  mutable std::string m_cachedData[kNumFormats];

  // Portal availability
  bool m_portalAvailable;

  // Clipboard monitoring
  std::unique_ptr<EiClipboardMonitor> m_monitor;

  // Portal clipboard interface
  std::unique_ptr<PortalClipboard> m_portalClipboard;

  // Format negotiation
  std::shared_ptr<EiClipboardNegotiator> m_negotiator;

  // Synchronization system
  std::shared_ptr<EiClipboardSync> m_sync;
  bool m_syncOptimizationEnabled;

  // Performance settings
  size_t m_maxDataSize;
  static constexpr size_t kDefaultMaxDataSize = 16 * 1024 * 1024; // 16MB

  // Cache management
  mutable std::chrono::steady_clock::time_point m_cacheTimestamp[kNumFormats];
  mutable std::chrono::milliseconds m_cacheTimeout;
  static constexpr std::chrono::milliseconds kDefaultCacheTimeout{5000}; // 5 seconds
};

} // namespace deskflow
