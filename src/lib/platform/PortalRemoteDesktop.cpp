/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/PortalRemoteDesktop.h"
#include "base/Log.h"
#include "common/Constants.h"
#include "common/Settings.h"
#include "platform/PortalRequest.h"

#include "platform/XdgPortalRemoteDesktopInterface.h"

#include <QTimer>

namespace deskflow {

PortalRemoteDesktop::PortalRemoteDesktop(EiScreen *screen, IEventQueue *events) : m_screen{screen}, m_events{events}
{
  m_remoteDesktopInterface = std::make_unique<OrgFreedesktopPortalRemoteDesktopInterface>(
      "org.freedesktop.portal.Desktop", "/org/freedesktop/portal/desktop", QDBusConnection::sessionBus()
  );
  if (!m_remoteDesktopInterface->isValid()) {
    LOG_INFO("failed initilize remote desktop portal interface");
    return;
  }
  reconnect(0);
}

PortalRemoteDesktop::~PortalRemoteDesktop()
{
  // Key codes from linux/input.h
  static const auto s_leftCtrl = 29;
  static const auto s_rightCtrl = 97;
  static const auto s_leftShift = 42;
  static const auto s_rightShift = 54;
  static const auto s_leftAlt = 56;
  static const auto s_rightAlt = 100;
  static const auto s_leftMeta = 125;
  static const auto s_rightMeta = 126;
  // Make sure to clear any modifier keys that were pressed when the session closed, otherwise
  // we risk those keys getting stuck and the original session becoming unusable.
  for (auto keycode :
       {s_leftCtrl, s_rightCtrl, s_leftShift, s_rightShift, s_leftAlt, s_rightAlt, s_leftMeta, s_rightMeta}) {
    auto call = m_remoteDesktopInterface->NotifyKeyboardKeycode(m_dbusSessionPath, QVariantMap{}, keycode, 0);
    call.waitForFinished();
  }

  auto closeMessage = QDBusMessage::createMethodCall(
      "org.freedesktop.portal.Desktop", m_dbusSessionPath.path(), "org.freedesktop.portal.Session",
      QStringLiteral("Close")
  );
  QDBusConnection::sessionBus().asyncCall(closeMessage);

  LOG_INFO("closing remote desktop portal session");
}

void PortalRemoteDesktop::reconnect(uint32_t timeout)
{
  if (timeout == 0)
    openPortal();
  else
    QTimer::singleShot(timeout, this, &PortalRemoteDesktop::openPortal);
}

void PortalRemoteDesktop::openPortal()
{
  LOG_INFO("open the remote desktop portal");

  auto options = QVariantMap{
      {QStringLiteral("handle_token"), createToken()},
      {QStringLiteral("session_handle_token"), createToken()},
  };

  new PortalRequest(m_remoteDesktopInterface->CreateSession(options), this, &PortalRemoteDesktop::createSession);
}

QString PortalRemoteDesktop::createToken()
{
  return QStringLiteral("%1%2").arg(kAppId, QRandomGenerator::global()->generate());
}

void PortalRemoteDesktop::createSession(uint code, const QVariantMap &result)
{
  LOG_INFO("createSession");

  if (code != 0) {
    LOG_INFO("Could not open a new remote desktop session, error code %d", code);
    Q_EMIT error();
    return;
  }

  m_dbusSessionPath = QDBusObjectPath(result.value(QStringLiteral("session_handle")).toString());

  static const uint PermissionsPersistUntilExplicitlyRevoked = 2;

  auto options = QVariantMap{
      {QStringLiteral("types"), s_portalDevices},
      {QStringLiteral("handle_token"), createToken()},
      {QStringLiteral("persist_mode"), PermissionsPersistUntilExplicitlyRevoked},
  };

  QString restoreToken = Settings::value(Settings::Client::XdgRestoreToken).toString();
  if (!restoreToken.isEmpty()) {
    options[QStringLiteral("restore_token")] = restoreToken;
  }

  new PortalRequest(
      m_remoteDesktopInterface->SelectDevices(m_dbusSessionPath, options), this, &PortalRemoteDesktop::selectDevices
  );
}

void PortalRemoteDesktop::selectDevices(uint code, const QVariantMap &result)
{
  LOG_INFO("setDevices");
  if (code != 0) {
    LOG_INFO("Could not select devices for remote desktop session, error code %d", code);
    Q_EMIT error();
    return;
  }

  const QVariantMap options = {{QStringLiteral("types"), 7u}};
  new PortalRequest(
      m_remoteDesktopInterface->Start(m_dbusSessionPath, QString{}, options), this, &PortalRemoteDesktop::sessionStarted
  );
}

void PortalRemoteDesktop::sessionStarted(uint code, const QVariantMap &result)
{
  LOG_INFO("Start was called");
  if (code != 0) {
    LOG_INFO("Could not start screencast session, error code &d", code);
    Q_EMIT error();
    return;
  }

  if (result.value(QStringLiteral("devices")).toUInt() == 0) {
    LOG_INFO("No devices were granted: %d", result);
    Q_EMIT error();
    return;
  }

  Settings::setValue(Settings::Client::XdgRestoreToken, result.value(QStringLiteral("restore_token")));
}

void PortalRemoteDesktop::sessionClosed()
{
  LOG_INFO("remote desktop portal closed");
  Q_EMIT error();
}

} // namespace deskflow
