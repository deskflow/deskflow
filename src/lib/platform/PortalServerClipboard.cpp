/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/PortalServerClipboard.h"

#include "base/Log.h"
#include "base/TMethodJob.h"
#include "common/Settings.h"
#include "platform/EiScreen.h"
#include "platform/PortalClipboard.h"

#include <cstdlib>
#include <cstring>

namespace deskflow {

namespace {

bool envIsOff(const char *value)
{
  return g_ascii_strcasecmp(value, "0") == 0 || g_ascii_strcasecmp(value, "false") == 0 ||
         g_ascii_strcasecmp(value, "off") == 0 || g_ascii_strcasecmp(value, "no") == 0;
}

} // namespace

bool PortalServerClipboard::isEnabled()
{
  const char *value = std::getenv("DESKFLOW_SERVER_CLIPBOARD_PORTAL");
  if (value == nullptr || *value == '\0')
    return true;

  if (envIsOff(value)) {
    LOG_INFO("server clipboard portal disabled by DESKFLOW_SERVER_CLIPBOARD_PORTAL");
    return false;
  }

  return true;
}

PortalServerClipboard::DeviceMode PortalServerClipboard::deviceMode()
{
  const char *value = std::getenv("DESKFLOW_SERVER_CLIPBOARD_DEVICES");
  if (value != nullptr && g_ascii_strcasecmp(value, "none") == 0)
    return DeviceMode::None;
  return DeviceMode::Input;
}

PortalServerClipboard::PortalServerClipboard(EiScreen *screen) : m_screen{screen}
{
  // A private main context, so this object's D-Bus dispatch never contends with
  // the input-capture glib loop for the global default context.
  m_glibContext = g_main_context_new();
  m_glibMainLoop = g_main_loop_new(m_glibContext, true);
  m_cancellable = g_cancellable_new();

  auto *tMethodJob = new TMethodJob<PortalServerClipboard>(this, &PortalServerClipboard::glibThread);
  m_glibThread = new Thread(tMethodJob);

  scheduleInit(0);
}

PortalServerClipboard::~PortalServerClipboard()
{
  m_shuttingDown.store(true, std::memory_order_release);
  m_ready.store(false, std::memory_order_release);

  // Cancel any in-flight portal call before we stop iterating, so its GTask
  // completes instead of sitting on the bus.
  if (m_cancellable)
    g_cancellable_cancel(m_cancellable);

  if (m_glibMainLoop && g_main_loop_is_running(m_glibMainLoop))
    g_main_loop_quit(m_glibMainLoop);

  if (m_glibContext)
    g_main_context_wakeup(m_glibContext);

  if (m_glibThread) {
    m_glibThread->cancel();
    m_glibThread->wait();
    delete m_glibThread;
    m_glibThread = nullptr;
  }

  // The glib thread is joined; taking the claim lock here waits out any
  // claimClipboard() call the event thread started before m_ready went false.
  {
    std::scoped_lock lock{m_claimMutex};

    disconnectSignals();

    if (m_session) {
      g_object_unref(m_session);
      m_session = nullptr;
    }
    if (m_portal) {
      g_object_unref(m_portal);
      m_portal = nullptr;
    }
    if (m_cancellable) {
      g_object_unref(m_cancellable);
      m_cancellable = nullptr;
    }
    if (m_glibMainLoop) {
      g_main_loop_unref(m_glibMainLoop);
      m_glibMainLoop = nullptr;
    }
    if (m_glibContext) {
      g_main_context_unref(m_glibContext);
      m_glibContext = nullptr;
    }
  }

  free(m_sessionRestoreToken);
  m_sessionRestoreToken = nullptr;
}

void PortalServerClipboard::disconnectSignals()
{
  if (!m_session)
    return;

  if (m_sessionClosedSignalId) {
    g_signal_handler_disconnect(m_session, m_sessionClosedSignalId);
    m_sessionClosedSignalId = 0;
  }
  if (m_selectionTransferSignalId) {
    g_signal_handler_disconnect(m_session, m_selectionTransferSignalId);
    m_selectionTransferSignalId = 0;
  }
  if (m_selectionOwnerChangedSignalId) {
    g_signal_handler_disconnect(m_session, m_selectionOwnerChangedSignalId);
    m_selectionOwnerChangedSignalId = 0;
  }
}

void PortalServerClipboard::glibThread(const void *)
{
  // Push before anything creates the XdpPortal: GDBus binds signal dispatch to
  // whatever context is thread-default at subscribe time, and every subscribe
  // this object makes happens from inside this loop.
  g_main_context_push_thread_default(m_glibContext);

  while (g_main_loop_is_running(m_glibMainLoop)) {
    Thread::testCancel();
    g_main_context_iteration(m_glibContext, true);
  }

  g_main_context_pop_thread_default(m_glibContext);
}

void PortalServerClipboard::scheduleInit(unsigned int delayMs)
{
  if (m_shuttingDown.load(std::memory_order_acquire) || m_gaveUp)
    return;

  GSource *source = delayMs > 0 ? g_timeout_source_new(delayMs) : g_idle_source_new();
  g_source_set_callback(
      source,
      [](gpointer data) -> gboolean {
        return static_cast<PortalServerClipboard *>(data)->initSession();
      },
      this, nullptr
  );
  g_source_attach(source, m_glibContext);
  g_source_unref(source);
}

void PortalServerClipboard::giveUp(const char *why)
{
  m_gaveUp = true;
  m_ready.store(false, std::memory_order_release);
  LOG_WARN(
      "server clipboard portal unavailable (%s); clipboard sharing from this server is disabled, "
      "input capture is unaffected",
      why
  );
}

void PortalServerClipboard::failAndRetry(const char *why)
{
  m_ready.store(false, std::memory_order_release);

  if (m_shuttingDown.load(std::memory_order_acquire))
    return;

  if (m_initAttempts >= kMaxInitAttempts) {
    giveUp(why);
    return;
  }

  unsigned int delay = kFirstRetryDelayMs;
  for (unsigned int i = 0; i < m_consecutiveFailures && delay < kMaxRetryDelayMs; ++i)
    delay *= 2;
  if (delay > kMaxRetryDelayMs)
    delay = kMaxRetryDelayMs;

  ++m_consecutiveFailures;
  LOG_DEBUG("server clipboard portal retry in %u ms (%s)", delay, why);
  scheduleInit(delay);
}

gboolean PortalServerClipboard::initSession()
{
  if (m_shuttingDown.load(std::memory_order_acquire) || m_gaveUp)
    return false; // don't reschedule

  if (m_initAttempts >= kMaxInitAttempts) {
    giveUp("retry limit reached");
    return false;
  }
  ++m_initAttempts;

  if (!m_portal) {
    g_autoptr(GError) portalError = nullptr;
    // Deliberately not xdp_portal_new(): that abort()s the process when the
    // session bus is missing, which would take the KVM down with it.
    m_portal = xdp_portal_initable_new(&portalError);
    if (!m_portal) {
      giveUp(portalError ? portalError->message : "cannot reach the session bus");
      return false;
    }
  }

  if (auto sessionToken = Settings::value(Settings::Server::XdpClipboardRestoreToken).toByteArray();
      !sessionToken.isEmpty()) {
    free(m_sessionRestoreToken);
    m_sessionRestoreToken = strdup(sessionToken.data());
  }

  const auto devices = deviceMode() == DeviceMode::None
                           ? XDP_DEVICE_NONE
                           : static_cast<XdpDeviceType>(XDP_DEVICE_POINTER | XDP_DEVICE_KEYBOARD);

  LOG_DEBUG(
      "setting up server clipboard portal session (attempt %u, devices=0x%x, restore token %s)", m_initAttempts,
      static_cast<unsigned int>(devices), m_sessionRestoreToken ? "present" : "none"
  );

  xdp_portal_create_remote_desktop_session_full(
      m_portal, devices, XDP_OUTPUT_NONE, XDP_REMOTE_DESKTOP_FLAG_NONE, XDP_CURSOR_MODE_HIDDEN,
      XDP_PERSIST_MODE_PERSISTENT, m_sessionRestoreToken, m_cancellable,
      [](GObject *obj, GAsyncResult *res, gpointer data) {
        static_cast<PortalServerClipboard *>(data)->handleInitSession(obj, res);
      },
      this
  );

  return false; // don't reschedule
}

void PortalServerClipboard::handleInitSession(GObject *object, GAsyncResult *res)
{
  g_autoptr(GError) error = nullptr;

  auto *session = xdp_portal_create_remote_desktop_session_finish(XDP_PORTAL(object), res, &error);
  if (!session) {
    if (g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
      // Either we are shutting down, or the user dismissed the portal dialog.
      // Neither is worth hammering the portal over.
      if (!m_shuttingDown.load(std::memory_order_acquire))
        giveUp("session request was cancelled or denied");
      return;
    }
    failAndRetry(error ? error->message : "failed to create session");
    return;
  }

  if (m_shuttingDown.load(std::memory_order_acquire)) {
    g_object_unref(session);
    return;
  }

  m_session = session;

  m_sessionClosedSignalId =
      g_signal_connect(G_OBJECT(session), "closed", G_CALLBACK(handleSessionClosedCallback), this);

  // Must happen before Start: the portal reports clipboard_enabled in the Start
  // reply, and RequestClipboard is only honoured on a session in initial state.
  xdp_session_request_clipboard(session);
  m_selectionTransferSignalId =
      g_signal_connect(G_OBJECT(session), "selection-transfer", G_CALLBACK(selectionTransferCallback), this);
  m_selectionOwnerChangedSignalId =
      g_signal_connect(G_OBJECT(session), "selection-owner-changed", G_CALLBACK(selectionOwnerChangedCallback), this);

  LOG_DEBUG("server clipboard portal session starting");
  xdp_session_start(
      session,
      nullptr, // parent
      m_cancellable,
      [](GObject *obj, GAsyncResult *result, gpointer data) {
        static_cast<PortalServerClipboard *>(data)->handleSessionStarted(obj, result);
      },
      this
  );
}

void PortalServerClipboard::handleSessionStarted(GObject *object, GAsyncResult *res)
{
  g_autoptr(GError) error = nullptr;
  auto *session = XDP_SESSION(object);

  if (!xdp_session_start_finish(session, res, &error)) {
    disconnectSignals();
    g_clear_object(&m_session);
    if (g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
      if (!m_shuttingDown.load(std::memory_order_acquire))
        giveUp("session start was cancelled or denied");
      return;
    }
    failAndRetry(error ? error->message : "failed to start session");
    return;
  }

  if (m_shuttingDown.load(std::memory_order_acquire))
    return;

  if (!xdp_session_is_clipboard_enabled(session)) {
    disconnectSignals();
    g_clear_object(&m_session);

    if (m_discardedTokenOnce) {
      giveUp("compositor started the session without a clipboard");
      return;
    }

    // A restore token minted before clipboard support can come back without a
    // clipboard. Drop it once and ask for a fresh session.
    LOG_DEBUG("server clipboard portal session has no clipboard, discarding restore token and retrying once");
    m_discardedTokenOnce = true;
    Settings::setValue(Settings::Server::XdpClipboardRestoreToken, QString());
    free(m_sessionRestoreToken);
    m_sessionRestoreToken = nullptr;
    scheduleInit(kFirstRetryDelayMs);
    return;
  }

  free(m_sessionRestoreToken);
  m_sessionRestoreToken = xdp_session_get_restore_token(session);
  if (m_sessionRestoreToken)
    Settings::setValue(Settings::Server::XdpClipboardRestoreToken, QString(m_sessionRestoreToken));

  // Deliberately NOT calling xdp_session_connect_to_eis() here. This session
  // exists only to carry the clipboard; the server's ei context is a receiver
  // already bound to the input-capture socket, and handing it a second fd would
  // break input capture.

  m_consecutiveFailures = 0;
  m_ready.store(true, std::memory_order_release);
  LOG_INFO("server clipboard portal session ready, clipboard enabled");

  // Publish whatever the screen is already holding (for example a clipboard a
  // client pushed while the session was still coming up).
  claimClipboard();
}

void PortalServerClipboard::handleSessionClosed(XdpSession *session)
{
  m_ready.store(false, std::memory_order_release);

  if (m_sessionClosedSignalId) {
    g_signal_handler_disconnect(session, m_sessionClosedSignalId);
    m_sessionClosedSignalId = 0;
  }
  if (m_selectionTransferSignalId) {
    g_signal_handler_disconnect(session, m_selectionTransferSignalId);
    m_selectionTransferSignalId = 0;
  }
  if (m_selectionOwnerChangedSignalId) {
    g_signal_handler_disconnect(session, m_selectionOwnerChangedSignalId);
    m_selectionOwnerChangedSignalId = 0;
  }

  // Note: no EISessionClosed and no Quit. Input capture owns its own session and
  // must not notice this at all.
  LOG_DEBUG("server clipboard portal session closed, will retry");

  g_clear_object(&m_session);

  if (m_shuttingDown.load(std::memory_order_acquire) || m_gaveUp)
    return;

  if (m_initAttempts >= kMaxInitAttempts) {
    giveUp("session kept closing");
    return;
  }
  scheduleInit(kSessionClosedRetryDelayMs);
}

void PortalServerClipboard::claimClipboard() const
{
  std::scoped_lock lock{m_claimMutex};

  if (!isReady() || m_glibContext == nullptr)
    return;

  // m_session is created and destroyed on the glib thread only, so do the work
  // there. When we are already on that thread (the post-Start claim) glib runs
  // this inline.
  g_main_context_invoke(
      m_glibContext,
      [](gpointer data) -> gboolean {
        static_cast<PortalServerClipboard *>(data)->claimClipboardOnGlibThread();
        return G_SOURCE_REMOVE;
      },
      const_cast<PortalServerClipboard *>(this)
  );
}

void PortalServerClipboard::claimClipboardOnGlibThread()
{
  if (m_shuttingDown.load(std::memory_order_acquire) || !m_session)
    return;

  if (!xdp_session_is_clipboard_enabled(m_session))
    return;

  PortalClipboard::claimOwnership(m_screen->getClipboardCache(), m_session);
}

void PortalServerClipboard::handleSelectionTransfer(XdpSession *session, const char *mimeType, uint32_t serial) const
{
  PortalClipboard::serveSelectionTransfer(m_screen->getClipboardCache(), session, mimeType, serial);
}

void PortalServerClipboard::handleSelectionOwnerChanged(XdpSession *session, char **mimeTypes, gboolean isOwner) const
{
  if (isOwner) {
    LOG_DEBUG("server clipboard portal selection owner changed, we own it, ignoring");
    return;
  }

  const qint64 maxBytes = static_cast<qint64>(m_screen->maximumClipboardSize()) * 1024;
  if (PortalClipboard::readSelectionIntoCache(m_screen->getClipboardCache(), session, mimeTypes, maxBytes))
    m_screen->sendClipboardEvent(EventTypes::ClipboardGrabbed, kClipboardClipboard);
}

} // namespace deskflow
