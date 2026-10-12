/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 - 2026 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2024, 2026 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2022, 2026 Red Hat, Inc.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/PortalInputCapture.h"
#include "base/DirectionTypes.h"
#include "base/Event.h"
#include "base/Log.h"
#include "base/TMethodJob.h"
#include "common/Settings.h"
#include "deskflow/ClipboardTypes.h"
#include "platform/EiClipboard.h"

#ifdef HAVE_LIBPORTAL_CLIPBOARD
#include "platform/PortalClipboard.h"
#endif

#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>
#include <poll.h>
#include <sys/socket.h> // for EIS fd hack, remove
#include <sys/un.h>     // for EIS fd hack, remove

#include <QBuffer>
#include <QByteArray>
#include <QByteArrayList>
#include <QDataStream>
#include <QFile>
#include <QImage>
#include <QtEndian>

namespace deskflow {

const char *PortalInputCapture::barrierSideName(BarrierSide side)
{
  using enum BarrierSide;

  switch (side) {
  case Left:
    return "left";
  case Right:
    return "right";
  case Top:
    return "top";
  case Bottom:
    return "bottom";
  }

  return "unknown";
}

int PortalInputCapture::scaleCoordinateBetweenRanges(
    double value, int sourceMin, int sourceMax, int destinationMin, int destinationMax
)
{
  if (sourceMax <= sourceMin || destinationMax <= destinationMin) {
    return destinationMin;
  }

  const auto clamped = std::clamp(value, static_cast<double>(sourceMin), static_cast<double>(sourceMax));
  const auto fraction = (clamped - sourceMin) / (sourceMax - sourceMin);
  const auto mapped = static_cast<int>(std::lround(destinationMin + fraction * (destinationMax - destinationMin)));
  return std::clamp(mapped, destinationMin, destinationMax);
}

bool PortalInputCapture::getPortalBounds(Bounds &bounds) const
{
  if (!m_zoneUnionValid) {
    return false;
  }

  bounds.left = m_zoneUnionLeft;
  bounds.top = m_zoneUnionTop;
  bounds.right = m_zoneUnionRight - 1;
  bounds.bottom = m_zoneUnionBottom - 1;

  return true;
}

bool PortalInputCapture::getClosestReleaseBarrier(
    double x, double y, int computerLeft, int computerTop, int computerRight, int computerBottom,
    const Bounds &portalBounds, BarrierInfo &barrier
) const
{
  // prefer the barrier that was actually crossed to get here, if it's still
  // in the current barrier set -- this is always correct, whereas the
  // distance-based fallback below assumes every active side's barrier sits
  // on the computer's outer bounding box, which isn't true for one placed
  // against a shape hole's inner edge (it can end up numerically closer to
  // a completely different side's outer edge than to the one actually
  // crossed)
  if (m_lastActivationBarrierId != 0) {
    const auto it = std::ranges::find_if(m_barrierInfo, [this](const auto &info) {
      return info.id == m_lastActivationBarrierId;
    });
    if (it != m_barrierInfo.end()) {
      barrier = *it;
      return true;
    }
  }

  const auto activeSides = m_computer->activeSides();
  using enum DirectionMask;

  auto side = BarrierSide::Left;
  auto sideDistance = std::numeric_limits<double>::max();
  const auto considerSide = [&side, &sideDistance](BarrierSide candidateSide, double candidateDistance) {
    if (candidateDistance < sideDistance) {
      side = candidateSide;
      sideDistance = candidateDistance;
    }
  };

  if (activeSides & static_cast<int>(LeftMask)) {
    considerSide(BarrierSide::Left, std::abs(x - computerLeft));
  }
  if (activeSides & static_cast<int>(RightMask)) {
    considerSide(BarrierSide::Right, std::abs(x - computerRight));
  }
  if (activeSides & static_cast<int>(TopMask)) {
    considerSide(BarrierSide::Top, std::abs(y - computerTop));
  }
  if (activeSides & static_cast<int>(BottomMask)) {
    considerSide(BarrierSide::Bottom, std::abs(y - computerBottom));
  }
  if (sideDistance == std::numeric_limits<double>::max()) {
    return false;
  }

  const auto portalX =
      scaleCoordinateBetweenRanges(x, computerLeft, computerRight, portalBounds.left, portalBounds.right);
  const auto portalY =
      scaleCoordinateBetweenRanges(y, computerTop, computerBottom, portalBounds.top, portalBounds.bottom);

  auto bestDistance = std::numeric_limits<int>::max();
  for (const auto &info : m_barrierInfo) {
    if (info.side != side) {
      continue;
    }

    const auto left = info.x;
    const auto top = info.y;
    const auto right = info.x + static_cast<gint>(info.width) - 1;
    const auto bottom = info.y + static_cast<gint>(info.height) - 1;
    auto distance = 0;

    switch (side) {
    case BarrierSide::Left:
    case BarrierSide::Right:
      if (portalY < top) {
        distance = top - portalY;
      } else if (portalY > bottom) {
        distance = portalY - bottom;
      }
      break;

    case BarrierSide::Top:
    case BarrierSide::Bottom:
      if (portalX < left) {
        distance = left - portalX;
      } else if (portalX > right) {
        distance = portalX - right;
      }
      break;
    }

    if (distance < bestDistance) {
      bestDistance = distance;
      barrier = info;
    }
  }

  return bestDistance != std::numeric_limits<int>::max();
}

PortalInputCapture::PortalInputCapture(EiComputer *computer, IEventQueue *events)
    : m_computer{computer},
      m_events{events},
      m_portalVersion(0),
      m_portal{xdp_portal_new()}
{
  // Create clipboard for primary clipboard ID
  m_clipboard = new EiClipboard(kClipboardClipboard);

  m_glibMainLoop = g_main_loop_new(nullptr, true);

  auto tMethodJob = new TMethodJob<PortalInputCapture>(this, &PortalInputCapture::glibThread);
  m_glibThread = new Thread(tMethodJob);

  auto captureCallback = [](gpointer data) { return static_cast<PortalInputCapture *>(data)->initSession(); };

  g_idle_add(captureCallback, this);
}

PortalInputCapture::~PortalInputCapture()
{
  if (g_main_loop_is_running(m_glibMainLoop))
    g_main_loop_quit(m_glibMainLoop);

  if (m_glibThread) {
    m_glibThread->cancel();
    m_glibThread->wait();
    m_glibThread = nullptr;

    g_main_loop_unref(m_glibMainLoop);
    m_glibMainLoop = nullptr;
  }

  if (m_session) {
    using enum Signal;
    XdpSession *parentSession = xdp_input_capture_session_get_session(m_session);
    if (m_signals.at(SessionClosed) != 0)
      g_signal_handler_disconnect(parentSession, m_signals.at(SessionClosed));
    g_signal_handler_disconnect(m_session, m_signals.at(Disabled));
    g_signal_handler_disconnect(m_session, m_signals.at(Activated));
    g_signal_handler_disconnect(m_session, m_signals.at(Deactivated));
    g_signal_handler_disconnect(m_session, m_signals.at(ZonesChanged));
#ifdef HAVE_LIBPORTAL_CLIPBOARD
    g_signal_handler_disconnect(parentSession, m_signals.at(SelectionTransfer));
#endif
    g_object_unref(m_session);
  }

  for (auto b : m_barriers) {
    g_object_unref(b);
  }
  m_barriers.clear();
  g_object_unref(m_portal);

  delete m_clipboard;
}

EiClipboard *PortalInputCapture::getClipboard(ClipboardID id) const
{
  // Currently only supporting primary clipboard
  if (id == kClipboardClipboard) {
    return m_clipboard;
  }
  return nullptr;
}

gboolean PortalInputCapture::timeoutHandler() const
{
  return true; // keep re-triggering
}

void PortalInputCapture::handleSessionClosed(XdpSession *session)
{
  LOG_ERR("portal input capture session was closed, exiting");
  g_main_loop_quit(m_glibMainLoop);
  m_events->addEvent(Event(EventTypes::Quit));

  g_signal_handler_disconnect(session, m_signals.at(Signal::SessionClosed));
  m_signals.at(Signal::SessionClosed) = 0;
}

void PortalInputCapture::claimClipboardOwnership([[maybe_unused]] XdpSession *session) const
{
#ifdef HAVE_LIBPORTAL_CLIPBOARD
  PortalClipboard::claimOwnership(m_clipboard, session);
#endif
}

void PortalInputCapture::readClipboardSelection(XdpSession *session) const
{
#ifdef HAVE_LIBPORTAL_CLIPBOARD
  const qint64 maxBytes = static_cast<qint64>(m_computer->maximumClipboardSize()) * 1024;
  LOG_DEBUG("clipboard read cap: %lld bytes", static_cast<long long>(maxBytes));

  const char **mimeTypes = xdp_session_get_selection_mime_types(session);
  if (!mimeTypes) {
    LOG_DEBUG("clipboard has no mime types available to read");
    return;
  }

  if (PortalClipboard::readSelectionIntoCache(m_clipboard, session, mimeTypes, maxBytes))
    m_computer->sendClipboardEvent(EventTypes::ClipboardGrabbed, kClipboardClipboard);
#else
  (void)session;
#endif
}

void PortalInputCapture::handleSelectionTransfer(XdpSession *session, const char *mimeType, uint32_t serial) const
{
#ifdef HAVE_LIBPORTAL_CLIPBOARD
  if (m_isActive) {
    LOG_DEBUG("skipping clipboard selection transfer, clipboard is active");
    return;
  }
  PortalClipboard::serveSelectionTransfer(m_clipboard, session, mimeType, serial);
#else
  (void)session;
  (void)mimeType;
  (void)serial;
#endif
}

void PortalInputCapture::setupSession(XdpInputCaptureSession *session)
{
  g_autoptr(GError) error = nullptr;
  XdpSession *parentSession = xdp_input_capture_session_get_session(session);

#ifdef HAVE_LIBPORTAL_CLIPBOARD
  if (!xdp_session_is_clipboard_enabled(parentSession) && m_portalVersion > 1) {
    if (Settings::value(Settings::Server::XdpClipboardRetried).toBool()) {
      // some backends never report clipboard enabled even when granted; don't loop forever
      LOG_DEBUG("clipboard still not enabled on session after one retry, continuing without it");
    } else {
      LOG_WARN("clipboard not enabled on session, discarding restore token to force a fresh session");
#ifdef HAVE_LIBPORTAL_INPUTCAPTURE_RESTORE
      Settings::setValue(Settings::Server::XdpRestoreToken, QString());
#endif
      Settings::setValue(Settings::Server::XdpClipboardRetried, true);
      g_object_unref(m_session);
      m_session = nullptr;
      g_idle_add([](gpointer data) { return static_cast<PortalInputCapture *>(data)->initSession(); }, this);
      return;
    }
  }
#endif

  auto fd = xdp_input_capture_session_connect_to_eis(session, &error);
  if (fd < 0) {
    LOG_ERR("failed to connect to eis: %s", error->message);
    g_main_loop_quit(m_glibMainLoop);
    m_events->addEvent(Event(EventTypes::Quit));
    return;
  }

  // Socket ownership is transferred to the EiComputer
  m_events->addEvent(Event(EventTypes::EIConnected, m_computer->getEventTarget(), EiComputer::EiConnectInfo::alloc(fd))
  );

  using enum Signal;
  m_signals.at(Disabled) = g_signal_connect(G_OBJECT(session), "disabled", G_CALLBACK(disabled), this);
  m_signals.at(Activated) = g_signal_connect(G_OBJECT(session), "activated", G_CALLBACK(activated), this);
  m_signals.at(Deactivated) = g_signal_connect(G_OBJECT(session), "deactivated", G_CALLBACK(deactivated), this);
  m_signals.at(ZonesChanged) = g_signal_connect(G_OBJECT(session), "zones-changed", G_CALLBACK(zonesChanged), this);
  m_signals.at(SessionClosed) = g_signal_connect(G_OBJECT(parentSession), "closed", G_CALLBACK(sessionClosed), this);
  handleZonesChanged(session, nullptr);

#ifdef HAVE_LIBPORTAL_CLIPBOARD
  m_signals.at(SelectionTransfer) =
      g_signal_connect(G_OBJECT(parentSession), "selection-transfer", G_CALLBACK(selectionTransfer), this);
#endif
}

void PortalInputCapture::handleInitSession(GObject *object, GAsyncResult *res)
{
  g_autoptr(GError) error = nullptr;

  LOG_DEBUG("portal input capture session initialized");

  auto session = xdp_portal_create_input_capture_session_finish(XDP_PORTAL(object), res, &error);
  if (!session) {
    LOG_ERR("failed to initialize input capture session, quitting: %s", error->message);
    g_main_loop_quit(m_glibMainLoop);
    m_events->addEvent(Event(EventTypes::Quit));
    return;
  }

  m_session = session;

  setupSession(session);
}

void PortalInputCapture::handleStart(GObject *object, GAsyncResult *res)
{
  g_autoptr(GError) error = nullptr;

#ifdef HAVE_LIBPORTAL_INPUTCAPTURE_RESTORE
  LOG_DEBUG("portal input capture session initialized");
  if (!xdp_input_capture_session_start_finish(m_session, res, &error)) {
    LOG_ERR("failed to start input capture session, quitting: %s", error->message);
    g_main_loop_quit(m_glibMainLoop);
    m_events->addEvent(Event(EventTypes::Quit));
    return;
  }

  auto restoreToken = QString(xdp_input_capture_session_get_restore_token(m_session));
  if (!restoreToken.isEmpty()) {
    Settings::setValue(Settings::Server::XdpRestoreToken, restoreToken);
  }
#endif
  setupSession(m_session);
}

void PortalInputCapture::handleSetPointerBarriers(const GObject *, GAsyncResult *res)
{
  g_autoptr(GError) error = nullptr;

  auto failed_list = xdp_input_capture_session_set_pointer_barriers_finish(m_session, res, &error);
  if (failed_list) {
    auto it = failed_list;
    while (it) {
      guint id;
      g_object_get(it->data, "id", &id, nullptr);
      for (auto elem = m_barriers.begin(); elem != m_barriers.end(); elem++) {
        if (*elem == it->data) {
          int x1;
          int x2;
          int y1;
          int y2;

          g_object_get(G_OBJECT(*elem), "x1", &x1, "x2", &x2, "y1", &y1, "y2", &y2, nullptr);

          LOG_WARN("failed to apply barrier %d (%d/%d-%d/%d)", id, x1, y1, x2, y2);
          g_object_unref(*elem);
          m_barriers.erase(elem);
          std::erase_if(m_barrierInfo, [id](const auto &info) { return info.id == id; });
          break;
        }
      }
      it = it->next;
    }
  }
  g_list_free_full(failed_list, g_object_unref);

  enable();
}

std::pair<int, int>
PortalInputCapture::mapPortalActivationToComputerPosition(guint barrierId, double rawX, double rawY) const
{
  auto x = static_cast<int>(rawX);
  auto y = static_cast<int>(rawY);

  const auto it = std::ranges::find_if(m_barrierInfo, [barrierId](const auto &info) { return info.id == barrierId; });
  if (it == m_barrierInfo.end()) {
    LOG_DEBUG("activated barrier %u is not in the current pointer barrier set", barrierId);
    return {x, y};
  }

  const auto zoneLeft = it->x;
  const auto zoneTop = it->y;
  const auto zoneRight = it->x + static_cast<gint>(it->width) - 1;
  const auto zoneBottom = it->y + static_cast<gint>(it->height) - 1;

  std::int32_t computerX;
  std::int32_t computerY;
  std::int32_t computerW;
  std::int32_t computerH;
  m_computer->getShape(computerX, computerY, computerW, computerH);

  if (Bounds portalBounds; getPortalBounds(portalBounds)) {
    x = scaleCoordinateBetweenRanges(rawX, portalBounds.left, portalBounds.right, computerX, computerX + computerW - 1);
    y = scaleCoordinateBetweenRanges(rawY, portalBounds.top, portalBounds.bottom, computerY, computerY + computerH - 1);
  } else {
    x = std::clamp(x, zoneLeft, zoneRight);
    y = std::clamp(y, zoneTop, zoneBottom);
  }

  // The portal reports per-output zones, while Deskflow models the whole computer as one computer.
  // Use the activated barrier's own line to preserve the intended switch direction in Deskflow's
  // aggregate coordinates -- this is the computer's outer edge for a barrier placed on the overall
  // union boundary, but a different, still-correct coordinate for one placed against a shape hole
  // (e.g. the missing corner of an L-shaped arrangement of monitors).
  using enum BarrierSide;
  switch (it->side) {
  case Left:
  case Right:
    x = it->x1;
    break;
  case Top:
  case Bottom:
    y = it->y1;
    break;
  }

  LOG_DEBUG(
      "activation barrier %u (%s) zone=%dx%d@%d,%d raw=%.1f,%.1f -> computer=%d,%d (computer shape %dx%d@%d,%d)",
      barrierId, barrierSideName(it->side), static_cast<gint>(it->width), static_cast<gint>(it->height), it->x, it->y,
      rawX, rawY, x, y, computerW, computerH, computerX, computerY
  );

  return {x, y};
}

std::pair<double, double> PortalInputCapture::mapPortalReleasePosition(double x, double y) const
{
  std::int32_t computerX;
  std::int32_t computerY;
  std::int32_t computerW;
  std::int32_t computerH;
  m_computer->getShape(computerX, computerY, computerW, computerH);

  const auto computerLeft = computerX;
  const auto computerTop = computerY;
  const auto computerRight = computerX + computerW - 1;
  const auto computerBottom = computerY + computerH - 1;
  const auto jumpZoneSize = m_computer->getJumpZoneSize();
  Bounds portalBounds;
  if (!getPortalBounds(portalBounds)) {
    return {x, y};
  }

  auto mappedX = static_cast<std::int32_t>(std::lround(x));
  auto mappedY = static_cast<std::int32_t>(std::lround(y));

  if (BarrierInfo releaseBarrier; getClosestReleaseBarrier(
          x, y, computerLeft, computerTop, computerRight, computerBottom, portalBounds, releaseBarrier
      )) {
    const Bounds releaseBounds = {
        releaseBarrier.x, releaseBarrier.y, releaseBarrier.x + static_cast<gint>(releaseBarrier.width) - 1,
        releaseBarrier.y + static_cast<gint>(releaseBarrier.height) - 1
    };

    using enum BarrierSide;
    switch (releaseBarrier.side) {
    case Left:
      mappedX = std::min(releaseBounds.left + jumpZoneSize, releaseBounds.right);
      mappedY = std::clamp(mappedY, releaseBounds.top, releaseBounds.bottom);
      break;
    case Right:
      mappedX = std::max(releaseBounds.right - jumpZoneSize, releaseBounds.left);
      mappedY = std::clamp(mappedY, releaseBounds.top, releaseBounds.bottom);
      break;
    case Top:
      mappedX = std::clamp(mappedX, releaseBounds.left, releaseBounds.right);
      mappedY = std::min(releaseBounds.top + jumpZoneSize, releaseBounds.bottom);
      break;
    case Bottom:
      mappedX = std::clamp(mappedX, releaseBounds.left, releaseBounds.right);
      mappedY = std::max(releaseBounds.bottom - jumpZoneSize, releaseBounds.top);
      break;
    }
  }

  return {mappedX, mappedY};
}

void PortalInputCapture::addBarrier(
    guint id, BarrierSide side, gint zoneX, gint zoneY, guint zoneWidth, guint zoneHeight
)
{
  gint x1 = 0;
  gint x2 = 0;
  gint y1 = 0;
  gint y2 = 0;

  using enum BarrierSide;
  switch (side) {
  case Left:
    x1 = zoneX;
    y1 = zoneY;
    x2 = zoneX;
    y2 = zoneY + static_cast<gint>(zoneHeight) - 1;
    break;
  case Right:
    x1 = zoneX + static_cast<gint>(zoneWidth);
    y1 = zoneY;
    x2 = x1;
    y2 = zoneY + static_cast<gint>(zoneHeight) - 1;
    break;
  case Top:
    x1 = zoneX;
    y1 = zoneY;
    x2 = zoneX + static_cast<gint>(zoneWidth) - 1;
    y2 = zoneY;
    break;
  case Bottom:
    x1 = zoneX;
    y1 = zoneY + static_cast<gint>(zoneHeight);
    x2 = zoneX + static_cast<gint>(zoneWidth) - 1;
    y2 = y1;
    break;
  }

  LOG_DEBUG("barrier (%s) %u at %d,%d-%d,%d", barrierSideName(side), id, x1, y1, x2, y2);
  m_barriers.push_back(XDP_INPUT_CAPTURE_POINTER_BARRIER(
      g_object_new(XDP_TYPE_INPUT_CAPTURE_POINTER_BARRIER, "id", id, "x1", x1, "y1", y1, "x2", x2, "y2", y2, nullptr)
  ));
  m_barrierInfo.push_back({id, side, zoneX, zoneY, zoneWidth, zoneHeight, x1, y1, x2, y2});
}

gboolean PortalInputCapture::initSession()
{
  LOG_DEBUG("setting up input capture session");
  XdpInputCaptureSession *session;
#ifdef HAVE_LIBPORTAL_INPUTCAPTURE_RESTORE
  g_autoptr(GError) error = nullptr;

  m_portalVersion = xdp_portal_get_input_capture_version_sync(
      m_portal,
      nullptr, // Cancellable
      nullptr
  );
  LOG_DEBUG("input capture version %d", m_portalVersion);

  switch (m_portalVersion) {
  case 0:
    LOG_WARN("portal bug: input capture version is %d", m_portalVersion);
    // fallthrough
  case 1:
    xdp_portal_create_input_capture_session(
        m_portal,
        nullptr, // parent
        static_cast<XdpInputCapability>(XDP_INPUT_CAPABILITY_KEYBOARD | XDP_INPUT_CAPABILITY_POINTER),
        nullptr, // cancellable
        [](GObject *obj, GAsyncResult *res, gpointer data) {
          static_cast<PortalInputCapture *>(data)->handleInitSession(obj, res);
        },
        this
    );
    break;
  default:
    session = xdp_portal_create_input_capture_session2_sync(
        m_portal,
        nullptr, // Cancellable
        &error
    );
    if (!session) {
      LOG_ERR("failed to initialize input capture session, quitting: %s", error->message);
      g_main_loop_quit(m_glibMainLoop);
      m_events->addEvent(Event(EventTypes::Quit));
      return FALSE;
    }
    m_session = session;
    xdp_session_request_clipboard(xdp_input_capture_session_get_session(session));
    xdp_input_capture_session_set_session_persistence(session, XDP_INPUT_CAPTURE_SESSION_PERSISTENCE_PERSISTENT);
    if (auto sessionToken = Settings::value(Settings::Server::XdpRestoreToken).toByteArray(); !sessionToken.isEmpty()) {
      xdp_input_capture_session_set_restore_token(session, strdup(sessionToken.data()));
    }
    xdp_input_capture_session_start(
        m_session,
        nullptr, // parent
        static_cast<XdpInputCapability>(XDP_INPUT_CAPABILITY_KEYBOARD | XDP_INPUT_CAPABILITY_POINTER),
        nullptr, // cancellable
        [](GObject *obj, GAsyncResult *res, gpointer data) {
          static_cast<PortalInputCapture *>(data)->handleStart(obj, res);
        },
        this
    );
    break;
  }
#else
  xdp_portal_create_input_capture_session(
      m_portal,
      nullptr, // parent
      static_cast<XdpInputCapability>(XDP_INPUT_CAPABILITY_KEYBOARD | XDP_INPUT_CAPABILITY_POINTER),
      nullptr, // cancellable
      [](GObject *obj, GAsyncResult *res, gpointer data) {
        static_cast<PortalInputCapture *>(data)->handleInitSession(obj, res);
      },
      this
  );
#endif

  return false;
}

void PortalInputCapture::enable()
{
  if (!m_enabled) {
    LOG_DEBUG("enabling the portal input capture session");
    xdp_input_capture_session_enable(m_session);
    m_enabled = true;
  }
}

void PortalInputCapture::disable()
{
  if (m_enabled) {
    LOG_DEBUG("disabling the portal input capture session");
    xdp_input_capture_session_disable(m_session);
    m_enabled = false;
  }
}

void PortalInputCapture::refreshPointerBarriers()
{
  if (m_session == nullptr) {
    return;
  }

  // this is called from EiComputer::reconfigure(), which runs on deskflow's
  // own event-queue thread -- the async xdp_input_capture_session_* calls
  // inside handleZonesChanged() must originate from the glib thread that
  // actually iterates m_glibMainLoop's context
  g_idle_add(
      [](gpointer data) -> gboolean {
        auto *self = static_cast<PortalInputCapture *>(data);
        if (self->m_session != nullptr) {
          LOG_DEBUG("refreshing pointer barriers");
          self->handleZonesChanged(self->m_session, nullptr);
        }
        return G_SOURCE_REMOVE;
      },
      this
  );
}

void PortalInputCapture::release()
{
  LOG_DEBUG("releasing input capture session, id=%d", m_activationId);
  xdp_input_capture_session_release(m_session, m_activationId);
  m_isActive = false;
}

void PortalInputCapture::release(double x, double y)
{
  const auto [mappedX, mappedY] = mapPortalReleasePosition(x, y);
  LOG_DEBUG(
      "releasing input capture session, id=%d x=%.1f y=%.1f mapped=%.1f,%.1f", m_activationId, x, y, mappedX, mappedY
  );
  xdp_input_capture_session_release_at(m_session, m_activationId, mappedX, mappedY);
  m_isActive = false;
}

void PortalInputCapture::handleDisabled(const XdpInputCaptureSession *, const GVariant *)
{
  LOG_DEBUG("portal cb disabled");

  if (!m_enabled)
    return; // Nothing to do

  m_enabled = false;
  m_isActive = false;

  // FIXME: need some better heuristics here of when we want to enable again
  // But we don't know *why* we got disabled (and it's doubtfull we ever
  // will), so we just assume that the zones will change or something and we
  // can re-enable again
  // ... very soon
  g_timeout_add(
      1000,
      [](gpointer data) -> gboolean {
        static_cast<PortalInputCapture *>(data)->enable();
        return false;
      },
      this
  );
}

void PortalInputCapture::handleActivated(
    const XdpInputCaptureSession *, const std::uint32_t activationId, GVariant *options
)
{
  LOG_DEBUG("portal cb activated, id=%d", activationId);

  if (options) {
    gdouble x;
    gdouble y;
    if (g_variant_lookup(options, "cursor_position", "(dd)", &x, &y)) {
      auto warpX = static_cast<int>(x);
      auto warpY = static_cast<int>(y);

      guint barrierId = 0;
      m_lastActivationBarrierId = 0;

      if (const bool hasBarrierId = g_variant_lookup(options, "barrier_id", "u", &barrierId);
          hasBarrierId && barrierId > 0) {
        auto [mappedX, mappedY] = mapPortalActivationToComputerPosition(barrierId, x, y);
        warpX = mappedX;
        warpY = mappedY;
        m_lastActivationBarrierId = barrierId;
      } else if (!hasBarrierId) {
        LOG_DEBUG("portal activation has no barrier id, using raw cursor position");
      } else {
        LOG_DEBUG("portal activation barrier id is zero, using raw cursor position");
      }

      m_computer->warpCursor(warpX, warpY);
      m_events->addEvent(Event(
          EventTypes::PrimaryComputerMotionOnPrimary, m_computer->getEventTarget(),
          IPrimaryComputer::MotionInfo::alloc(warpX, warpY)
      ));
    } else {
      LOG_WARN("failed to get cursor position");
    }
  } else {
    LOG_WARN("activation has no options");
  }
  m_activationId = activationId;
  m_isActive = true;

#ifdef HAVE_LIBPORTAL_CLIPBOARD
  if (m_session) {
    LOG_DEBUG("reading clipboard selection on activation");
    m_computer->sendClipboardEvent(EventTypes::ClipboardGrabbed, kClipboardClipboard);

    XdpSession *session = xdp_input_capture_session_get_session(m_session);
    const char **mimeTypes = xdp_session_get_selection_mime_types(session);

    if (mimeTypes && mimeTypes[0]) {
      LOG_DEBUG("clipboard current selection mime types: %s", PortalClipboard::formatMimeTypes(mimeTypes).constData());
      if (!xdp_session_is_selection_owned_by_session(session))
        readClipboardSelection(session);
    } else {
      LOG_DEBUG("no current clipboard selection");
    }

    claimClipboardOwnership(session);
    LOG_DEBUG("activation clipboard handling complete");
  } else {
    LOG_WARN("input capture activated without a session, skipping clipboard read");
  }
#endif
}

void PortalInputCapture::handleDeactivated(
    const XdpInputCaptureSession *, const std::uint32_t activationId, const GVariant *
)
{
  LOG_DEBUG("cb deactivated, id=%i", activationId);
  m_isActive = false;
}

void PortalInputCapture::handleZonesChanged(XdpInputCaptureSession *session, const GVariant *)
{
  for (auto b : m_barriers)
    g_object_unref(b);
  m_barriers.clear();
  m_barrierInfo.clear();

  const auto activeSides = m_computer->activeSides();
  using enum DirectionMask;

  auto zones = xdp_input_capture_session_get_zones(session);

  // First pass: record every zone's rect and compute the bounding box (union) of
  // all of them. A pointer barrier must lie on the outer boundary of the combined
  // desktop and be adjacent to a single monitor edge. A barrier placed on an
  // internal edge between two adjacent monitors is rejected by the portal
  // ("adjacent to multiple monitor edges"), and a single rejected barrier fails
  // the whole barrier set - so on a multi-monitor server input capture never
  // engages.
  struct ZoneRect
  {
    gint x = 0;
    gint y = 0;
    guint w = 0;
    guint h = 0;
  };
  std::vector<ZoneRect> zoneRects;
  gint unionLeft = 0;
  gint unionTop = 0;
  gint unionRight = 0;
  gint unionBottom = 0;
  bool boundsInit = false;
  for (auto z = zones; z != nullptr; z = z->next) {
    guint w;
    guint h;
    gint x;
    gint y;
    g_object_get(z->data, "width", &w, "height", &h, "x", &x, "y", &y, nullptr);
    zoneRects.push_back({x, y, w, h});
    const gint right = x + static_cast<gint>(w);
    const gint bottom = y + static_cast<gint>(h);
    if (!boundsInit) {
      unionLeft = x;
      unionTop = y;
      unionRight = right;
      unionBottom = bottom;
      boundsInit = true;
    } else {
      unionLeft = std::min(unionLeft, x);
      unionTop = std::min(unionTop, y);
      unionRight = std::max(unionRight, right);
      unionBottom = std::max(unionBottom, bottom);
    }
  }

  m_zoneUnionValid = boundsInit;
  m_zoneUnionLeft = unionLeft;
  m_zoneUnionTop = unionTop;
  m_zoneUnionRight = unionRight;
  m_zoneUnionBottom = unionBottom;

  // the OTHER zone (if any) that also has an edge of the same orientation at
  // `line` (e.g. another monitor's bottom edge at the same y as this one).
  // The portal requires a barrier to be unambiguously "within one zone"; if
  // two zones share the exact same edge coordinate, a barrier placed there --
  // however narrow -- is liable to be rejected, since the compositor can't
  // tell which zone it belongs to.
  const auto otherZoneSharingEdge = [&zoneRects](std::size_t selfIndex, bool vertical,
                                                  gint line) -> std::optional<ZoneRect> {
    for (std::size_t i = 0; i < zoneRects.size(); ++i) {
      if (i == selfIndex) {
        continue;
      }
      const auto &z = zoneRects[i];
      if (vertical ? (z.x == line || z.x + static_cast<gint>(z.w) == line)
                   : (z.y == line || z.y + static_cast<gint>(z.h) == line)) {
        return z;
      }
    }
    return std::nullopt;
  };

  // warn when a hole-adjacent barrier's edge coincides with another monitor's
  // edge, since that's the one geometry the portal has been observed to reject
  // outright (see the comment above) with no way to route around it in
  // software. named clearly enough for a non-technical user to act on, and
  // only logged once per (hole, side) for the life of the session -- this is
  // re-evaluated every time handleZonesChanged() runs, which happens far more
  // often than the monitor layout actually changes
  const auto warnIfSharedEdge = [this, &otherZoneSharingEdge, &zoneRects](
                                     std::size_t zoneIndex, BarrierSide side, bool vertical, gint line,
                                     deskflow::HoleLocation location
                                 ) {
    const auto other = otherZoneSharingEdge(zoneIndex, vertical, line);
    if (!other.has_value()) {
      return;
    }
    if (!m_warnedSharedEdges.insert({location, side}).second) {
      return; // already warned about this one this session
    }
    const auto &zone = zoneRects[zoneIndex];
    const char axis = vertical ? 'x' : 'y';
    LOG_INFO(
        "the monitor at %dx%d@%d,%d and the monitor at %dx%d@%d,%d are aligned exactly on %c=%d -- DeskFlow can't "
        "switch through the \"%s\" side of the \"%s\" hole there, because the display server can't tell which "
        "monitor's edge a switching boundary placed at that exact pixel belongs to. Moving either monitor by a "
        "single pixel so they no longer share that %c coordinate will fix it.",
        zone.w, zone.h, zone.x, zone.y, other->w, other->h, other->x, other->y, axis, line, barrierSideName(side),
        location.c_str(), axis
    );
  };

  // Second pass: only add a barrier for a zone edge that is part of the outer
  // boundary of the union (i.e. not an internal seam between two monitors).
  const auto holes = m_computer->getShapeHoles();
  guint id = 0;
  for (std::size_t zoneIndex = 0; zoneIndex < zoneRects.size(); ++zoneIndex) {
    const auto &zone = zoneRects[zoneIndex];
    const gint x = zone.x;
    const gint y = zone.y;
    const guint w = zone.w;
    const guint h = zone.h;

    LOG_DEBUG("input capture zone, %dx%d@%d,%d", w, h, x, y);

    if ((activeSides & static_cast<int>(LeftMask)) && x == unionLeft) {
      addBarrier(++id, BarrierSide::Left, x, y, w, h);
    }

    if ((activeSides & static_cast<int>(RightMask)) && (x + static_cast<gint>(w)) == unionRight) {
      addBarrier(++id, BarrierSide::Right, x, y, w, h);
    }

    if ((activeSides & static_cast<int>(TopMask)) && y == unionTop) {
      addBarrier(++id, BarrierSide::Top, x, y, w, h);
    }

    if ((activeSides & static_cast<int>(BottomMask)) && (y + static_cast<gint>(h)) == unionBottom) {
      addBarrier(++id, BarrierSide::Bottom, x, y, w, h);
    }

    // a zone edge bordering a shape hole (e.g. the missing corner of an
    // L-shaped arrangement of monitors) is also an outer boundary from this
    // zone's own perspective, even though it sits inside the overall union
    // and so was skipped above -- place a barrier there too. The portal
    // rejects a barrier that doesn't span a reported zone's entire edge (it
    // must match one exactly, not an arbitrary sub-segment of it), so this
    // only uses the hole to decide whether this zone's edge qualifies at
    // all, then places the barrier along the zone's full edge regardless of
    // how much of it the hole actually covers -- any extra length just
    // borders a neighboring zone, which is a harmless local seam the OS
    // already lets the cursor cross without deskflow's involvement.
    for (const auto &hole : holes) {
      const auto directions = deskflow::activeDirections(hole.location);
      const auto hasDirection = [&directions](Direction d) {
        return std::ranges::find(directions, d) != directions.end();
      };
      const auto hx = static_cast<gint>(hole.rect.x);
      const auto hy = static_cast<gint>(hole.rect.y);
      const auto hw = static_cast<gint>(hole.rect.w);
      const auto hh = static_cast<gint>(hole.rect.h);
      const bool overlapsVertically = y < hy + hh && (y + static_cast<gint>(h)) > hy;
      const bool overlapsHorizontally = x < hx + hw && (x + static_cast<gint>(w)) > hx;

      // this zone's left edge borders the hole's right edge
      if (hasDirection(Direction::Left) && x == hx + hw && overlapsVertically) {
        warnIfSharedEdge(zoneIndex, BarrierSide::Left, true, x, hole.location);
        addBarrier(++id, BarrierSide::Left, x, y, w, h);
      }
      // this zone's right edge borders the hole's left edge
      if (hasDirection(Direction::Right) && (x + static_cast<gint>(w)) == hx && overlapsVertically) {
        warnIfSharedEdge(zoneIndex, BarrierSide::Right, true, x + static_cast<gint>(w), hole.location);
        addBarrier(++id, BarrierSide::Right, x, y, w, h);
      }
      // this zone's top edge borders the hole's bottom edge
      if (hasDirection(Direction::Top) && y == hy + hh && overlapsHorizontally) {
        warnIfSharedEdge(zoneIndex, BarrierSide::Top, false, y, hole.location);
        addBarrier(++id, BarrierSide::Top, x, y, w, h);
      }
      // this zone's bottom edge borders the hole's top edge
      if (hasDirection(Direction::Bottom) && (y + static_cast<gint>(h)) == hy && overlapsHorizontally) {
        warnIfSharedEdge(zoneIndex, BarrierSide::Bottom, false, y + static_cast<gint>(h), hole.location);
        addBarrier(++id, BarrierSide::Bottom, x, y, w, h);
      }
    }
  }

  GList *list = nullptr;
  for (auto const &b : m_barriers) {
    list = g_list_append(list, b);
  }

  if (list != nullptr) {
    xdp_input_capture_session_set_pointer_barriers(
        m_session, list,
        nullptr, // cancellable
        [](GObject *obj, GAsyncResult *res, gpointer data) {
          static_cast<PortalInputCapture *>(data)->handleSetPointerBarriers(obj, res);
        },
        this
    );
  } else {
    LOG_WARN("no input capture pointer barriers found");
  }
}

void PortalInputCapture::glibThread(const void *)
{
  auto context = g_main_loop_get_context(m_glibMainLoop);

  LOG_DEBUG("glib thread running");

  while (g_main_loop_is_running(m_glibMainLoop)) {
    Thread::testCancel();
    g_main_context_iteration(context, true);
  }

  LOG_DEBUG("shutting down glib thread");
}

} // namespace deskflow
