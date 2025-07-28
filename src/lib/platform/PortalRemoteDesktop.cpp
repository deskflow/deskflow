/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2022 Red Hat, Inc.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/PortalRemoteDesktop.h"
#include "base/Log.h"
#include "base/TMethodJob.h"

namespace deskflow {

PortalRemoteDesktop::PortalRemoteDesktop(EiScreen *screen, IEventQueue *events)
    : m_screen{screen},
      m_events{events},
      m_portal{xdp_portal_new()}
{
  m_glibMainLoop = g_main_loop_new(nullptr, true);

  auto tMethodJob = new TMethodJob<PortalRemoteDesktop>(this, &PortalRemoteDesktop::glibThread);
  m_glibThread = new Thread(tMethodJob);

  reconnect(0);
}

PortalRemoteDesktop::~PortalRemoteDesktop()
{
  if (g_main_loop_is_running(m_glibMainLoop))
    g_main_loop_quit(m_glibMainLoop);

  if (m_glibThread != nullptr) {
    m_glibThread->cancel();
    m_glibThread->wait();
    delete m_glibThread;
    m_glibThread = nullptr;

    g_main_loop_unref(m_glibMainLoop);
    m_glibMainLoop = nullptr;
  }

  if (m_sessionSignalId)
    g_signal_handler_disconnect(m_session, m_sessionSignalId);
  if (m_session)
    g_object_unref(m_session);
  g_object_unref(m_portal);

  free(m_sessionRestoreToken);
}

gboolean PortalRemoteDesktop::timeoutHandler() const
{
  return true; // keep re-triggering
}

void PortalRemoteDesktop::reconnect(unsigned int timeout)
{
  auto initCallback = [](gpointer data) { return static_cast<PortalRemoteDesktop *>(data)->initSession(); };

  if (timeout > 0)
    g_timeout_add(timeout, initCallback, this);
  else
    g_idle_add(initCallback, this);
}

void PortalRemoteDesktop::handleSessionClosed(XdpSession *session)
{
  LOG_ERR("portal remote desktop session was closed, reconnecting");
  g_signal_handler_disconnect(session, m_sessionSignalId);
  m_sessionSignalId = 0;
  m_events->addEvent(Event(EventTypes::EISessionClosed, m_screen->getEventTarget()));

  // gcc warning "Suspicious usage of 'sizeof(A*)'" can be ignored
  g_clear_object(&m_session);

  reconnect(1000);
}

void PortalRemoteDesktop::handleSessionStarted(GObject *object, GAsyncResult *res)
{
  g_autoptr(GError) error = nullptr;
  auto session = XDP_SESSION(object);
  if (!xdp_session_start_finish(session, res, &error)) {
    LOG_ERR("failed to start portal remote desktop session, quitting: %s", error->message);
    g_main_loop_quit(m_glibMainLoop);
    m_events->addEvent(Event(EventTypes::Quit));
    return;
  }

  m_sessionRestoreToken = xdp_session_get_restore_token(session);

  // ConnectToEIS requires version 2 of the xdg-desktop-portal (and the same
  // version in the impl.portal), i.e. you'll need an updated compositor on
  // top of everything...
  auto fd = -1;
  fd = xdp_session_connect_to_eis(session, &error);
  if (fd < 0) {
    g_main_loop_quit(m_glibMainLoop);
    m_events->addEvent(Event(EventTypes::Quit));
    return;
  }

  // Socket ownership is transferred to the EiScreen
  m_events->addEvent(Event(EventTypes::EIConnected, m_screen->getEventTarget(), EiScreen::EiConnectInfo::alloc(fd)));
}

void PortalRemoteDesktop::handleInitSession(GObject *object, GAsyncResult *res)
{
  LOG_DEBUG("portal remote desktop session initialized");
  g_autoptr(GError) error = nullptr;

  auto session = xdp_portal_create_remote_desktop_session_finish(XDP_PORTAL(object), res, &error);
  if (!session) {
    LOG_ERR("failed to initialize remote desktop session: %s", error->message);
    // This was the first attempt to connect to the RD portal - quit if that
    // fails.
    if (m_sessionIteration == 0) {
      g_main_loop_quit(m_glibMainLoop);
      m_events->addEvent(Event(EventTypes::Quit));
    } else {
      this->reconnect(1000);
    }
    return;
  }

  m_session = session;
  ++m_sessionIteration;

  // FIXME: the lambda trick doesn't work here for unknown reasons, we need
  // the static function
  m_sessionSignalId = g_signal_connect(G_OBJECT(session), "closed", G_CALLBACK(handleSessionClosedCallback), this);

  LOG_DEBUG("portal remote desktop session starting");
  xdp_session_start(
      session,
      nullptr, // parent
      nullptr, // cancellable
      [](GObject *obj, GAsyncResult *res, gpointer data) {
        static_cast<PortalRemoteDesktop *>(data)->handleSessionStarted(obj, res);
      },
      this
  );
}

gboolean PortalRemoteDesktop::initSession()
{
  LOG_DEBUG("setting up remote desktop session with restore token %s", m_sessionRestoreToken);
  xdp_portal_create_remote_desktop_session_full(
      m_portal, static_cast<XdpDeviceType>(XDP_DEVICE_POINTER | XDP_DEVICE_KEYBOARD), XDP_OUTPUT_NONE,
      XDP_REMOTE_DESKTOP_FLAG_NONE, XDP_CURSOR_MODE_HIDDEN, XDP_PERSIST_MODE_TRANSIENT, m_sessionRestoreToken,
      nullptr, // cancellable
      [](GObject *obj, GAsyncResult *res, gpointer data) {
        static_cast<PortalRemoteDesktop *>(data)->handleInitSession(obj, res);
      },
      this
  );

  return false; // don't reschedule
}

void PortalRemoteDesktop::glibThread(void *)
{
  auto context = g_main_loop_get_context(m_glibMainLoop);

  while (g_main_loop_is_running(m_glibMainLoop)) {
    Thread::testCancel();
    g_main_context_iteration(context, true);
  }
}

} // namespace deskflow
