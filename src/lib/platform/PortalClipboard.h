/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "deskflow/ClipboardTypes.h"
#include "deskflow/IClipboard.h"

#include <QObject>

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

// XDG Desktop Portal headers
#include <glib.h>
#include <libportal/portal.h>

//!
//! XDG Desktop Portal clipboard implementation
/*!
This class implements clipboard functionality for Wayland and X11 environments
using the XDG Desktop Portal (org.freedesktop.portal.Clipboard).
This provides clipboard support through the portal API, enabling clipboard
sharing in sandboxed environments and improved Wayland support.
*/
class PortalClipboard : public QObject, public IClipboard
{
  Q_OBJECT
public:
  explicit PortalClipboard(ClipboardID id);
  PortalClipboard(PortalClipboard const &) = delete;
  PortalClipboard(PortalClipboard &&) = delete;
  ~PortalClipboard() override;

  PortalClipboard &operator=(PortalClipboard const &) = delete;
  PortalClipboard &operator=(PortalClipboard &&) = delete;

  //! Get clipboard ID
  ClipboardID getID() const;

  //! Check if XDG Desktop Portal clipboard is available
  static bool isAvailable();

  //! Check if PortalClipboard is enabled
  static bool isEnabled();

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
  void add(Format format, const std::string &data) override;
  bool open(Time time) const override;
  void close() const override;
  Time getTime() const override;
  bool has(Format format) const override;
  std::string get(Format format) const override;

private:
  //! Initialize the portal connection
  bool initPortal();

  //! Convert IClipboard format to MIME type
  QString formatToMimeType(Format format) const;

  //! Convert MIME type to IClipboard format
  Format mimeTypeToFormat(const QString &mimeType) const;

  //! Read clipboard content from portal
  bool readFromPortal(const QString &mimeType, std::string &data) const;

  //! Write clipboard content to portal
  bool writeToPortal(const QString &mimeType, const std::vector<char> &data) const;

  //! Monitor clipboard changes via portal
  void monitorClipboard();

  //! Get current clipboard serial/timestamp
  Time getCurrentTime() const;

private:
  ClipboardID m_id;
  mutable bool m_open = false;
  mutable Time m_time = 0;
  mutable Time m_cachedTime = 0;

  mutable std::atomic<bool> m_hasChanged = false;

  // Cached clipboard data
  mutable std::mutex m_cacheMutex;
  mutable bool m_cached = false;
  mutable std::string m_cachedData[static_cast<int>(Format::TotalFormats)];
  mutable bool m_cachedAvailable[static_cast<int>(Format::TotalFormats)];

  // Background monitoring
  std::unique_ptr<std::thread> m_monitorThread;
  std::atomic<bool> m_monitoring = false;
  std::atomic<bool> m_stopMonitoring = false;

  // Portal connection
  XdpPortal *m_portal = nullptr;

  // Clipboard selection type (true = clipboard, false = primary)
  bool m_useClipboard;

  // Parent window for portal
  guint32 m_parentWindow = 0;
};
