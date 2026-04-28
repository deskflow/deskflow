/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2022 Red Hat, Inc.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "deskflow/Clipboard.h"
#include "deskflow/ClipboardTypes.h"
#include "mt/Thread.h"
#include "platform/EiScreen.h"

#include <glib.h>
#include <gio/gio.h>
#include <libportal/portal.h>

#include <atomic>
#include <mutex>
#include <string>
#include <vector>

namespace deskflow {

class PortalRemoteDesktop
{
public:
  /// @param clipboardOnly If true, skip EIS connection (used on primary screen
  ///        where InputCapture handles input but we need RemoteDesktop for clipboard)
  PortalRemoteDesktop(EiScreen *screen, IEventQueue *events, bool clipboardOnly = false);
  ~PortalRemoteDesktop();

  //! Clipboard support via org.freedesktop.portal.Clipboard
  bool getClipboard(ClipboardID id, IClipboard *clipboard) const;
  bool setClipboard(ClipboardID id, const IClipboard *clipboard);

  bool hasClipboardSupport() const { return m_clipboardEnabled; }

private:
  void glibThread(const void *);
  gboolean timeoutHandler() const;
  gboolean initSession();
  void handleInitSession(GObject *object, GAsyncResult *res);
  void handleSessionStarted(GObject *object, GAsyncResult *res);
  void handleSessionClosed(XdpSession *session);
  void reconnect(unsigned int timeout = 1000);

  /// g_signal_connect callback wrapper
  static void handleSessionClosedCallback(XdpSession *session, gpointer data)
  {
    static_cast<PortalRemoteDesktop *>(data)->handleSessionClosed(session);
  }

  // Clipboard via XDG Portal DBus
  void requestClipboard();
  void cleanupClipboard();

  void onSelectionOwnerChanged(GDBusConnection *connection, const gchar *sender_name, const gchar *object_path,
                               const gchar *interface_name, const gchar *signal_name, GVariant *parameters);
  void onSelectionTransfer(GDBusConnection *connection, const gchar *sender_name, const gchar *object_path,
                           const gchar *interface_name, const gchar *signal_name, GVariant *parameters);

  static void cbSelectionOwnerChanged(GDBusConnection *connection, const gchar *sender_name, const gchar *object_path,
                                      const gchar *interface_name, const gchar *signal_name, GVariant *parameters,
                                      gpointer user_data)
  {
    static_cast<PortalRemoteDesktop *>(user_data)
        ->onSelectionOwnerChanged(connection, sender_name, object_path, interface_name, signal_name, parameters);
  }

  static void cbSelectionTransfer(GDBusConnection *connection, const gchar *sender_name, const gchar *object_path,
                                  const gchar *interface_name, const gchar *signal_name, GVariant *parameters,
                                  gpointer user_data)
  {
    static_cast<PortalRemoteDesktop *>(user_data)
        ->onSelectionTransfer(connection, sender_name, object_path, interface_name, signal_name, parameters);
  }

  std::string readMimeTypeFromPortal(const std::string &mime_type) const;

private:
  EiScreen *m_screen;
  IEventQueue *m_events;

  Thread *m_glibThread;
  GMainLoop *m_glibMainLoop = nullptr;

  XdpPortal *m_portal = nullptr;
  XdpSession *m_session = nullptr;
  char *m_sessionRestoreToken = nullptr;

  guint m_sessionSignalId = 0;

  //! The number of successful sessions we've had already
  guint m_sessionIteration = 0;

  bool m_clipboardOnly = false;

  // Clipboard state
  GDBusConnection *m_dbusConnection = nullptr;
  std::string m_sessionHandle;
  guint m_selectionOwnerChangedSub = 0;
  guint m_selectionTransferSub = 0;
  mutable std::mutex m_clipboardMutex;
  mutable Clipboard m_storedClipboard;
  mutable std::vector<std::string> m_availableMimeTypes;
  std::atomic<bool> m_clipboardEnabled{false};
  bool m_weOwnClipboard = false;
};

} // namespace deskflow