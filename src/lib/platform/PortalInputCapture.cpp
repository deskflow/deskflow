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

#include <cerrno>
#include <cstring>
#include <unistd.h>

#include <sys/socket.h> // for EIS fd hack, remove
#include <sys/un.h>     // for EIS fd hack, remove

namespace deskflow {

PortalInputCapture::PortalInputCapture(EiScreen *screen, IEventQueue *events)
    : m_screen{screen},
      m_events{events},
      m_portal{xdp_portal_new()},
      m_clipboard{new EiClipboard(kClipboardClipboard)}
{
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

#ifdef HAVE_LIBPORTAL_INPUTCAPTURE_CLIPBOARD
    if (m_signals.at(SelectionOwnerChanged))
      g_signal_handler_disconnect(G_OBJECT(parentSession), m_signals.at(SelectionOwnerChanged));
    if (m_signals.at(SelectionTransfer))
      g_signal_handler_disconnect(G_OBJECT(parentSession), m_signals.at(SelectionTransfer));
#endif

    g_object_unref(m_session);
  }

  for (auto b : m_barriers)
    g_object_unref(b);
  m_barriers.clear();
  g_object_unref(m_portal);

  delete m_clipboard;
}

EiClipboard *PortalInputCapture::getClipboard(ClipboardID id) const
{
  if (id == kClipboardClipboard)
    return m_clipboard;
  return nullptr;
}

void PortalInputCapture::notifySelectionChanged()
{
#ifdef HAVE_LIBPORTAL_INPUTCAPTURE_CLIPBOARD
  if (!m_session)
    return;

  XdpSession *parentSession = xdp_input_capture_session_get_session(m_session);
  if (!xdp_session_is_clipboard_enabled(parentSession))
    return;

  std::vector<const char *> mimeTypes;
  m_clipboard->open(0);
  if (m_clipboard->has(IClipboard::Format::Text)) {
    mimeTypes.push_back("text/plain;charset=utf-8");
    mimeTypes.push_back("text/plain");
  }
  if (m_clipboard->has(IClipboard::Format::HTML)) {
    mimeTypes.push_back("text/html");
  }
  m_clipboard->close();
  mimeTypes.push_back(nullptr);

  if (mimeTypes.size() == 1) {
    // only the null terminator — nothing to advertise
    return;
  }

  xdp_session_set_selection(parentSession, mimeTypes.data());
  LOG_DEBUG("portal clipboard: advertised %zu MIME type(s) to compositor", mimeTypes.size() - 1);
#endif
}

gboolean PortalInputCapture::timeoutHandler() const
{
  return true;
}

void PortalInputCapture::handleSessionClosed(XdpSession *session)
{
  LOG_ERR("portal input capture session was closed, exiting");
  g_main_loop_quit(m_glibMainLoop);
  m_events->addEvent(Event(EventTypes::Quit));

  g_signal_handler_disconnect(session, m_signals.at(Signal::SessionClosed));
  m_signals.at(Signal::SessionClosed) = 0;
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

  m_events->addEvent(Event(EventTypes::EIConnected, m_screen->getEventTarget(), EiScreen::EiConnectInfo::alloc(fd)));

  XdpSession *parentSession = xdp_input_capture_session_get_session(session);

  using enum Signal;
  m_signals.at(Disabled)      = g_signal_connect(G_OBJECT(session), "disabled",        G_CALLBACK(disabled),      this);
  m_signals.at(Activated)     = g_signal_connect(G_OBJECT(session), "activated",       G_CALLBACK(activated),     this);
  m_signals.at(Deactivated)   = g_signal_connect(G_OBJECT(session), "deactivated",     G_CALLBACK(deactivated),   this);
  m_signals.at(ZonesChanged)  = g_signal_connect(G_OBJECT(session), "zones-changed",   G_CALLBACK(zonesChanged),  this);
  m_signals.at(SessionClosed) = g_signal_connect(G_OBJECT(parentSession), "closed",    G_CALLBACK(sessionClosed), this);

#ifdef HAVE_LIBPORTAL_INPUTCAPTURE_CLIPBOARD
  if (xdp_session_is_clipboard_enabled(parentSession)) {
    m_signals.at(SelectionOwnerChanged) = g_signal_connect(
        G_OBJECT(parentSession), "selection-owner-changed", G_CALLBACK(selectionOwnerChanged), this
    );
    m_signals.at(SelectionTransfer) = g_signal_connect(
        G_OBJECT(parentSession), "selection-transfer", G_CALLBACK(selectionTransfer), this
    );
    LOG_DEBUG("portal clipboard: connected clipboard signals");
  } else {
    LOG_WARN("portal clipboard: compositor did not grant clipboard access");
  }
#endif

  handleZonesChanged(session, nullptr);
}

void PortalInputCapture::handleInitSession(GObject *object, GAsyncResult *res)
{
  LOG_DEBUG("portal input capture session initialized");
  g_autoptr(GError) error = nullptr;

  auto session = xdp_portal_create_input_capture_session_finish(XDP_PORTAL(object), res, &error);
  if (!session) {
    LOG_ERR("failed to initialize input capture session: %s", error->message);
    g_main_loop_quit(m_glibMainLoop);
    m_events->addEvent(Event(EventTypes::Quit));
    return;
  }

  m_session = session;

#ifdef HAVE_LIBPORTAL_INPUTCAPTURE_CLIPBOARD
  XdpSession *parentSession = xdp_input_capture_session_get_session(session);
  xdp_session_request_clipboard(parentSession);
  LOG_DEBUG("portal clipboard: requested clipboard access");
#endif

  setupSession(session);
}

#ifdef HAVE_LIBPORTAL_INPUTCAPTURE_RESTORE

void PortalInputCapture::handleInitSession2(GObject *object, GAsyncResult *res)
{
  LOG_DEBUG("portal input capture session2 created");
  g_autoptr(GError) error = nullptr;

  auto session = xdp_portal_create_input_capture_session2_finish(XDP_PORTAL(object), res, &error);
  if (!session) {
    LOG_ERR("failed to create input capture session2: %s", error->message);
    g_main_loop_quit(m_glibMainLoop);
    m_events->addEvent(Event(EventTypes::Quit));
    return;
  }

  m_session = session;

  xdp_input_capture_session_set_session_persistence(session, XDP_INPUT_CAPTURE_SESSION_PERSISTENCE_PERSISTENT);
  if (auto token = Settings::value(Settings::Server::XdpRestoreToken).toByteArray(); !token.isEmpty())
    xdp_input_capture_session_set_restore_token(session, token.data());

#ifdef HAVE_LIBPORTAL_INPUTCAPTURE_CLIPBOARD
  XdpSession *parentSession = xdp_input_capture_session_get_session(session);
  xdp_session_request_clipboard(parentSession);
  LOG_DEBUG("portal clipboard: requested clipboard access");
#endif

  xdp_input_capture_session_start(
      session, nullptr,
      static_cast<XdpInputCapability>(XDP_INPUT_CAPABILITY_KEYBOARD | XDP_INPUT_CAPABILITY_POINTER),
      nullptr,
      [](GObject *obj, GAsyncResult *res, gpointer data) {
        static_cast<PortalInputCapture *>(data)->handleStart(obj, res);
      },
      this
  );
}

void PortalInputCapture::handleStart(GObject *, GAsyncResult *res)
{
  g_autoptr(GError) error = nullptr;

  if (!xdp_input_capture_session_start_finish(m_session, res, &error)) {
    LOG_ERR("failed to start input capture session: %s", error->message);
    g_main_loop_quit(m_glibMainLoop);
    m_events->addEvent(Event(EventTypes::Quit));
    return;
  }

  if (auto token = xdp_input_capture_session_get_restore_token(m_session); token) {
    Settings::setValue(Settings::Server::XdpRestoreToken, QString(token));
    LOG_DEBUG("portal: saved input capture restore token");
  }

  setupSession(m_session);
}

#endif // HAVE_LIBPORTAL_INPUTCAPTURE_RESTORE

#ifdef HAVE_LIBPORTAL_INPUTCAPTURE_CLIPBOARD

void PortalInputCapture::handleSelectionOwnerChanged(XdpSession *session, GStrv mimeTypes, gboolean sessionIsOwner)
{
  if (sessionIsOwner || !mimeTypes) // ignore our own selection changes
    return;

  // Pick the best available format. Prefer UTF-8 text, fall back to plain.
  const char *textMime = nullptr;
  bool hasHtml = false;

  for (int i = 0; mimeTypes[i] != nullptr; ++i) {
    if (g_str_equal(mimeTypes[i], "text/plain;charset=utf-8"))
      textMime = mimeTypes[i];
    else if (!textMime && g_str_equal(mimeTypes[i], "text/plain"))
      textMime = mimeTypes[i];
    else if (g_str_has_prefix(mimeTypes[i], "text/html"))
      hasHtml = true;
  }

  if (!textMime && !hasHtml) {
    LOG_DEBUG("portal clipboard: remote has no text content, ignoring");
    return;
  }

  m_clipboard->open(0);
  m_clipboard->empty();

  if (textMime) {
    int fd = xdp_session_selection_read(session, textMime);
    if (fd >= 0) {
      std::string data;
      char buf[4096];
      ssize_t n;
      while ((n = read(fd, buf, sizeof(buf))) > 0)
        data.append(buf, n);
      if (n < 0)
        LOG_WARN("portal clipboard: read error on text fd: %s", strerror(errno));
      close(fd);

      if (!data.empty()) {
        m_clipboard->add(IClipboard::Format::Text, data);
        LOG_DEBUG("portal clipboard: received %zu bytes of text from compositor", data.size());
      }
    } else {
      LOG_WARN("portal clipboard: selection_read failed for %s", textMime);
    }
  }

  if (hasHtml) {
    int fd = xdp_session_selection_read(session, "text/html");
    if (fd >= 0) {
      std::string data;
      char buf[4096];
      ssize_t n;
      while ((n = read(fd, buf, sizeof(buf))) > 0)
        data.append(buf, n);
      if (n < 0)
        LOG_WARN("portal clipboard: read error on html fd: %s", strerror(errno));
      close(fd);

      if (!data.empty()) {
        m_clipboard->add(IClipboard::Format::HTML, data);
        LOG_DEBUG("portal clipboard: received %zu bytes of HTML from compositor", data.size());
      }
    }
  }

  m_clipboard->close();

  m_screen->sendClipboardEvent(EventTypes::ClipboardChanged, kClipboardClipboard);
}

void PortalInputCapture::handleSelectionTransfer(XdpSession *session, const char *mimeType, guint32 serial)
{
  IClipboard::Format format;

  if (g_str_equal(mimeType, "text/plain;charset=utf-8") || g_str_equal(mimeType, "text/plain")) {
    format = IClipboard::Format::Text;
  } else if (g_str_has_prefix(mimeType, "text/html")) {
    format = IClipboard::Format::HTML;
  } else {
    LOG_WARN("portal clipboard: unsupported MIME type requested: %s", mimeType);
    xdp_session_selection_write_done(session, serial, false);
    return;
  }

  m_clipboard->open(0);
  const bool haveData = m_clipboard->has(format);
  const std::string data = haveData ? m_clipboard->get(format) : std::string{};
  m_clipboard->close();

  if (!haveData || data.empty()) {
    LOG_WARN("portal clipboard: no data for %s", mimeType);
    xdp_session_selection_write_done(session, serial, false);
    return;
  }

  int fd = xdp_session_selection_write(session, serial);
  if (fd < 0) {
    LOG_WARN("portal clipboard: selection_write failed for serial %u", serial);
    xdp_session_selection_write_done(session, serial, false);
    return;
  }

  const char *ptr = data.data();
  size_t remaining = data.size();
  bool ok = true;

  while (remaining > 0) {
    ssize_t n = write(fd, ptr, remaining);
    if (n < 0) {
      if (errno == EINTR)
        continue;
      LOG_WARN("portal clipboard: write error for serial %u: %s", serial, strerror(errno));
      ok = false;
      break;
    }
    ptr += n;
    remaining -= n;
  }

  close(fd);
  xdp_session_selection_write_done(session, serial, ok);

  LOG_DEBUG("portal clipboard: served %zu bytes of %s (serial %u)", data.size(), mimeType, serial);
}

#endif // HAVE_LIBPORTAL_INPUTCAPTURE_CLIPBOARD

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
          int x1, x2, y1, y2;
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

#ifdef HAVE_LIBPORTAL_INPUTCAPTURE_RESTORE
  m_portalVersion = xdp_portal_get_input_capture_version_sync(m_portal, nullptr, nullptr);
  LOG_DEBUG("input capture portal version: %d", m_portalVersion);

  if (m_portalVersion >= 2) {
    xdp_portal_create_input_capture_session2(
        m_portal, nullptr,
        [](GObject *obj, GAsyncResult *res, gpointer data) {
          static_cast<PortalInputCapture *>(data)->handleInitSession2(obj, res);
        },
        this
    );
    return false;
  }
#endif

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
    return;

  m_enabled = false;
  m_isActive = false;

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
    gdouble x, y;
    if (g_variant_lookup(options, "cursor_position", "(dd)", &x, &y)) {
      m_screen->warpCursor((int)x, (int)y);
    } else {
      LOG_WARN("activation has no cursor position");
    }
  }

  m_activationId = activationId;
  m_isActive = true;
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

  auto zones = xdp_input_capture_session_get_zones(session);
  while (zones != nullptr) {
    guint w, h;
    gint x, y;
    g_object_get(zones->data, "width", &w, "height", &h, "x", &x, "y", &y, nullptr);
    LOG_DEBUG("input capture zone, %dx%d@%d,%d", w, h, x, y);

    int x1, x2, y1, y2;
    auto id = 0;

    if (activeSides & static_cast<int>(LeftMask)) {
      id++;
      x1 = x; y1 = y; x2 = x; y2 = y + h - 1;
      LOG_DEBUG("barrier (left) %d at %d,%d-%d,%d", id, x1, y1, x2, y2);
      m_barriers.push_back(XDP_INPUT_CAPTURE_POINTER_BARRIER(g_object_new(
          XDP_TYPE_INPUT_CAPTURE_POINTER_BARRIER, "id", id, "x1", x1, "y1", y1, "x2", x2, "y2", y2, nullptr
      )));
    }

    if (activeSides & static_cast<int>(RightMask)) {
      id++;
      x1 = x + w; y1 = y; x2 = x + w; y2 = y + h - 1;
      LOG_DEBUG("barrier (right) %d at %d,%d-%d,%d", id, x1, y1, x2, y2);
      m_barriers.push_back(XDP_INPUT_CAPTURE_POINTER_BARRIER(g_object_new(
          XDP_TYPE_INPUT_CAPTURE_POINTER_BARRIER, "id", id, "x1", x1, "y1", y1, "x2", x2, "y2", y2, nullptr
      )));
    }

    if (activeSides & static_cast<int>(TopMask)) {
      id++;
      x1 = x; y1 = y; x2 = x + w - 1; y2 = y;
      LOG_DEBUG("barrier (top) %d at %d,%d-%d,%d", id, x1, y1, x2, y2);
      m_barriers.push_back(XDP_INPUT_CAPTURE_POINTER_BARRIER(g_object_new(
          XDP_TYPE_INPUT_CAPTURE_POINTER_BARRIER, "id", id, "x1", x1, "y1", y1, "x2", x2, "y2", y2, nullptr
      )));
    }

    if (activeSides & static_cast<int>(BottomMask)) {
      id++;
      x1 = x; y1 = y + h; x2 = x + w - 1; y2 = y + h;
      LOG_DEBUG("barrier (bottom) %d at %d,%d-%d,%d", id, x1, y1, x2, y2);
      m_barriers.push_back(XDP_INPUT_CAPTURE_POINTER_BARRIER(g_object_new(
          XDP_TYPE_INPUT_CAPTURE_POINTER_BARRIER, "id", id, "x1", x1, "y1", y1, "x2", x2, "y2", y2, nullptr
      )));
    }

    zones = zones->next;
  }

  GList *list = nullptr;
  for (auto const &b : m_barriers)
    list = g_list_append(list, b);

  if (list != nullptr) {
    xdp_input_capture_session_set_pointer_barriers(
        m_session, list,
        nullptr,
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
