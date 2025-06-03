/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "deskflow/ClipboardTypes.h"
#include "deskflow/IClipboard.h"

#include <string>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include <vector>
#include <fcntl.h>

//! Wayland clipboard implementation using wl-copy/wl-paste
/*!
This class implements clipboard functionality for Wayland environments
by using the wl-clipboard utilities (wl-copy and wl-paste).
*/
class WaylandClipboard : public IClipboard
{
public:
  WaylandClipboard(ClipboardID id);
  WaylandClipboard(WaylandClipboard const &) = delete;
  WaylandClipboard(WaylandClipboard &&) = delete;
  ~WaylandClipboard() override;

  WaylandClipboard &operator=(WaylandClipboard const &) = delete;
  WaylandClipboard &operator=(WaylandClipboard &&) = delete;

  //! Get clipboard ID
  ClipboardID getID() const;

  //! Check if wl-clipboard tools are available
  static bool isAvailable();

  //! Start monitoring clipboard changes
  void startMonitoring();

  //! Stop monitoring clipboard changes
  void stopMonitoring();

  //! Check if clipboard has changed
  bool hasChanged() const;

  //! Reset the changed flag and clear cache
  void resetChanged();

  // IClipboard overrides
  bool empty() override;
  void add(EFormat format, const std::string &data) override;
  bool open(Time time) const override;
  void close() const override;
  Time getTime() const override;
  bool has(EFormat format) const override;
  std::string get(EFormat format) const override;

private:
  //! Execute a command and return its output
  std::string executeCommand(const std::vector<const char*> &args) const;

  //! Execute a command with input data
  bool executeCommandWithInput(const std::vector<const char*> &args, const std::string &input) const;

  //! Check if a command exists
  static bool checkCommandExists(const char* command);

  //! Convert IClipboard format to MIME type
  std::string formatToMimeType(EFormat format) const;

  //! Convert MIME type to IClipboard format
  EFormat mimeTypeToFormat(const std::string &mimeType) const;

  //! Get available MIME types from clipboard
  std::vector<std::string> getAvailableMimeTypes() const;

  //! Get clipboard data for specific MIME type
  std::string getClipboardData(const std::string &mimeType) const;

  //! Set clipboard data for specific MIME type
  bool setClipboardData(const std::string &mimeType, const std::string &data) const;

  //! Monitor clipboard changes in background thread
  void monitorClipboard();

  //! Get current clipboard serial/timestamp
  Time getCurrentTime() const;

  //! Check if we own the clipboard
  bool isOwned() const;

  //! Update our ownership status
  void updateOwnership(bool owned);

  //! Invalidate cached clipboard data
  void invalidateCache();

private:
  ClipboardID m_id;
  mutable bool m_open = false;
  mutable Time m_time = 0;
  mutable Time m_cachedTime = 0;
  mutable bool m_owned = false;
  mutable std::atomic<bool> m_hasChanged = false;

  // Cached clipboard data
  mutable std::mutex m_cacheMutex;
  mutable bool m_cached = false;
  mutable std::string m_cachedData[kNumFormats];
  mutable bool m_cachedAvailable[kNumFormats];

  // Background monitoring
  std::unique_ptr<std::thread> m_monitorThread;
  std::atomic<bool> m_monitoring = false;
  std::atomic<bool> m_stopMonitoring = false;

  // Clipboard selection type (true = clipboard, false = primary)
  bool m_useClipboard;
};