/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2022 Peter Hutterer, Olivier Fourdan
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

#include "config.h"

#include "mt/Thread.h"
#include "platform/EiScreen.h"

#include <glib.h>
#include <libportal/inputcapture.h>
#include <libportal/portal.h>
#include <memory>

namespace synergy {

class PortalInputCapture {
public:
  PortalInputCapture(EiScreen *screen, IEventQueue *events);
  ~PortalInputCapture();
  void enable();
  void disable();
  void release();
  void release(double x, double y);
  bool isActive() const { return isActive_; }

private:
  void glibThread();
  gboolean timeoutHandler();
  gboolean initInputCaptureSession();
  void cbInitInputCaptureSession(GObject *object, GAsyncResult *res);
  void cbSetPointerBarriers(GObject *object, GAsyncResult *res);
  void cbSessionClosed(XdpSession *session);
  void cbDisabled(XdpInputCaptureSession *session);
  void cbActivated(
      XdpInputCaptureSession *session, std::uint32_t activation_id,
      GVariant *options);
  void cbDeactivated(
      XdpInputCaptureSession *session, std::uint32_t activation_id,
      GVariant *options);
  void cb_zones_changed(XdpInputCaptureSession *session, GVariant *options);

  /// g_signal_connect callback wrapper
  static void cbSessionClosedCb(XdpSession *session, gpointer data) {
    reinterpret_cast<PortalInputCapture *>(data)->cbSessionClosed(session);
  }
  static void cbDisabledCb(XdpInputCaptureSession *session, gpointer data) {
    reinterpret_cast<PortalInputCapture *>(data)->cbDisabled(session);
  }
  static void cbActivatedCb(
      XdpInputCaptureSession *session, std::uint32_t activation_id,
      GVariant *options, gpointer data) {
    reinterpret_cast<PortalInputCapture *>(data)->cbActivated(
        session, activation_id, options);
  }
  static void cbDeactivatedCb(
      XdpInputCaptureSession *session, std::uint32_t activation_id,
      GVariant *options, gpointer data) {
    reinterpret_cast<PortalInputCapture *>(data)->cbDeactivated(
        session, activation_id, options);
  }
  static void cbZonesChangedCb(
      XdpInputCaptureSession *session, GVariant *options, gpointer data) {
    reinterpret_cast<PortalInputCapture *>(data)->cb_zones_changed(
        session, options);
  }

  int fakeEisFd();

private:
  EiScreen *screen_ = nullptr;
  IEventQueue *events_ = nullptr;

  std::unique_ptr<Thread> glibThread_;
  GMainLoop *glib_main_loop_ = nullptr;

  XdpPortal *portal_ = nullptr;
  XdpInputCaptureSession *session_ = nullptr;

  std::vector<guint> signals_;

  bool enabled_ = false;
  bool isActive_ = false;
  std::uint32_t activation_id_ = 0;

  std::vector<XdpInputCapturePointerBarrier *> barriers_;
};

} // namespace synergy
