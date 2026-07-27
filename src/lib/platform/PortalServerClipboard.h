/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "mt/Thread.h"

#include <atomic>
#include <glib.h>
#include <libportal/portal.h>
#include <mutex>

namespace deskflow {

class EiScreen;

//! Clipboard-only RemoteDesktop portal session for the deskflow server.
/*!
The server drives input through an InputCapture portal session. On GNOME that
session can never carry a clipboard: xdg-desktop-portal-gnome only accepts
org.freedesktop.portal.Clipboard.RequestClipboard on a RemoteDesktop session,
and at InputCapture portal version 1 libportal does not even ask. The result is
a server that reads an always-empty clipboard and broadcasts it to its clients.

This class opens a *second*, completely independent portal session whose only
job is to carry the clipboard. It reuses the already-proven PortalClipboard
helpers, which are session-type agnostic.

Everything here is strictly additive and strictly optional. The safety
properties are structural, not incidental:

  - It holds no IEventQueue pointer, so it is not able to post EventTypes::Quit
    or EventTypes::EISessionClosed. A denied, failed or closed clipboard session
    can never terminate the server or tear down the EI context.
  - It never calls xdp_session_connect_to_eis() and never posts
    EventTypes::EIConnected, so it can never reach
    EiScreen::handleConnectedToEisEvent() and clobber the EIS backend fd that
    input capture owns.
  - It runs its own GMainContext on its own thread, pushed as the thread-default
    before the XdpPortal is created, so its D-Bus signal dispatch does not
    contend with the input-capture glib loop on the global default context.
  - It uses xdp_portal_initable_new() rather than xdp_portal_new(), which
    abort()s the process when the session bus is unavailable.
  - It uses its own settings key for its restore token, so it cannot clobber the
    input-capture token or a client's remote-desktop token.
  - Retries are bounded and backed off; after the cap it gives up permanently
    and stays quiet.
  - isReady() stays false until the portal has actually granted a session *and*
    reported clipboard_enabled. Every caller falls back to the stock code path
    while it is false, so a failure is indistinguishable from not building this
    class at all.
*/
class PortalServerClipboard
{
public:
  explicit PortalServerClipboard(EiScreen *screen);
  ~PortalServerClipboard();

  PortalServerClipboard(const PortalServerClipboard &) = delete;
  PortalServerClipboard &operator=(const PortalServerClipboard &) = delete;

  //! True once the portal granted a session whose clipboard is enabled.
  /*!
  Safe to call from any thread. While this is false the screen must keep using
  the stock clipboard paths.
  */
  bool isReady() const
  {
    return m_ready.load(std::memory_order_acquire);
  }

  //! Advertise the screen's clipboard cache as the local selection.
  /*!
  Callable from the event thread. The work is bounced onto this object's own
  glib thread, because the portal session handle is only ever created and
  destroyed there.
  */
  void claimClipboard() const;

  //! Kill switch, read once at construction time.
  /*!
  Returns false when DESKFLOW_SERVER_CLIPBOARD_PORTAL is set to 0/false/off/no.
  Anything else (including unset) enables the feature.
  */
  static bool isEnabled();

private:
  void glibThread(const void *);
  void scheduleInit(unsigned int delayMs);
  void giveUp(const char *why);
  void failAndRetry(const char *why);

  gboolean initSession();
  void claimClipboardOnGlibThread();
  void handleInitSession(GObject *object, GAsyncResult *res);
  void handleSessionStarted(GObject *object, GAsyncResult *res);
  void handleSessionClosed(XdpSession *session);
  void handleSelectionTransfer(XdpSession *session, const char *mimeType, uint32_t serial) const;
  void handleSelectionOwnerChanged(XdpSession *session, char **mimeTypes, gboolean isOwner) const;
  void disconnectSignals();

  static void handleSessionClosedCallback(XdpSession *session, gpointer data)
  {
    static_cast<PortalServerClipboard *>(data)->handleSessionClosed(session);
  }

  static void selectionTransferCallback(XdpSession *session, const char *mimeType, uint32_t serial, gpointer data)
  {
    static_cast<PortalServerClipboard *>(data)->handleSelectionTransfer(session, mimeType, serial);
  }

  static void selectionOwnerChangedCallback(XdpSession *session, char **mimeTypes, gboolean isOwner, gpointer data)
  {
    static_cast<PortalServerClipboard *>(data)->handleSelectionOwnerChanged(session, mimeTypes, isOwner);
  }

  //! Devices asked for in the portal dialog.
  /*!
  Defaults to Input (pointer + keyboard), which is byte for byte the request the
  deskflow *client* already makes successfully on this compositor. Set
  DESKFLOW_SERVER_CLIPBOARD_DEVICES=none to ask for no devices at all, which is
  a lighter touch on the seat but is not a path any shipping deskflow code
  exercises.
  */
  enum class DeviceMode : std::uint8_t
  {
    Input,
    None
  };
  static DeviceMode deviceMode();

  // Total init attempts across the whole process lifetime. Hard bound so a
  // compositor that keeps refusing or closing the session cannot produce an
  // endless stream of portal dialogs or log spam.
  static constexpr unsigned int kMaxInitAttempts = 12;
  static constexpr unsigned int kFirstRetryDelayMs = 2000;
  static constexpr unsigned int kMaxRetryDelayMs = 60000;
  static constexpr unsigned int kSessionClosedRetryDelayMs = 5000;

  EiScreen *m_screen = nullptr;

  Thread *m_glibThread = nullptr;
  GMainContext *m_glibContext = nullptr;
  GMainLoop *m_glibMainLoop = nullptr;
  GCancellable *m_cancellable = nullptr;

  XdpPortal *m_portal = nullptr;
  XdpSession *m_session = nullptr;
  char *m_sessionRestoreToken = nullptr;

  gulong m_sessionClosedSignalId = 0;
  gulong m_selectionTransferSignalId = 0;
  gulong m_selectionOwnerChangedSignalId = 0;

  // Written on the glib thread, read from the event thread.
  std::atomic<bool> m_ready{false};
  std::atomic<bool> m_shuttingDown{false};

  // Serialises claimClipboard() against teardown so the event thread can never
  // hand work to a main context that the destructor has already unreffed.
  mutable std::mutex m_claimMutex;

  // glib-thread only.
  unsigned int m_initAttempts = 0;
  unsigned int m_consecutiveFailures = 0;
  bool m_gaveUp = false;
  bool m_discardedTokenOnce = false;
};

} // namespace deskflow
