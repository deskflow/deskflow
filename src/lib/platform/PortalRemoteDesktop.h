/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QDBusObjectPath>
#include <QObject>

#include "platform/EiScreen.h"

class OrgFreedesktopPortalRemoteDesktopInterface;

namespace deskflow {

class PortalRemoteDesktop : public QObject
{
  Q_OBJECT
public:
  PortalRemoteDesktop(EiScreen *screen, IEventQueue *events);
  ~PortalRemoteDesktop();

Q_SIGNALS:
  void error();
  void started();

private:
  void reconnect(uint32_t timeout = 1000);
  void openPortal();
  QString createToken();
  void createSession(uint code, const QVariantMap &result);
  void selectDevices(uint code, const QVariantMap &result);
  void sessionStarted(uint code, const QVariantMap &result);
  void sessionClosed();

  EiScreen *m_screen;
  IEventQueue *m_events;
  std::unique_ptr<OrgFreedesktopPortalRemoteDesktopInterface> m_remoteDesktopInterface;
  QDBusObjectPath m_dbusSessionPath;

  // Items needed for portal access
  // Device Keyboadrd and Device Pointer as definded in libportal
  inline static const uint s_portalDevices = 1 << 0 | 1 << 1;
  inline static const uint s_portalPersistanceType = 2; // Until Revoked
};

} // namespace deskflow
