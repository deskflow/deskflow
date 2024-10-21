/*
 * Deskflow -- mouse and keyboard sharing utility
 * Copyright (C) 2022 Red Hat, Inc.
 * Copyright (C) 2024 Symless Ltd.
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
