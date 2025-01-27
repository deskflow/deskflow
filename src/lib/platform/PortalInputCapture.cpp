/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2022 Red Hat, Inc.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/PortalInputCapture.h"
#include "base/Event.h"
#include "base/Log.h"
#include "base/TMethodJob.h"

#include <sys/socket.h> // for EIS fd hack, remove
#include <sys/un.h>     // for EIS fd hack, remove

namespace deskflow {

enum signals
{
  SESSION_CLOSED,
  DISABLED,
  ACTIVATED,
  DEACTIVATED,
  ZONES_CHANGED,
  _N_SIGNALS,
};

PortalInputCapture::PortalInputCapture(EiScreen *screen, IEventQueue *events)
    : screen_(screen),
      events_(events),
      portal_(xdp_portal_new()),
      signals_(_N_SIGNALS)
{
  glib_main_loop_ = g_main_loop_new(nullptr, true);
  glib_thread_ = new Thread(new TMethodJob<PortalInputCapture>(this, &PortalInputCapture::glib_thread));

  auto init_capture_cb = [](gpointer data) -> gboolean {
    return reinterpret_cast<PortalInputCapture *>(data)->init_input_capture_session();
  };

  g_idle_add(init_capture_cb, this);
}

PortalInputCapture::~PortalInputCapture()
{
  if (g_main_loop_is_running(glib_main_loop_))
    g_main_loop_quit(glib_main_loop_);

  if (glib_thread_) {
    glib_thread_->cancel();
    glib_thread_->wait();
    glib_thread_ = nullptr;

    g_main_loop_unref(glib_main_loop_);
    glib_main_loop_ = nullptr;
  }

  if (session_) {
    XdpSession *parent_session = xdp_input_capture_session_get_session(session_);
    g_signal_handler_disconnect(G_OBJECT(parent_session), signals_[SESSION_CLOSED]);
    g_signal_handler_disconnect(session_, signals_[DISABLED]);
    g_signal_handler_disconnect(session_, signals_[ACTIVATED]);
    g_signal_handler_disconnect(session_, signals_[DEACTIVATED]);
    g_signal_handler_disconnect(session_, signals_[ZONES_CHANGED]);
    g_object_unref(session_);
  }

  for (auto b : barriers_) {
    g_object_unref(b);
  }
  barriers_.clear();
  g_object_unref(portal_);
}

gboolean PortalInputCapture::timeout_handler()
{
  return true; // keep re-triggering
}

int PortalInputCapture::fake_eis_fd()
{
  auto path = std::getenv("LIBEI_SOCKET");

  if (!path) {
    LOG_DEBUG("cannot fake eis socket, env var not set: LIBEI_SOCKET");
    return -1;
  }

  auto sock = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0);

  // Dealing with the socket directly because nothing in lib/... supports
  // AF_UNIX and I'm too lazy to fix all this for a temporary hack
  int fd = sock;
  struct sockaddr_un addr = {
      .sun_family = AF_UNIX,
      .sun_path = {0},
  };
  std::snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", path);

  auto result = connect(fd, (struct sockaddr *)&addr, sizeof(addr));
  if (result != 0) {
    LOG_DEBUG("faked eis fd failed: %s", strerror(errno));
  }

  return sock;
}

void PortalInputCapture::cb_session_closed(XdpSession *session)
{
  LOG_ERR("portal input capture session was closed, exiting");
  g_main_loop_quit(glib_main_loop_);
  events_->addEvent(Event::kQuit);

  g_signal_handler_disconnect(session, signals_[SESSION_CLOSED]);
  signals_[SESSION_CLOSED] = 0;
}

void PortalInputCapture::cb_init_input_capture_session(GObject *object, GAsyncResult *res)
{
  LOG_DEBUG("portal input capture session initialized");
  g_autoptr(GError) error = nullptr;

  auto session = xdp_portal_create_input_capture_session_finish(XDP_PORTAL(object), res, &error);
  if (!session) {
    LOG_ERR("failed to initialize input capture session, quitting: %s", error->message);
    g_main_loop_quit(glib_main_loop_);
    events_->addEvent(Event::kQuit);
    return;
  }

  session_ = session;

  auto fd = xdp_input_capture_session_connect_to_eis(session, &error);
  if (fd < 0) {
    LOG_ERR("failed to connect to eis: %s", error->message);

    // FIXME: Development hack to avoid having to assemble all parts just for
    // testing this code.
    fd = fake_eis_fd();

    if (fd < 0) {
      g_main_loop_quit(glib_main_loop_);
      events_->addEvent(Event::kQuit);
      return;
    }
  }
  // Socket ownership is transferred to the EiScreen
  events_->addEvent(Event(events_->forEi().connected(), screen_->getEventTarget(), EiScreen::EiConnectInfo::alloc(fd)));

  // FIXME: the lambda trick doesn't work here for unknown reasons, we need
  // the static function
  signals_[DISABLED] = g_signal_connect(G_OBJECT(session), "disabled", G_CALLBACK(cb_disabled_cb), this);
  signals_[ACTIVATED] = g_signal_connect(G_OBJECT(session_), "activated", G_CALLBACK(cb_activated_cb), this);
  signals_[DEACTIVATED] = g_signal_connect(G_OBJECT(session_), "deactivated", G_CALLBACK(cb_deactivated_cb), this);
  signals_[ZONES_CHANGED] =
      g_signal_connect(G_OBJECT(session_), "zones-changed", G_CALLBACK(cb_zones_changed_cb), this);
  XdpSession *parent_session = xdp_input_capture_session_get_session(session);
  signals_[SESSION_CLOSED] =
      g_signal_connect(G_OBJECT(parent_session), "closed", G_CALLBACK(cb_session_closed_cb), this);

  cb_zones_changed(session_, nullptr);
}

void PortalInputCapture::cb_set_pointer_barriers(GObject *object, GAsyncResult *res)
{
  g_autoptr(GError) error = nullptr;

  auto failed_list = xdp_input_capture_session_set_pointer_barriers_finish(session_, res, &error);
  if (failed_list) {
    auto it = failed_list;
    while (it) {
      guint id;
      g_object_get(it->data, "id", &id, nullptr);

      for (auto elem = barriers_.begin(); elem != barriers_.end(); elem++) {
        if (*elem == it->data) {
          int x1, x2, y1, y2;

          g_object_get(G_OBJECT(*elem), "x1", &x1, "x2", &x2, "y1", &y1, "y2", &y2, nullptr);

          LOG_WARN("failed to apply barrier %d (%d/%d-%d/%d)", id, x1, y1, x2, y2);
          g_object_unref(*elem);
          barriers_.erase(elem);
          break;
        }
      }
      it = it->next;
    }
  }
  g_list_free_full(failed_list, g_object_unref);

  enable();
}

gboolean PortalInputCapture::init_input_capture_session()
{
  LOG_DEBUG("setting up input capture session");
  xdp_portal_create_input_capture_session(
      portal_,
      nullptr, // parent
      static_cast<XdpInputCapability>(XDP_INPUT_CAPABILITY_KEYBOARD | XDP_INPUT_CAPABILITY_POINTER),
      nullptr, // cancellable
      [](GObject *obj, GAsyncResult *res, gpointer data) {
        reinterpret_cast<PortalInputCapture *>(data)->cb_init_input_capture_session(obj, res);
      },
      this
  );

  return false;
}

void PortalInputCapture::enable()
{
  if (!enabled_) {
    LOG_DEBUG("enabling the portal input capture session");
    xdp_input_capture_session_enable(session_);
    enabled_ = true;
  }
}

void PortalInputCapture::disable()
{
  if (enabled_) {
    LOG_DEBUG("disabling the portal input capture session");
    xdp_input_capture_session_disable(session_);
    enabled_ = false;
  }
}

void PortalInputCapture::release()
{
  LOG_DEBUG("releasing input capture session, id=%d", activation_id_);
  xdp_input_capture_session_release(session_, activation_id_);
  is_active_ = false;
}

void PortalInputCapture::release(double x, double y)
{
  LOG_DEBUG("releasing input capture session, id=%d x=%.1f y=%.1f", activation_id_, x, y);
  xdp_input_capture_session_release_at(session_, activation_id_, x, y);
  is_active_ = false;
}

void PortalInputCapture::cb_disabled(XdpInputCaptureSession *session, GVariant *option)
{
  LOG_DEBUG("portal cb disabled");

  if (!enabled_)
    return; // Nothing to do

  enabled_ = false;
  is_active_ = false;

  // FIXME: need some better heuristics here of when we want to enable again
  // But we don't know *why* we got disabled (and it's doubtfull we ever
  // will), so we just assume that the zones will change or something and we
  // can re-enable again
  // ... very soon
  g_timeout_add(
      1000,
      [](gpointer data) -> gboolean {
        reinterpret_cast<PortalInputCapture *>(data)->enable();
        return false;
      },
      this
  );
}

void PortalInputCapture::cb_activated(XdpInputCaptureSession *session, std::uint32_t activation_id, GVariant *options)
{
  LOG_DEBUG("portal cb activated, id=%d", activation_id);

  if (options) {
    gdouble x, y;
    if (g_variant_lookup(options, "cursor_position", "(dd)", &x, &y)) {
      screen_->warpCursor((int)x, (int)y);
    } else {
      LOG_WARN("failed to get cursor position");
    }
  } else {
    LOG_WARN("activation has no options");
  }
  activation_id_ = activation_id;
  is_active_ = true;
}

void PortalInputCapture::cb_deactivated(XdpInputCaptureSession *session, std::uint32_t activation_id, GVariant *options)
{
  LOG_DEBUG("cb deactivated, id=%i", activation_id);
  is_active_ = false;
}

void PortalInputCapture::cb_zones_changed(XdpInputCaptureSession *session, GVariant *options)
{
  for (auto b : barriers_)
    g_object_unref(b);
  barriers_.clear();

  auto zones = xdp_input_capture_session_get_zones(session);
  while (zones != nullptr) {
    guint w, h;
    gint x, y;
    g_object_get(zones->data, "width", &w, "height", &h, "x", &x, "y", &y, nullptr);

    LOG_DEBUG("input capture zone, %dx%d@%d,%d", w, h, x, y);

    int x1, x2, y1, y2;

    // Hardcoded behaviour: our pointer barriers are always at the edges of
    // all zones. Since the implementation is supposed to reject the ones in
    // the wrong place, we can just install barriers everywhere and let EIS
    // figure it out. Also a lot easier to implement for now though it doesn't
    // cover differently-sized screens...
    auto id = barriers_.size() + 1;
    x1 = x;
    y1 = y;
    x2 = x + w - 1;
    y2 = y;
    LOG_DEBUG("barrier (top) %zd at %d,%d-%d,%d", id, x1, y1, x2, y2);
    barriers_.push_back(XDP_INPUT_CAPTURE_POINTER_BARRIER(
        g_object_new(XDP_TYPE_INPUT_CAPTURE_POINTER_BARRIER, "id", id, "x1", x1, "y1", y1, "x2", x2, "y2", y2, nullptr)
    ));
    id = barriers_.size() + 1;
    x1 = x + w;
    y1 = y;
    x2 = x + w;
    y2 = y + h - 1;
    LOG_DEBUG("barrier (right) %zd at %d,%d-%d,%d", id, x1, y1, x2, y2);
    barriers_.push_back(XDP_INPUT_CAPTURE_POINTER_BARRIER(
        g_object_new(XDP_TYPE_INPUT_CAPTURE_POINTER_BARRIER, "id", id, "x1", x1, "y1", y1, "x2", x2, "y2", y2, nullptr)
    ));
    id = barriers_.size() + 1;
    x1 = x;
    y1 = y;
    x2 = x;
    y2 = y + h - 1;
    LOG_DEBUG("barrier (left) %zd at %d,%d-%d,%d", id, x1, y1, x2, y2);
    barriers_.push_back(XDP_INPUT_CAPTURE_POINTER_BARRIER(
        g_object_new(XDP_TYPE_INPUT_CAPTURE_POINTER_BARRIER, "id", id, "x1", x1, "y1", y1, "x2", x2, "y2", y2, nullptr)
    ));
    id = barriers_.size() + 1;
    x1 = x;
    y1 = y + h;
    x2 = x + w - 1;
    y2 = y + h;
    LOG_DEBUG("barrier (bottom) %zd at %d,%d-%d,%d", id, x1, y1, x2, y2);
    barriers_.push_back(XDP_INPUT_CAPTURE_POINTER_BARRIER(
        g_object_new(XDP_TYPE_INPUT_CAPTURE_POINTER_BARRIER, "id", id, "x1", x1, "y1", y1, "x2", x2, "y2", y2, nullptr)
    ));
    zones = zones->next;
  }

  GList *list = nullptr;
  for (auto const &b : barriers_) {
    list = g_list_append(list, b);
  }

  xdp_input_capture_session_set_pointer_barriers(
      session_, list,
      nullptr, // cancellable
      [](GObject *obj, GAsyncResult *res, gpointer data) {
        reinterpret_cast<PortalInputCapture *>(data)->cb_set_pointer_barriers(obj, res);
      },
      this
  );
}

void PortalInputCapture::glib_thread(void *)
{
  auto context = g_main_loop_get_context(glib_main_loop_);

  LOG_DEBUG("glib thread running");

  while (g_main_loop_is_running(glib_main_loop_)) {
    Thread::testCancel();
    g_main_context_iteration(context, true);
  }

  LOG_DEBUG("shutting down glib thread");
}

} // namespace deskflow
