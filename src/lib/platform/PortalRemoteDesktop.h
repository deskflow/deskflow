/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2022 Red Hat, Inc.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "mt/Thread.h"
#include "platform/EiScreen.h"

#include <glib.h>
#include <libportal/portal.h>

#include <QByteArray>

namespace deskflow {

class PortalRemoteDesktop
{
public:
  PortalRemoteDesktop(EiScreen *screen, IEventQueue *events);
  ~PortalRemoteDesktop();

  void claimClipboard() const;

private:
  void glibThread(const void *);
  gboolean timeoutHandler() const;
  gboolean initSession();
  void handleInitSession(GObject *object, GAsyncResult *res);
  void handleSessionStarted(GObject *object, GAsyncResult *res);
  void handleSessionClosed(XdpSession *session);
  void handleSelectionTransfer(XdpSession *session, const char *mimeType, uint32_t serial) const;
  void handleSelectionOwnerChanged(XdpSession *session, char **mimeTypes, gboolean isOwner) const;
  void reconnect(unsigned int timeout = 1000);

  static void handleSessionClosedCallback(XdpSession *session, gpointer data)
  {
    static_cast<PortalRemoteDesktop *>(data)->handleSessionClosed(session);
  }

  static void selectionTransferCallback(XdpSession *session, const char *mimeType, uint32_t serial, gpointer data)
  {
    static_cast<PortalRemoteDesktop *>(data)->handleSelectionTransfer(session, mimeType, serial);
  }

  static void selectionOwnerChangedCallback(XdpSession *session, char **mimeTypes, gboolean isOwner, gpointer data)
  {
    static_cast<PortalRemoteDesktop *>(data)->handleSelectionOwnerChanged(session, mimeTypes, isOwner);
  }

private:
  EiScreen *m_screen;
  IEventQueue *m_events;

  Thread *m_glibThread;
  GMainLoop *m_glibMainLoop = nullptr;

  XdpPortal *m_portal = nullptr;
  XdpSession *m_session = nullptr;
  char *m_sessionRestoreToken = nullptr;

  guint m_sessionSignalId = 0;
  guint m_selectionTransferSignalId = 0;
  guint m_selectionOwnerChangedSignalId = 0;

  /// The number of successful sessions we've had already
  guint m_sessionIteration = 0;
};

} // namespace deskflow
