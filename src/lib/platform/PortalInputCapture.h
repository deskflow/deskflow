/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: 2025 Deskflow Developers
 * SPDX-FileCopyrightText: 2024 Symless Ltd.
 * SPDX-FileCopyrightText: 2022 Red Hat, Inc.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "mt/Thread.h"
#include "platform/EiScreen.h"
#include "platform/PortalClipboardProxy.h"
#include "platform/PortalSessionProxy.h"

#ifdef HAVE_LIBPORTAL_INPUTCAPTURE
#include <libportal/inputcapture.h>
#endif
#include <libportal/portal.h>
#include <memory>

class IClipboard;

namespace deskflow {

class PortalClipboardProxy;
class PortalClipboard;

#ifdef HAVE_LIBPORTAL_INPUTCAPTURE
class PortalInputCapture
{
public:
  PortalInputCapture(EiScreen *screen, IEventQueue *events);
  ~PortalInputCapture();
  void enable();
  void disable();
  void release();
  void release(double x, double y);
  bool isActive() const
  {
    return m_isActive;
  }
  IClipboard *getClipboard() const;

Q_SIGNALS:
  void clipboardChanged();

private:
  void glibThread(const void *);
  gboolean timeoutHandler() const;
  gboolean initSession();
  void handleInitSession(GObject *object, GAsyncResult *res);
  void handleSetPointerBarriers(const GObject *object, GAsyncResult *res);
  void handleSessionClosed(XdpSession *session);
  void handleDisabled(const XdpInputCaptureSession *session, const GVariant *option);
  void handleActivated(const XdpInputCaptureSession *session, const std::uint32_t activationId, GVariant *options);
  void handleDeactivated(const XdpInputCaptureSession *session, const std::uint32_t activationId, GVariant *options);
  void handleReleased(const XdpInputCaptureSession *session, const std::uint32_t activationId, GVariant *options);
  void
  handlePointerBarrierFired(const XdpInputCaptureSession *session, const std::uint32_t activationId, GVariant *options);
  void handleZonesChanged(XdpInputCaptureSession *session, const GVariant *options);
  void onSessionCreated(const QDBusObjectPath &handle);
  void onSessionStarted(const QString &restoreToken);
  void initClipboard(const QDBusObjectPath &handle);

  /// g_signal_connect callback wrapper
  static void sessionClosed(XdpSession *session, const gpointer data)
  {
    static_cast<PortalInputCapture *>(data)->handleSessionClosed(session);
  }
  static void disabled(const XdpInputCaptureSession *session, const GVariant *options, const gpointer data)
  {
    static_cast<PortalInputCapture *>(data)->handleDisabled(session, options);
  }
  static void activated(
      const XdpInputCaptureSession *session, const std::uint32_t activationId, GVariant *options, const gpointer data
  )
  {
    static_cast<PortalInputCapture *>(data)->handleActivated(session, activationId, options);
  }
  static void deactivated(
      const XdpInputCaptureSession *session, const std::uint32_t activationId, const GVariant *options,
      const gpointer data
  )
  {
    static_cast<PortalInputCapture *>(data)->handleDeactivated(session, activationId, options);
  }
  static void zonesChanged(XdpInputCaptureSession *session, const GVariant *options, const gpointer data)
  {
    static_cast<PortalInputCapture *>(data)->handleZonesChanged(session, options);
  }

private:
  enum class Signal : uint8_t
  {
    SessionClosed,
    Disabled,
    Activated,
    Deactivated,
    ZonesChanged
  };

  EiScreen *m_screen = nullptr;
  IEventQueue *m_events = nullptr;

  Thread *m_glibThread;
  GMainLoop *m_glibMainLoop = nullptr;

  XdpPortal *m_portal = nullptr;
  XdpInputCaptureSession *m_session = nullptr;

  std::map<Signal, gulong> m_signals = {
      {Signal::SessionClosed, 0},
      {Signal::Disabled, 0},
      {Signal::Activated, 0},
      {Signal::Deactivated, 0},
      {Signal::ZonesChanged, 0}
  };

  bool m_enabled = false;
  bool m_isActive = false;
  std::uint32_t m_activationId = 0;

  std::vector<XdpInputCapturePointerBarrier *> m_barriers;

  // Session and Clipboard support
  std::unique_ptr<PortalSessionProxy> m_sessionProxy;
  std::unique_ptr<PortalClipboardProxy> m_clipboardProxy;
  std::unique_ptr<PortalClipboard> m_clipboardStandard;
  std::unique_ptr<PortalClipboard> m_clipboardPrimary;
};
#endif

} // namespace deskflow
