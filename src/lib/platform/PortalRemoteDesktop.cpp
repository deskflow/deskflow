/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Applone <app1oneme@gmail.com>
 * SPDX-FileCopyrightText: (C) 2024 - 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2024, 2026 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2022 Red Hat, Inc.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/PortalRemoteDesktop.h"

#include "base/Log.h"
#include "common/Settings.h"

#ifdef HAVE_LIBPORTAL_CLIPBOARD
#include "platform/PortalClipboard.h"
#endif

namespace deskflow {

PortalRemoteDesktop::PortalRemoteDesktop(EiComputer *computer, IEventQueue *events)
    : m_computer{computer},
      m_events{events},
      m_portal{m_context.portal()}
{
  m_context.start();
  m_context.defer([this] { reconnect(0); });
}

PortalRemoteDesktop::~PortalRemoteDesktop()
{
  m_context.stop();
#ifdef HAVE_LIBPORTAL_CLIPBOARD
  m_selectionRead.cancel();
#endif
  if (m_sessionSignalId)
    g_signal_handler_disconnect(m_session, m_sessionSignalId);
  if (m_selectionTransferSignalId)
    g_signal_handler_disconnect(m_session, m_selectionTransferSignalId);
  if (m_selectionOwnerChangedSignalId)
    g_signal_handler_disconnect(m_session, m_selectionOwnerChangedSignalId);
  if (m_session)
    g_object_unref(m_session);

  free(m_sessionRestoreToken);
}

void PortalRemoteDesktop::reconnect(unsigned int timeout)
{
  g_source_unref(m_context.post([this] { initSession(); }, timeout));
}

void PortalRemoteDesktop::handleSessionClosed(XdpSession *session)
{
#ifdef HAVE_LIBPORTAL_CLIPBOARD
  m_selectionRead.cancel();
#endif

  LOG_ERR("portal remote desktop session was closed, reconnecting");
  g_signal_handler_disconnect(session, m_sessionSignalId);
  m_sessionSignalId = 0;
  if (m_selectionTransferSignalId)
    g_signal_handler_disconnect(session, m_selectionTransferSignalId);
  if (m_selectionOwnerChangedSignalId)
    g_signal_handler_disconnect(session, m_selectionOwnerChangedSignalId);
  m_selectionTransferSignalId = 0;
  m_selectionOwnerChangedSignalId = 0;
  m_events->addEvent(Event(EventTypes::EISessionClosed, m_computer->getEventTarget()));

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
    m_events->addEvent(Event(EventTypes::Quit));
    return;
  }

#ifdef HAVE_LIBPORTAL_CLIPBOARD
  if (!xdp_session_is_clipboard_enabled(session)) {
    if (Settings::value(Settings::Client::XdpClipboardRetried).toBool()) {
      // some backends never report clipboard enabled even when granted; don't loop forever
      LOG_DEBUG("clipboard still not enabled on remote desktop session after one retry, continuing without it");
    } else {
      LOG_WARN("clipboard not enabled on remote desktop session, discarding restore token to force a fresh session");
      Settings::setValue(Settings::Client::XdpRestoreToken, QString());
      Settings::setValue(Settings::Client::XdpClipboardRetried, true);
      free(m_sessionRestoreToken);
      m_sessionRestoreToken = nullptr;
      if (m_selectionTransferSignalId) {
        g_signal_handler_disconnect(session, m_selectionTransferSignalId);
        m_selectionTransferSignalId = 0;
      }
      if (m_selectionOwnerChangedSignalId) {
        g_signal_handler_disconnect(session, m_selectionOwnerChangedSignalId);
        m_selectionOwnerChangedSignalId = 0;
      }
      if (m_sessionSignalId) {
        g_signal_handler_disconnect(session, m_sessionSignalId);
        m_sessionSignalId = 0;
      }
      g_clear_object(&m_session);
      reconnect(0);
      return;
    }
  }
#endif

  free(m_sessionRestoreToken);
  m_sessionRestoreToken = xdp_session_get_restore_token(session);
  if (m_sessionRestoreToken) {
    Settings::setValue(Settings::Client::XdpRestoreToken, QString(m_sessionRestoreToken));
  }

  // ConnectToEIS requires version 2 of the xdg-desktop-portal (and the same
  // version in the impl.portal), i.e. you'll need an updated compositor on
  // top of everything...
  auto fd = -1;
  fd = xdp_session_connect_to_eis(session, &error);
  if (fd < 0) {
    m_events->addEvent(Event(EventTypes::Quit));
    return;
  }

  // Socket ownership is transferred to the EiComputer
  m_events->addEvent(Event(EventTypes::EIConnected, m_computer->getEventTarget(), EiComputer::EiConnectInfo::alloc(fd))
  );
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

#ifdef HAVE_LIBPORTAL_CLIPBOARD
  xdp_session_request_clipboard(session);
  m_selectionTransferSignalId =
      g_signal_connect(G_OBJECT(session), "selection-transfer", G_CALLBACK(selectionTransferCallback), this);
  m_selectionOwnerChangedSignalId =
      g_signal_connect(G_OBJECT(session), "selection-owner-changed", G_CALLBACK(selectionOwnerChangedCallback), this);
#endif

  LOG_DEBUG("portal remote desktop session starting");
  xdp_session_start(
      session,
      nullptr,                 // parent
      m_context.cancellable(), // cancellable
      PortalContext::complete,
      m_context.callback([this](GObject *obj, GAsyncResult *res) { handleSessionStarted(obj, res); })
  );
}

gboolean PortalRemoteDesktop::initSession()
{
  if (auto sessionToken = Settings::value(Settings::Client::XdpRestoreToken).toByteArray(); !sessionToken.isEmpty()) {
    free(m_sessionRestoreToken);
    m_sessionRestoreToken = strdup(sessionToken.data());
  }

  LOG_DEBUG("setting up remote desktop session with restore token %s", m_sessionRestoreToken);
  xdp_portal_create_remote_desktop_session_full(
      m_portal, static_cast<XdpDeviceType>(XDP_DEVICE_POINTER | XDP_DEVICE_KEYBOARD), XDP_OUTPUT_NONE,
      XDP_REMOTE_DESKTOP_FLAG_NONE, XDP_CURSOR_MODE_HIDDEN, XDP_PERSIST_MODE_PERSISTENT, m_sessionRestoreToken,
      m_context.cancellable(), // cancellable
      PortalContext::complete,
      m_context.callback([this](GObject *obj, GAsyncResult *res) { handleInitSession(obj, res); })
  );

  return false; // don't reschedule
}

void PortalRemoteDesktop::claimClipboard() const
{
  m_context.defer([this] {
#ifdef HAVE_LIBPORTAL_CLIPBOARD
    if (!m_session) {
      LOG_DEBUG("portal remote desktop clipboard claim deferred, no session yet");
      return;
    }
    if (!xdp_session_is_clipboard_enabled(m_session)) {
      LOG_DEBUG("portal remote desktop clipboard not enabled on session, cannot claim");
      return;
    }
    PortalClipboard::claimOwnership(m_computer->getClipboardCache(), m_session);
#endif
  });
}

void PortalRemoteDesktop::handleSelectionTransfer(XdpSession *session, const char *mimeType, uint32_t serial) const
{
#ifdef HAVE_LIBPORTAL_CLIPBOARD
  PortalClipboard::serveSelectionTransfer(m_computer->getClipboardCache(), session, mimeType, serial);
#else
  (void)session;
  (void)mimeType;
  (void)serial;
#endif
}

void PortalRemoteDesktop::handleSelectionOwnerChanged(XdpSession *session, char **mimeTypes, gboolean isOwner)
{
#ifdef HAVE_LIBPORTAL_CLIPBOARD
  m_selectionRead.cancel();
#endif

#ifdef HAVE_LIBPORTAL_CLIPBOARD
  if (isOwner) {
    LOG_DEBUG("portal remote desktop selection owner changed, we own it, ignoring");
    return;
  }

  const qint64 maxBytes = static_cast<qint64>(m_computer->maximumClipboardSize()) * 1024;
  PortalClipboard::readSelectionAsync(
      m_selectionRead, m_computer->getClipboardCache(), session, mimeTypes, maxBytes,
      [this] { m_computer->sendClipboardEvent(EventTypes::ClipboardGrabbed, kClipboardClipboard); }
  );
#else
  (void)session;
  (void)mimeTypes;
  (void)isOwner;
#endif
}

} // namespace deskflow
