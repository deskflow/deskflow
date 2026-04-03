/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "deskflow/ClipboardTypes.h"
#include "deskflow/IClipboard.h"

#include <atomic>
#include <mutex>
#include <string>

#include <QObject>
#include <QString>

#include <glib.h>
#include <libportal/portal.h>

//! Wayland portal clipboard implementation using XDG Desktop Portal
/*!
This class implements clipboard functionality for Wayland environments
by using the XDG Desktop Portal clipboard API.

Current Status:
- The clipboard portal API is defined in libportal 0.9.1+
- xdg-desktop-portal backends (Mutter, KWin) don't implement clipboard yet
- This implementation is ready for when portal clipboard becomes available
- Falls back gracefully when portal clipboard is unavailable

See: https://github.com/deskflow/deskflow/issues/8031
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

  //! Check if portal clipboard is available
  static bool isAvailable();

  //! Check if portal clipboard is enabled
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
  //! Convert IClipboard format to MIME type
  QString formatToMimeType(Format format) const;

  //! Convert MIME type to IClipboard format
  Format mimeTypeToFormat(const QString &mimeType) const;

  //! Request clipboard access from the portal
  void requestClipboardAccess();

  //! Handle clipboard access response
  void handleAccessResponse(GObject *object, GAsyncResult *res);

  //! Handle selection owner changed signal
  void handleSelectionOwnerChanged(const char **mimeTypes);

  //! Get current timestamp
  Time getCurrentTime() const;

  //! Invalidate cached clipboard data
  void invalidateCache();

  //! Static callback for access response
  static void onAccessResponse(GObject *object, GAsyncResult *res, gpointer data);

  //! Static callback for selection owner changed
  static void onSelectionOwnerChanged(XdpPortal *portal, const char **mimeTypes, gpointer data);

private:
  ClipboardID m_id;
  mutable bool m_open = false;
  mutable Time m_time = 0;
  mutable Time m_cachedTime = 0;
  mutable bool m_owned = false;
  mutable std::atomic<bool> m_hasChanged{false};

  // Portal connection
  XdpPortal *m_portal = nullptr;
  bool m_hasAccess = false;
  bool m_accessRequested = false;

  // Cached clipboard data
  mutable std::mutex m_cacheMutex;
  mutable bool m_cached = false;
  mutable std::string m_cachedData[static_cast<int>(Format::TotalFormats)];
  mutable bool m_cachedAvailable[static_cast<int>(Format::TotalFormats)];

  // Clipboard selection type (true = clipboard, false = primary)
  bool m_useClipboard;

  // Signal handler ID for selection changed
  gulong m_signalId = 0;
};
