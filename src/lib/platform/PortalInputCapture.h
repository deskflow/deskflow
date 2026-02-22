/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2022 Red Hat, Inc.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "deskflow/ClipboardTypes.h"
#include "mt/Thread.h"
#include "platform/EiScreen.h"

#include <glib.h>
#include <libportal/portal.h>
#include <libportal/clipboard.h>
#include <libportal/inputcapture.h>

namespace deskflow {

class EiClipboard;

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

  //! Returns the in-memory clipboard for the given ID, or nullptr.
  EiClipboard *getClipboard(ClipboardID id) const;

  //! Notify the portal that our local clipboard has changed.
  //! Called by EiScreen::setClipboard() after data is stored in EiClipboard.
  void notifySelectionChanged();

private:
  void glibThread(const void *);
  gboolean timeoutHandler() const;
  gboolean initSession();
  void handleInitSession(GObject *object, GAsyncResult *res);
  void handleSetPointerBarriers(const GObject *object, GAsyncResult *res);
  void handleSessionClosed(XdpSession *session);
  void handleDisabled(const XdpInputCaptureSession *session, const GVariant *option);
  void handleActivated(const XdpInputCaptureSession *session, const std::uint32_t activationId, GVariant *options);
  void
  handleDeactivated(const XdpInputCaptureSession *session, const std::uint32_t activationId, const GVariant *options);
  void handleZonesChanged(XdpInputCaptureSession *session, const GVariant *options);

#ifdef HAVE_LIBPORTAL_INPUTCAPTURE_CLIPBOARD
  void handleSelectionOwnerChanged(XdpSession *session, GStrv mimeTypes, gboolean sessionIsOwner);
  void handleSelectionTransfer(XdpSession *session, const char *mimeType, guint32 serial);
#endif

  /// g_signal_connect callback wrappers
  static void sessionClosed(XdpSession *session, const gpointer data)
  {
    static_cast<PortalInputCapture *>(data)->handleSessionClosed(session);
  }
  static void disabled(const XdpInputCaptureSession *session, const GVariant *options, const gpointer data)
  {
    static_cast<PortalInputCapture *>(data)->handleDisabled(session, options);
  }
  static void
  activated(const XdpInputCaptureSession *session, const std::uint32_t activationId, GVariant *options, const gpointer data)
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

#ifdef HAVE_LIBPORTAL_INPUTCAPTURE_CLIPBOARD
  static void selectionOwnerChanged(XdpSession *session, GStrv mimeTypes, gboolean sessionIsOwner, gpointer data)
  {
    static_cast<PortalInputCapture *>(data)->handleSelectionOwnerChanged(session, mimeTypes, sessionIsOwner);
  }
  static void selectionTransfer(XdpSession *session, const char *mimeType, guint32 serial, gpointer data)
  {
    static_cast<PortalInputCapture *>(data)->handleSelectionTransfer(session, mimeType, serial);
  }
#endif

private:
  enum class Signal : uint8_t
  {
    SessionClosed,
    Disabled,
    Activated,
    Deactivated,
    ZonesChanged,
    SelectionOwnerChanged,
    SelectionTransfer,
  };

  EiScreen *m_screen = nullptr;
  IEventQueue *m_events = nullptr;

  Thread *m_glibThread = nullptr;
  GMainLoop *m_glibMainLoop = nullptr;

  XdpPortal *m_portal = nullptr;
  XdpInputCaptureSession *m_session = nullptr;

  std::map<Signal, gulong> m_signals = {
      {Signal::SessionClosed, 0},         {Signal::Disabled, 0},
      {Signal::Activated, 0},             {Signal::Deactivated, 0},
      {Signal::ZonesChanged, 0},          {Signal::SelectionOwnerChanged, 0},
      {Signal::SelectionTransfer, 0},
  };

  bool m_enabled = false;
  bool m_isActive = false;
  std::uint32_t m_activationId = 0;

  std::vector<XdpInputCapturePointerBarrier *> m_barriers;

  EiClipboard *m_clipboard = nullptr;
};

} // namespace deskflow
