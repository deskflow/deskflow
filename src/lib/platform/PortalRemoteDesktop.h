/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "mt/Thread.h"
#include "platform/EiScreen.h"

#include "platform/PortalClipboardProxy.h"
#include "platform/PortalSessionProxy.h"
#include <glib.h>
#include <libportal/portal.h>
#include <memory>

class IClipboard;

namespace deskflow {
class PortalClipboard;
class PortalClipboardProxy;

class PortalRemoteDesktop
{
public:
  PortalRemoteDesktop(EiScreen *screen, IEventQueue *events);
  virtual ~PortalRemoteDesktop();
  IClipboard *getClipboard(ClipboardID id) const;

Q_SIGNALS:
  void clipboardChanged();

private Q_SLOTS:
  void onSessionCreated(const QDBusObjectPath &handle);
  void onSessionStarted(const QString &restoreToken);
  void glibThread(const void *);
  gboolean timeoutHandler() const;
  gboolean initSession();
  void handleInitSession(GObject *object, GAsyncResult *res);
  void handleSessionStarted(GObject *object, GAsyncResult *res);
  void handleSessionClosed(XdpSession *session);
  void reconnect(unsigned int timeout = 1000);
#ifdef HAVE_LIBPORTAL_INPUTCAPTURE
  void initClipboard(const QDBusObjectPath &handle);
#endif

  /// g_signal_connect callback wrapper
  static void handleSessionClosedCallback(XdpSession *session, gpointer data)
  {
    static_cast<PortalRemoteDesktop *>(data)->handleSessionClosed(session);
  }

private:
  EiScreen *m_screen;
  IEventQueue *m_events;

  Thread *m_glibThread;
  GMainLoop *m_glibMainLoop = nullptr;

  XdpPortal *m_portal = nullptr;
  XdpSession *m_session = nullptr;
  char *m_sessionRestoreToken = nullptr;

  guint m_sessionSignalId = 0;

  /// The number of successful sessions we've had already
  guint m_sessionIteration = 0;

  // Session and Clipboard support
  std::unique_ptr<PortalSessionProxy> m_sessionProxy;
#ifdef HAVE_LIBPORTAL_INPUTCAPTURE
  std::unique_ptr<PortalClipboardProxy> m_clipboardProxy;
  std::unique_ptr<PortalClipboard> m_clipboardStandard;
  std::unique_ptr<PortalClipboard> m_clipboardPrimary;
#endif
};

} // namespace deskflow
