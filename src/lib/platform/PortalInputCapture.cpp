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
#include "deskflow/ClipboardTypes.h"
#include "platform/EiClipboard.h"

#ifdef HAVE_LIBPORTAL_CLIPBOARD
#include "platform/PortalClipboard.h"
#endif

#ifdef HAVE_LIBPORTAL_INPUTCAPTURE_RESTORE
#include "common/Settings.h"
#endif

#include <algorithm>
#include <cmath>
#include <limits>
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
  if (m_barrierInfo.empty()) {
    return false;
  }

  bounds.left = std::numeric_limits<gint>::max();
  bounds.top = std::numeric_limits<gint>::max();
  bounds.right = std::numeric_limits<gint>::min();
  bounds.bottom = std::numeric_limits<gint>::min();

  for (const auto &info : m_barrierInfo) {
    bounds.left = std::min(bounds.left, info.x);
    bounds.top = std::min(bounds.top, info.y);
    bounds.right = std::max(bounds.right, info.x + static_cast<gint>(info.width) - 1);
    bounds.bottom = std::max(bounds.bottom, info.y + static_cast<gint>(info.height) - 1);
  }

  return true;
}

bool PortalInputCapture::getClosestReleaseBarrier(
    double x, double y, int screenLeft, int screenTop, int screenRight, int screenBottom, const Bounds &portalBounds,
    BarrierInfo &barrier
) const
{
  const auto activeSides = m_screen->activeSides();
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
    considerSide(BarrierSide::Left, std::abs(x - screenLeft));
  }
  if (activeSides & static_cast<int>(RightMask)) {
    considerSide(BarrierSide::Right, std::abs(x - screenRight));
  }
  if (activeSides & static_cast<int>(TopMask)) {
    considerSide(BarrierSide::Top, std::abs(y - screenTop));
  }
  if (activeSides & static_cast<int>(BottomMask)) {
    considerSide(BarrierSide::Bottom, std::abs(y - screenBottom));
  }
  if (sideDistance == std::numeric_limits<double>::max()) {
    return false;
  }

  const auto portalX = scaleCoordinateBetweenRanges(x, screenLeft, screenRight, portalBounds.left, portalBounds.right);
  const auto portalY = scaleCoordinateBetweenRanges(y, screenTop, screenBottom, portalBounds.top, portalBounds.bottom);

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

PortalInputCapture::PortalInputCapture(EiScreen *screen, IEventQueue *events)
    : m_screen{screen},
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
    g_signal_handler_disconnect(G_OBJECT(parentSession), m_signals.at(SessionClosed));
    g_signal_handler_disconnect(m_session, m_signals.at(Disabled));
    g_signal_handler_disconnect(m_session, m_signals.at(Activated));
    g_signal_handler_disconnect(m_session, m_signals.at(Deactivated));
    g_signal_handler_disconnect(m_session, m_signals.at(ZonesChanged));
    g_signal_handler_disconnect(m_session, m_signals.at(SelectionTransfer));
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
  const qint64 maxBytes = static_cast<qint64>(m_screen->maximumClipboardSize()) * 1024;
  LOG_DEBUG("clipboard read cap: %lld bytes", static_cast<long long>(maxBytes));

  const char **mimeTypes = xdp_session_get_selection_mime_types(session);
  if (!mimeTypes) {
    LOG_DEBUG("clipboard has no mime types available to read");
    return;
  }

  if (PortalClipboard::readSelectionIntoCache(m_clipboard, session, mimeTypes, maxBytes))
    m_screen->sendClipboardEvent(EventTypes::ClipboardGrabbed, kClipboardClipboard);
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
    // Restored sessions can pre-date clipboard support, leaving the channel
    // disabled even though we requested it. Drop the saved token and recreate
    // the session from scratch so the user gets a fresh permission dialog.
    LOG_WARN("clipboard not enabled on session, discarding restore token to force a fresh session");
#ifdef HAVE_LIBPORTAL_INPUTCAPTURE_RESTORE
    Settings::setValue(Settings::Server::XdpRestoreToken, QString());
#endif
    g_object_unref(m_session);
    m_session = nullptr;
    g_idle_add([](gpointer data) { return static_cast<PortalInputCapture *>(data)->initSession(); }, this);
    return;
  }
#endif

  auto fd = xdp_input_capture_session_connect_to_eis(session, &error);
  if (fd < 0) {
    LOG_ERR("failed to connect to eis: %s", error->message);
    g_main_loop_quit(m_glibMainLoop);
    m_events->addEvent(Event(EventTypes::Quit));
    return;
  }

  // Socket ownership is transferred to the EiScreen
  m_events->addEvent(Event(EventTypes::EIConnected, m_screen->getEventTarget(), EiScreen::EiConnectInfo::alloc(fd)));

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
PortalInputCapture::mapPortalActivationToScreenPosition(guint barrierId, double rawX, double rawY) const
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

  std::int32_t screenX;
  std::int32_t screenY;
  std::int32_t screenW;
  std::int32_t screenH;
  m_screen->getShape(screenX, screenY, screenW, screenH);

  if (Bounds portalBounds; getPortalBounds(portalBounds)) {
    x = scaleCoordinateBetweenRanges(rawX, portalBounds.left, portalBounds.right, screenX, screenX + screenW - 1);
    y = scaleCoordinateBetweenRanges(rawY, portalBounds.top, portalBounds.bottom, screenY, screenY + screenH - 1);
  } else {
    x = std::clamp(x, zoneLeft, zoneRight);
    y = std::clamp(y, zoneTop, zoneBottom);
  }

  // The portal reports per-output zones, while Deskflow models the whole computer as one screen.
  // Use the activated barrier to preserve the intended switch direction in Deskflow's aggregate coordinates.
  using enum BarrierSide;
  switch (it->side) {
  case Left:
    x = screenX;
    break;
  case Right:
    x = screenX + screenW - 1;
    break;
  case Top:
    y = screenY;
    break;
  case Bottom:
    y = screenY + screenH - 1;
    break;
  }

  return {x, y};
}

std::pair<double, double> PortalInputCapture::mapPortalReleasePosition(double x, double y) const
{
  std::int32_t screenX;
  std::int32_t screenY;
  std::int32_t screenW;
  std::int32_t screenH;
  m_screen->getShape(screenX, screenY, screenW, screenH);

  const auto screenLeft = screenX;
  const auto screenTop = screenY;
  const auto screenRight = screenX + screenW - 1;
  const auto screenBottom = screenY + screenH - 1;
  const auto jumpZoneSize = m_screen->getJumpZoneSize();
  Bounds portalBounds;
  if (!getPortalBounds(portalBounds)) {
    return {x, y};
  }

  auto mappedX = scaleCoordinateBetweenRanges(x, screenLeft, screenRight, portalBounds.left, portalBounds.right);
  auto mappedY = scaleCoordinateBetweenRanges(y, screenTop, screenBottom, portalBounds.top, portalBounds.bottom);

  if (BarrierInfo releaseBarrier;
      getClosestReleaseBarrier(x, y, screenLeft, screenTop, screenRight, screenBottom, portalBounds, releaseBarrier)) {
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

      if (const bool hasBarrierId = g_variant_lookup(options, "barrier_id", "u", &barrierId);
          hasBarrierId && barrierId > 0) {
        auto [mappedX, mappedY] = mapPortalActivationToScreenPosition(barrierId, x, y);
        warpX = mappedX;
        warpY = mappedY;
      } else if (!hasBarrierId) {
        LOG_DEBUG("portal activation has no barrier id, using raw cursor position");
      } else {
        LOG_DEBUG("portal activation barrier id is zero, using raw cursor position");
      }

      m_screen->warpCursor(warpX, warpY);
      m_events->addEvent(Event(
          EventTypes::PrimaryScreenMotionOnPrimary, m_screen->getEventTarget(),
          IPrimaryScreen::MotionInfo::alloc(warpX, warpY)
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
    m_screen->sendClipboardEvent(EventTypes::ClipboardGrabbed, kClipboardClipboard);

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

  const auto activeSides = m_screen->activeSides();
  using enum DirectionMask;

  // May not correctly handle different sized screens
  auto zones = xdp_input_capture_session_get_zones(session);
  guint id = 0;
  while (zones != nullptr) {
    guint w;
    guint h;
    gint x;
    gint y;
    g_object_get(zones->data, "width", &w, "height", &h, "x", &x, "y", &y, nullptr);

    LOG_DEBUG("input capture zone, %dx%d@%d,%d", w, h, x, y);

    if (activeSides & static_cast<int>(LeftMask)) {
      addBarrier(++id, BarrierSide::Left, x, y, w, h);
    }

    if (activeSides & static_cast<int>(RightMask)) {
      addBarrier(++id, BarrierSide::Right, x, y, w, h);
    }

    if (activeSides & static_cast<int>(TopMask)) {
      addBarrier(++id, BarrierSide::Top, x, y, w, h);
    }

    if (activeSides & static_cast<int>(BottomMask)) {
      addBarrier(++id, BarrierSide::Bottom, x, y, w, h);
    }
    zones = zones->next;
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
