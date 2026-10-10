/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 - 2026 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2024, 2026 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2022, 2026 Red Hat, Inc.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "mt/Thread.h"
#include "platform/EiComputer.h"

#include <QByteArray>

#include <glib.h>
#include <libportal/inputcapture.h>
#include <libportal/portal.h>

#include <cstdint>
#include <map>
#include <set>
#include <utility>
#include <vector>

namespace deskflow {

class EiClipboard;

class PortalInputCapture
{
public:
  PortalInputCapture(EiComputer *computer, IEventQueue *events);
  ~PortalInputCapture();

  // Get the clipboard for the specified ID
  EiClipboard *getClipboard(ClipboardID id) const;
  void enable();
  void disable();
  void release();
  void release(double x, double y);

  // Recompute and reapply pointer barriers against the computer's current
  // active sides. Barriers are otherwise only recomputed when the portal
  // itself reports a zone change (e.g. a monitor is plugged in), so this
  // must be called whenever the active sides mask may have changed for some
  // other reason, e.g. a new neighbor was linked to a shape hole.
  void refreshPointerBarriers();
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
      double x, double y, int computerLeft, int computerTop, int computerRight, int computerBottom,
      const Bounds &portalBounds, BarrierInfo &barrier
  ) const;
  std::pair<int, int> mapPortalActivationToComputerPosition(guint barrierId, double rawX, double rawY) const;
  std::pair<double, double> mapPortalReleasePosition(double x, double y) const;
  void addBarrier(guint id, BarrierSide side, gint zoneX, gint zoneY, guint zoneWidth, guint zoneHeight);

  EiComputer *m_computer = nullptr;
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
  std::uint32_t m_activationId = 0;

  // the barrier that triggered the most recent activation, if known -- used
  // to release the cursor relative to the edge actually crossed, rather
  // than guessing the nearest active side from raw distance to the
  // computer's outer bounding box (which is wrong whenever the barrier sits
  // on a shape hole's inner edge rather than that outer box)
  guint m_lastActivationBarrierId = 0;

  std::vector<XdpInputCapturePointerBarrier *> m_barriers;
  std::vector<BarrierInfo> m_barrierInfo;

  // the true union of all reported zones, computed once per handleZonesChanged()
  // call. getPortalBounds() returns this rather than re-aggregating from
  // m_barrierInfo, since a rejected barrier is pruned from that list -- using
  // it as the scaling range would then understate the real extent and distort
  // every activation's coordinate mapping, not just the rejected barrier's own.
  bool m_zoneUnionValid = false;
  gint m_zoneUnionLeft = 0;
  gint m_zoneUnionTop = 0;
  gint m_zoneUnionRight = 0;
  gint m_zoneUnionBottom = 0;

  // (hole location, side) pairs already warned about via warnIfSharedEdge(),
  // so a shared-edge conflict is reported once per session rather than every
  // time handleZonesChanged() happens to re-run (e.g. once per reconfigure())
  std::set<std::pair<deskflow::HoleLocation, BarrierSide>> m_warnedSharedEdges;

  EiClipboard *m_clipboard = nullptr;
};

} // namespace deskflow
