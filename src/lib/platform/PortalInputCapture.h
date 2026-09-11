/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 - 2026 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2024, 2026 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2022, 2026 Red Hat, Inc.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "mt/Thread.h"
#include "platform/EiScreen.h"

#include <QByteArray>

#include <glib.h>
#include <libportal/inputcapture.h>
#include <libportal/portal.h>

#include <cstdint>
#include <map>
#include <utility>
#include <vector>

namespace deskflow {

class EiClipboard;

class PortalInputCapture
{
public:
  PortalInputCapture(EiScreen *screen, IEventQueue *events);
  ~PortalInputCapture();

  // Get the clipboard for the specified ID
  EiClipboard *getClipboard(ClipboardID id) const;
  void enable();
  void disable();
  void release();
  void release(double x, double y);
  void claimClipboard() const;
  bool isActive() const
  {
    return m_isActive;
  }

private:
  void glibThread(const void *);
  gboolean timeoutHandler() const;
  gboolean initSession();
  void setupSession(XdpInputCaptureSession *session);
  void handleStart(GObject *object, GAsyncResult *res);
  void handleInitSession(GObject *object, GAsyncResult *res);
  void handleSetPointerBarriers(const GObject *object, GAsyncResult *res);
  void handleSessionClosed(XdpSession *session);
  void handleDisabled(const XdpInputCaptureSession *session, const GVariant *option);
  void handleActivated(const XdpInputCaptureSession *session, const std::uint32_t activationId, GVariant *options);
  void
  handleDeactivated(const XdpInputCaptureSession *session, const std::uint32_t activationId, const GVariant *options);
  void handleZonesChanged(XdpInputCaptureSession *session, const GVariant *options);

  void handleSelectionTransfer(XdpSession *session, const char *mimeType, uint32_t serial) const;
  void readClipboardSelection(XdpSession *session) const;
  void claimClipboardOwnership(XdpSession *session) const;

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
  static void selectionTransfer(XdpSession *session, const char *mimeType, uint32_t serial, const gpointer data)
  {
    static_cast<PortalInputCapture *>(data)->handleSelectionTransfer(session, mimeType, serial);
  }

  enum class Signal : uint8_t
  {
    SessionClosed,
    Disabled,
    Activated,
    Deactivated,
    ZonesChanged,

    // Clipboard signals
    SelectionTransfer,
  };

  enum class BarrierSide : uint8_t
  {
    Left,
    Right,
    Top,
    Bottom
  };

  struct BarrierInfo
  {
    guint id = 0;
    BarrierSide side = BarrierSide::Left;
    gint x = 0;
    gint y = 0;
    guint width = 0;
    guint height = 0;
    gint x1 = 0;
    gint y1 = 0;
    gint x2 = 0;
    gint y2 = 0;
  };

  struct Bounds
  {
    gint left = 0;
    gint top = 0;
    gint right = 0;
    gint bottom = 0;
  };

  static const char *barrierSideName(BarrierSide side);
  static int
  scaleCoordinateBetweenRanges(double value, int sourceMin, int sourceMax, int destinationMin, int destinationMax);
  bool getPortalBounds(Bounds &bounds) const;
  bool getClosestReleaseBarrier(
      double x, double y, int screenLeft, int screenTop, int screenRight, int screenBottom, const Bounds &portalBounds,
      BarrierInfo &barrier
  ) const;
  std::pair<int, int> mapPortalActivationToScreenPosition(guint barrierId, double rawX, double rawY) const;
  std::pair<double, double> mapPortalReleasePosition(double x, double y) const;
  void addBarrier(guint id, BarrierSide side, gint zoneX, gint zoneY, guint zoneWidth, guint zoneHeight);

  EiScreen *m_screen = nullptr;
  IEventQueue *m_events = nullptr;
  int m_portalVersion = 0;

  Thread *m_glibThread;
  GMainLoop *m_glibMainLoop = nullptr;

  XdpPortal *m_portal = nullptr;
  XdpInputCaptureSession *m_session = nullptr;

  std::map<Signal, gulong> m_signals = {
      {Signal::SessionClosed, 0},     {Signal::Disabled, 0},     {Signal::Activated, 0},
      {Signal::Deactivated, 0},       {Signal::ZonesChanged, 0},

      {Signal::SelectionTransfer, 0},
  };

  bool m_enabled = false;
  bool m_isActive = false;
  mutable bool m_clipboardClaimPending = false;
  std::uint32_t m_activationId = 0;

  std::vector<XdpInputCapturePointerBarrier *> m_barriers;
  std::vector<BarrierInfo> m_barrierInfo;

  EiClipboard *m_clipboard = nullptr;
};

} // namespace deskflow
