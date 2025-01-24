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

namespace deskflow {

class PortalRemoteDesktop
{
public:
  PortalRemoteDesktop(EiScreen *screen, IEventQueue *events);
  ~PortalRemoteDesktop();

private:
  void glib_thread(void *);
  gboolean timeout_handler();
  gboolean init_remote_desktop_session();
  void cb_init_remote_desktop_session(GObject *object, GAsyncResult *res);
  void cb_session_started(GObject *object, GAsyncResult *res);
  void cb_session_closed(XdpSession *session);
  void reconnect(unsigned int timeout = 1000);

  /// g_signal_connect callback wrapper
  static void cb_session_closed_cb(XdpSession *session, gpointer data)
  {
    reinterpret_cast<PortalRemoteDesktop *>(data)->cb_session_closed(session);
  }

  int fake_eis_fd();

private:
  EiScreen *screen_;
  IEventQueue *events_;

  Thread *glib_thread_;
  GMainLoop *glib_main_loop_ = nullptr;

  XdpPortal *portal_ = nullptr;
  XdpSession *session_ = nullptr;
  char *session_restore_token_ = nullptr;

  guint session_signal_id_ = 0;

  /// The number of successful sessions we've had already
  guint session_iteration_ = 0;
};

} // namespace deskflow
