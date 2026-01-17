/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2022 Red Hat, Inc.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/PortalRemoteDesktop.h"
#ifdef HAVE_LIBPORTAL_INPUTCAPTURE
#include "platform/PortalClipboardProxy.h"
#include "platform/PortalClipboard.h"
#endif
#include "base/Log.h"
#include "base/TMethodJob.h"
#include "common/Settings.h"

namespace deskflow {

PortalRemoteDesktop::PortalRemoteDesktop(EiScreen *screen, IEventQueue *events)
    : m_screen{screen},
      m_events{events},
      m_portal{xdp_portal_new()}
{
  m_glibMainLoop = g_main_loop_new(nullptr, true);

  auto tMethodJob = new TMethodJob<PortalRemoteDesktop>(this, &PortalRemoteDesktop::glibThread);
  m_glibThread = new Thread(tMethodJob);

  m_sessionProxy = std::make_unique<PortalSessionProxy>(PortalSessionProxy::SessionType::RemoteDesktop);
  connect(m_sessionProxy.get(), &PortalSessionProxy::sessionCreated, this, &PortalRemoteDesktop::onSessionCreated);
  connect(m_sessionProxy.get(), &PortalSessionProxy::sessionStarted, this, &PortalRemoteDesktop::onSessionStarted);

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
  auto initCallback = [](gpointer data) { 
    auto self = static_cast<PortalRemoteDesktop *>(data);
    QString token = Settings::value(Settings::Client::XdpRestoreToken).toString();
    self->m_sessionProxy->createSession(token);
    return false;
  };

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

void PortalRemoteDesktop::onSessionCreated(const QDBusObjectPath &handle)
{
  LOG_DEBUG("PortalRemoteDesktop: Session handle received: %s", handle.path().toUtf8().constData());
  
  g_autoptr(GError) error = nullptr;
  m_session = xdp_session_new_from_handle(m_portal, handle.path().toUtf8().constData(), &error);
  
  if (!m_session) {
    LOG_ERR("PortalRemoteDesktop: Failed to wrap session from handle: %s", error->message);
    m_events->addEvent(Event(EventTypes::Quit));
    return;
  }

  m_sessionSignalId = g_signal_connect(G_OBJECT(m_session), "closed", G_CALLBACK(handleSessionClosedCallback), this);

#ifdef HAVE_LIBPORTAL_INPUTCAPTURE
  initClipboard(handle);
#endif
}

void PortalRemoteDesktop::onSessionStarted(const QString &restoreToken)
{
  if (!restoreToken.isEmpty()) {
    Settings::setValue(Settings::Client::XdpRestoreToken, restoreToken);
  }

  LOG_INFO("PortalRemoteDesktop: Session started, connecting to EIS...");

  g_autoptr(GError) error = nullptr;
  auto fd = xdp_session_connect_to_eis(m_session, &error);
  if (fd < 0) {
    LOG_ERR("PortalRemoteDesktop: Failed to connect to EIS: %s", error->message);
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

#ifdef HAVE_LIBPORTAL_INPUTCAPTURE
  const char *sessionHandle = xdp_session_get_session_handle(session);
  LOG_DEBUG("portal remote desktop session started, handle: %s", sessionHandle);
  initClipboard(session);
#else
  LOG_DEBUG("portal remote desktop session started");
#endif

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
  if (auto sessionToken = Settings::value(Settings::Client::XdpRestoreToken).toByteArray(); !sessionToken.isEmpty()) {
    free(m_sessionRestoreToken);
    m_sessionRestoreToken = strdup(sessionToken.data());
  }

  LOG_DEBUG("setting up remote desktop session with restore token %s", m_sessionRestoreToken);
  xdp_portal_create_remote_desktop_session(
      m_portal, static_cast<XdpDeviceType>(XDP_DEVICE_POINTER | XDP_DEVICE_KEYBOARD), XDP_OUTPUT_NONE,
      XDP_REMOTE_DESKTOP_FLAG_NONE, XDP_CURSOR_MODE_HIDDEN, nullptr,
      [](GObject *obj, GAsyncResult *res, gpointer data) {
        static_cast<PortalRemoteDesktop *>(data)->handleInitSession(obj, res);
      },
      this
  );

  return false; // don't reschedule
}

void PortalRemoteDesktop::glibThread(const void *)
{
  auto context = g_main_loop_get_context(m_glibMainLoop);

  while (g_main_loop_is_running(m_glibMainLoop)) {
    Thread::testCancel();
    g_main_context_iteration(context, true);
  }
}

#ifdef HAVE_LIBPORTAL_INPUTCAPTURE
void PortalRemoteDesktop::initClipboard(const QDBusObjectPath &handle)
{
  LOG_DEBUG("Initializing PortalClipboardProxy for session: %s", handle.path().toUtf8().constData());

  // Create low-level DBus proxy for clipboard
  m_clipboardProxy = std::make_unique<PortalClipboardProxy>();

  if (!m_clipboardProxy->init(handle)) {
    LOG_ERR("Failed to initialize clipboard proxy");
    m_clipboardProxy.reset();
    return;
  }

  // Create High-level IClipboard implementations
  m_clipboardStandard = std::make_unique<PortalClipboard>(m_clipboardProxy.get(), PortalClipboardProxy::Standard);
  m_clipboardPrimary = std::make_unique<PortalClipboard>(m_clipboardProxy.get(), PortalClipboardProxy::Primary);
  
  connect(m_clipboardStandard.get(), &PortalClipboard::changed, this, &PortalRemoteDesktop::clipboardChanged);
  connect(m_clipboardPrimary.get(), &PortalClipboard::changed, this, &PortalRemoteDesktop::clipboardChanged);

  // Request clipboard access
  m_clipboardProxy->requestClipboard();

  LOG_INFO("Dual clipboards initialized (Standard + Primary)");
}

IClipboard *PortalRemoteDesktop::getClipboard(ClipboardID id) const
{
  if (id == kClipboardSelection) {
    return m_clipboardPrimary.get();
  }
  return m_clipboardStandard.get();
}
#else
IClipboard *PortalRemoteDesktop::getClipboard(ClipboardID) const
{
  return nullptr;
}
#endif

} // namespace deskflow
