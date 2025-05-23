/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2022 Red Hat, Inc.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "mt/Thread.h"
#include "platform/EiScreen.h"

#include <glib.h>
#include <libportal/portal.h>

#include <memory>

namespace deskflow {

class PortalRemoteDesktop
{
public:
  PortalRemoteDesktop(EiScreen *screen, IEventQueue *events);
  ~PortalRemoteDesktop();

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

private:
  EiScreen *m_screen;
  IEventQueue *m_events;

  Thread *m_glibThread;
  GMainLoop *m_glibMainLoop = nullptr;

  XdpPortal *m_portal = nullptr;
  XdpSession *m_session = nullptr;
  char *m_sessionRestoreToken = nullptr;

  guint m_sessionSignalId = 0;

  /// The number of successful sessions we've had already
  guint m_sessionIteration = 0;
};

} // namespace deskflow
