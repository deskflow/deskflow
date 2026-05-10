/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2022 Red Hat, Inc.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/PortalInputCapture.h"
#include "base/DirectionTypes.h"
#include "base/Event.h"
#include "base/Log.h"
#include "base/TMethodJob.h"
#include "deskflow/ClipboardTypes.h"
#include "platform/EiClipboard.h"

#ifdef HAVE_LIBPORTAL_INPUTCAPTURE_RESTORE
#include "common/Settings.h"
#endif

#include <poll.h>
#include <sys/socket.h> // for EIS fd hack, remove
#include <sys/un.h>     // for EIS fd hack, remove

#include <QByteArray>
#include <QByteArrayList>
#include <QFile>

namespace deskflow {

static const char *kSupportedMimeTypes[] = {"text/plain;charset=utf-8", "text/plain", nullptr};

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

    g_signal_handler_disconnect(m_session, m_signals.at(SelectionOwnerChanged));
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

QByteArray PortalInputCapture::formatMimeTypes(const char *const *mimeTypes)
{
  if (!mimeTypes || !mimeTypes[0])
    return QByteArrayLiteral("(none)");

  QByteArrayList parts;
  for (int i = 0; mimeTypes[i]; i++)
    parts.append(mimeTypes[i]);

  return parts.join(", ");
}

bool PortalInputCapture::isSupportedMimeType(const char *mimeType)
{
  if (!mimeType)
    return false;

  for (int i = 0; kSupportedMimeTypes[i]; i++) {
    if (g_strcmp0(mimeType, kSupportedMimeTypes[i]) == 0)
      return true;
  }

  return false;
}

const char *PortalInputCapture::pickSupportedMimeType(const char *const *mimeTypes)
{
  if (!mimeTypes) {
    LOG_WARN("cannot pick mime type, no mime types provided");
    return nullptr;
  }

  for (int i = 0; kSupportedMimeTypes[i]; i++) {
    if (g_strv_contains(mimeTypes, kSupportedMimeTypes[i]))
      return kSupportedMimeTypes[i];
  }

  return nullptr;
}

void PortalInputCapture::claimClipboardOwnership(XdpSession *session)
{
#ifdef HAVE_LIBPORTAL_CLIPBOARD
  LOG_DEBUG("claiming clipboard, mime types: %s", formatMimeTypes(kSupportedMimeTypes).constData());
  xdp_session_set_selection(session, kSupportedMimeTypes);
#endif
}

void PortalInputCapture::readTextClipboardSelection(XdpSession *session)
{
#ifdef HAVE_LIBPORTAL_CLIPBOARD
  constexpr int kClipboardReadTimeoutMs = 200;
  constexpr qint64 kInitialReserveBytes = 64 * 1024;
  const qint64 kClipboardReadCap = static_cast<qint64>(m_screen->maximumClipboardSize()) * 1024;

  LOG_DEBUG("clipboard read cap: %lld bytes", static_cast<long long>(kClipboardReadCap));

  const char **mimeTypes = xdp_session_get_selection_mime_types(session);
  if (!mimeTypes) {
    LOG_DEBUG("clipboard has no mime types available to read");
    return;
  }

  const char *readType = pickSupportedMimeType(mimeTypes);
  if (!readType) {
    LOG_DEBUG(
        "clipboard no supported mime type in selection, available types: %s", formatMimeTypes(mimeTypes).constData()
    );
    return;
  }

  LOG_DEBUG("clipboard reading selection for mime type: %s", readType);

  int fd = xdp_session_selection_read(session, readType);
  if (fd < 0) {
    LOG_ERR("failed to read clipboard selection: invalid fd");
    return;
  }

  QFile pipe;
  if (!pipe.open(fd, QIODevice::ReadOnly, QFileDevice::AutoCloseHandle)) {
    LOG_WARN("failed to wrap clipboard pipe");
    ::close(fd);
    return;
  }

  QByteArray contents;
  contents.reserve(std::min(kClipboardReadCap, kInitialReserveBytes));
  while (contents.size() < kClipboardReadCap) {
    pollfd pfd{fd, POLLIN, 0};
    if (poll(&pfd, 1, kClipboardReadTimeoutMs) <= 0)
      break;
    QByteArray chunk = pipe.read(kClipboardReadCap - contents.size());
    if (chunk.isEmpty())
      break;
    contents.append(chunk);
  }

  while (contents.endsWith('\0'))
    contents.chop(1);
  contents.replace("\r\n", "\n");

  if (contents.isEmpty()) {
    LOG_WARN("clipboard read returned no data");
    return;
  }

  LOG_INFO("clipboard content is: %s", contents.constData());

  m_clipboard->open(0);
  m_clipboard->empty();
  m_clipboard->add(IClipboard::Format::Text, contents.toStdString());
  m_clipboard->close();
  m_screen->sendClipboardEvent(EventTypes::ClipboardGrabbed, kClipboardClipboard);
#else
  (void)session;
#endif
}

void PortalInputCapture::handleSelectionOwnerChanged(XdpSession *session, GStrv mimeTypes, gboolean isOwner)
{
#ifdef HAVE_LIBPORTAL_CLIPBOARD
  if (!mimeTypes || isOwner)
    return;

  if (!pickSupportedMimeType(mimeTypes)) {
    LOG_DEBUG("ignoring selection owner change, no supported mime types: %s", formatMimeTypes(mimeTypes).constData());
    return;
  }

  m_screen->sendClipboardEvent(EventTypes::ClipboardGrabbed, kClipboardClipboard);

  if (!m_isActive) {
    LOG_DEBUG("deferring clipboard read until input capture activates");
    m_pendingClipboardRead = true;
    return;
  }

  readTextClipboardSelection(session);
#endif
}

void PortalInputCapture::handleSelectionTransfer(XdpSession *session, const char *mimeType, uint32_t serial)
{
#ifdef HAVE_LIBPORTAL_CLIPBOARD
  constexpr int kClipboardWriteTimeoutMs = 200;

  LOG_DEBUG("clipboard selection transfer requested, mime type: %s, serial: %u", mimeType, serial);

  if (!isSupportedMimeType(mimeType)) {
    LOG_DEBUG("rejecting clipboard selection transfer, unsupported mime type: %s", mimeType);
    xdp_session_selection_write_done(session, serial, false);
    return;
  }

  int fd = xdp_session_selection_write(session, serial);
  if (fd < 0) {
    LOG_WARN("failed to write clipboard content: invalid fd");
    xdp_session_selection_write_done(session, serial, false);
    return;
  }

  m_clipboard->open(0);
  std::string data;
  if (m_clipboard->has(IClipboard::Format::Text))
    data = m_clipboard->get(IClipboard::Format::Text);
  m_clipboard->close();

  LOG_DEBUG("writing clipboard content: %s", data.c_str());

  QFile pipe;
  if (!pipe.open(fd, QIODevice::WriteOnly, QFileDevice::AutoCloseHandle)) {
    LOG_WARN("failed to wrap clipboard pipe");
    ::close(fd);
    xdp_session_selection_write_done(session, serial, false);
    return;
  }

  const char *buf = data.data();
  qint64 total = static_cast<qint64>(data.size());
  qint64 written = 0;

  while (written < total) {
    pollfd pfd{fd, POLLOUT, 0};
    if (poll(&pfd, 1, kClipboardWriteTimeoutMs) <= 0) {
      LOG_ERR("timed out writing clipboard selection");
      xdp_session_selection_write_done(session, serial, false);
      return;
    }
    qint64 n = pipe.write(buf + written, total - written);
    if (n < 0) {
      LOG_ERR("failed to write clipboard selection");
      xdp_session_selection_write_done(session, serial, false);
      return;
    }
    if (n == 0) {
      LOG_ERR("clipboard pipe accepted no bytes");
      xdp_session_selection_write_done(session, serial, false);
      return;
    }
    written += n;
  }

  xdp_session_selection_write_done(session, serial, true);
  LOG_DEBUG("clipboard write complete");

#endif
}

void PortalInputCapture::setupSession(XdpInputCaptureSession *session)
{
  g_autoptr(GError) error = nullptr;

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
  XdpSession *parentSession = xdp_input_capture_session_get_session(session);
  m_signals.at(Disabled) = g_signal_connect(G_OBJECT(session), "disabled", G_CALLBACK(disabled), this);
  m_signals.at(Activated) = g_signal_connect(G_OBJECT(session), "activated", G_CALLBACK(activated), this);
  m_signals.at(Deactivated) = g_signal_connect(G_OBJECT(session), "deactivated", G_CALLBACK(deactivated), this);
  m_signals.at(ZonesChanged) = g_signal_connect(G_OBJECT(session), "zones-changed", G_CALLBACK(zonesChanged), this);
  m_signals.at(SessionClosed) = g_signal_connect(G_OBJECT(parentSession), "closed", G_CALLBACK(sessionClosed), this);
  handleZonesChanged(session, nullptr);

#ifdef HAVE_LIBPORTAL_CLIPBOARD
  if (xdp_session_is_clipboard_enabled(parentSession)) {
    m_signals.at(SelectionOwnerChanged) =
        g_signal_connect(G_OBJECT(parentSession), "selection-owner-changed", G_CALLBACK(selectionOwnerChanged), this);
    m_signals.at(SelectionTransfer) =
        g_signal_connect(G_OBJECT(parentSession), "selection-transfer", G_CALLBACK(selectionTransfer), this);
  }
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

  auto restoreToken = xdp_input_capture_session_get_restore_token(m_session);
  if (restoreToken) {
    Settings::setValue(Settings::Server::XdpRestoreToken, QString(restoreToken));
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
          break;
        }
      }
      it = it->next;
    }
  }
  g_list_free_full(failed_list, g_object_unref);

  enable();
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
  LOG_DEBUG("releasing input capture session, id=%d x=%.1f y=%.1f", m_activationId, x, y);
  xdp_input_capture_session_release_at(m_session, m_activationId, x, y);
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
      m_screen->warpCursor((int)x, (int)y);
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
    m_pendingClipboardRead = false;
    LOG_DEBUG("reading clipboard selection on activation");
    m_screen->sendClipboardEvent(EventTypes::ClipboardGrabbed, kClipboardClipboard);

    XdpSession *session = xdp_input_capture_session_get_session(m_session);
    const char **mimeTypes = xdp_session_get_selection_mime_types(session);

    if (mimeTypes && mimeTypes[0]) {
      LOG_DEBUG("clipboard current selection mime types: %s", formatMimeTypes(mimeTypes).constData());
      if (!xdp_session_is_selection_owned_by_session(session))
        readTextClipboardSelection(session);
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

  const auto activeSides = m_screen->activeSides();
  using enum DirectionMask;

  // May not correctly handle different sized screens
  auto zones = xdp_input_capture_session_get_zones(session);
  while (zones != nullptr) {
    guint w;
    guint h;
    gint x;
    gint y;
    g_object_get(zones->data, "width", &w, "height", &h, "x", &x, "y", &y, nullptr);

    LOG_DEBUG("input capture zone, %dx%d@%d,%d", w, h, x, y);

    int x1;
    int x2;
    int y1;
    int y2;

    auto id = 0;

    if (activeSides & static_cast<int>(LeftMask)) {
      id++;
      x1 = x;
      y1 = y;
      x2 = x;
      y2 = y + h - 1;
      LOG_DEBUG("barrier (left) %zd at %d,%d-%d,%d", id, x1, y1, x2, y2);
      m_barriers.push_back(XDP_INPUT_CAPTURE_POINTER_BARRIER(g_object_new(
          XDP_TYPE_INPUT_CAPTURE_POINTER_BARRIER, "id", id, "x1", x1, "y1", y1, "x2", x2, "y2", y2, nullptr
      )));
    }

    if (activeSides & static_cast<int>(RightMask)) {
      id++;
      x1 = x + w;
      y1 = y;
      x2 = x + w;
      y2 = y + h - 1;
      LOG_DEBUG("barrier (right) %zd at %d,%d-%d,%d", id, x1, y1, x2, y2);
      m_barriers.push_back(XDP_INPUT_CAPTURE_POINTER_BARRIER(g_object_new(
          XDP_TYPE_INPUT_CAPTURE_POINTER_BARRIER, "id", id, "x1", x1, "y1", y1, "x2", x2, "y2", y2, nullptr
      )));
    }

    if (activeSides & static_cast<int>(TopMask)) {
      id++;
      x1 = x;
      y1 = y;
      x2 = x + w - 1;
      y2 = y;
      LOG_DEBUG("barrier (top) %zd at %d,%d-%d,%d", id, x1, y1, x2, y2);
      m_barriers.push_back(XDP_INPUT_CAPTURE_POINTER_BARRIER(g_object_new(
          XDP_TYPE_INPUT_CAPTURE_POINTER_BARRIER, "id", id, "x1", x1, "y1", y1, "x2", x2, "y2", y2, nullptr
      )));
    }

    if (activeSides & static_cast<int>(BottomMask)) {
      id++;
      x1 = x;
      y1 = y + h;
      x2 = x + w - 1;
      y2 = y + h;
      LOG_DEBUG("barrier (bottom) %zd at %d,%d-%d,%d", id, x1, y1, x2, y2);
      m_barriers.push_back(XDP_INPUT_CAPTURE_POINTER_BARRIER(g_object_new(
          XDP_TYPE_INPUT_CAPTURE_POINTER_BARRIER, "id", id, "x1", x1, "y1", y1, "x2", x2, "y2", y2, nullptr
      )));
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
