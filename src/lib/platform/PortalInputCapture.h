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
#include <libportal/inputcapture.h>
#include <libportal/portal.h>
#include <memory>

namespace deskflow {

class PortalInputCapture
{
public:
  PortalInputCapture(EiScreen *screen, IEventQueue *events);
  ~PortalInputCapture();
  void enable();
  void disable();
  void release();
  void release(double x, double y);
  bool is_active() const
  {
    return is_active_;
  }

private:
  void glib_thread(void *);
  gboolean timeout_handler();
  gboolean init_input_capture_session();
  void cb_init_input_capture_session(GObject *object, GAsyncResult *res);
  void cb_set_pointer_barriers(GObject *object, GAsyncResult *res);
  void cb_session_closed(XdpSession *session);
  void cb_disabled(XdpInputCaptureSession *session);
  void cb_activated(XdpInputCaptureSession *session, std::uint32_t activation_id, GVariant *options);
  void cb_deactivated(XdpInputCaptureSession *session, std::uint32_t activation_id, GVariant *options);
  void cb_zones_changed(XdpInputCaptureSession *session, GVariant *options);

  /// g_signal_connect callback wrapper
  static void cb_session_closed_cb(XdpSession *session, gpointer data)
  {
    reinterpret_cast<PortalInputCapture *>(data)->cb_session_closed(session);
  }
  static void cb_disabled_cb(XdpInputCaptureSession *session, gpointer data)
  {
    reinterpret_cast<PortalInputCapture *>(data)->cb_disabled(session);
  }
  static void
  cb_activated_cb(XdpInputCaptureSession *session, std::uint32_t activation_id, GVariant *options, gpointer data)
  {
    reinterpret_cast<PortalInputCapture *>(data)->cb_activated(session, activation_id, options);
  }
  static void
  cb_deactivated_cb(XdpInputCaptureSession *session, std::uint32_t activation_id, GVariant *options, gpointer data)
  {
    reinterpret_cast<PortalInputCapture *>(data)->cb_deactivated(session, activation_id, options);
  }
  static void cb_zones_changed_cb(XdpInputCaptureSession *session, GVariant *options, gpointer data)
  {
    reinterpret_cast<PortalInputCapture *>(data)->cb_zones_changed(session, options);
  }

  int fake_eis_fd();

private:
  EiScreen *screen_ = nullptr;
  IEventQueue *events_ = nullptr;

  Thread *glib_thread_;
  GMainLoop *glib_main_loop_ = nullptr;

  XdpPortal *portal_ = nullptr;
  XdpInputCaptureSession *session_ = nullptr;

  std::vector<guint> signals_;

  bool enabled_ = false;
  bool is_active_ = false;
  std::uint32_t activation_id_ = 0;

  std::vector<XdpInputCapturePointerBarrier *> barriers_;
};

} // namespace deskflow
