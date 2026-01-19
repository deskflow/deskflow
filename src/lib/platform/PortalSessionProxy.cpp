/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 *
 * Portal Session Proxy - Qt DBus implementation for XDG Desktop Portal Session creation
 */

#include "PortalSessionProxy.h"
#include "base/Log.h"

#include <QDBusMessage>
#include <QDBusPendingCall>
#include <QDBusPendingReply>
#include <QUuid>

namespace deskflow {

const char *PortalSessionProxy::SERVICE = "org.freedesktop.portal.Desktop";
const char *PortalSessionProxy::PATH = "/org/freedesktop/portal/desktop";
const char *PortalSessionProxy::REMOTE_DESKTOP_INTERFACE = "org.freedesktop.portal.RemoteDesktop";
const char *PortalSessionProxy::INPUT_CAPTURE_INTERFACE = "org.freedesktop.portal.InputCapture";
const char *PortalSessionProxy::SESSION_INTERFACE = "org.freedesktop.portal.Session";
const char *PortalSessionProxy::REQUEST_INTERFACE = "org.freedesktop.portal.Request";

PortalSessionProxy::PortalSessionProxy(SessionType type, QObject *parent)
    : QObject(parent),
      m_type(type),
      m_bus(QDBusConnection::sessionBus())
{
}

PortalSessionProxy::~PortalSessionProxy()
{
}

bool PortalSessionProxy::createSession(const QString &restoreToken)
{
  if (!m_bus.isConnected()) {
    LOG_ERR("PortalSessionProxy: Session bus not connected");
    return false;
  }

  QString interface = (m_type == SessionType::RemoteDesktop) ? REMOTE_DESKTOP_INTERFACE : INPUT_CAPTURE_INTERFACE;

  QDBusMessage msg = QDBusMessage::createMethodCall(SERVICE, PATH, interface, "CreateSession");

  QVariantMap options;
  // Unique tokens for this request and the resulting session
  QString requestToken = "deskflow_req_" + QUuid::createUuid().toString(QUuid::WithoutBrackets);
  QString sessionToken = "deskflow_sess_" + QUuid::createUuid().toString(QUuid::WithoutBrackets);

  options["handle_token"] = requestToken;
  options["session_handle_token"] = sessionToken;

  if (!restoreToken.isEmpty()) {
    options["restore_token"] = restoreToken;
    LOG_DEBUG("PortalSessionProxy: Using restore token: %s", restoreToken.toUtf8().constData());
  }

  msg << options;

  // The request path follows a specific pattern
  QString sender = m_bus.baseService().replace(":", "").replace(".", "_");
  QString requestPath = QString("/org/freedesktop/portal/desktop/request/%1/%2").arg(sender, requestToken);

  connectToRequest(requestPath);

  QDBusPendingCall async = m_bus.asyncCall(msg);
  return true;
}

void PortalSessionProxy::connectToRequest(const QString &path)
{
  m_bus.connect(
      SERVICE, path, REQUEST_INTERFACE, "Response", this, SLOT(onRequestResponse(QDBusObjectPath, QVariantMap))
  );
}

void PortalSessionProxy::onRequestResponse(const QDBusObjectPath &path, const QVariantMap &results)
{
  // disconnect immediately
  m_bus.disconnect(
      SERVICE, path.path(), REQUEST_INTERFACE, "Response", this, SLOT(onRequestResponse(QDBusObjectPath, QVariantMap))
  );

  uint response = results.value("response").toUInt();
  if (response != 0) {
    LOG_ERR("PortalSessionProxy: Session creation failed or cancelled (response %u)", response);
    Q_EMIT sessionFailed("Portal request failed or cancelled");
    return;
  }

  if (results.contains("session_handle")) {
    m_sessionHandle = QDBusObjectPath(results.value("session_handle").toString());
    LOG_DEBUG("PortalSessionProxy: Session handle: %s", m_sessionHandle.path().toUtf8().constData());

    // Listen for session closure
    m_bus.connect(SERVICE, m_sessionHandle.path(), SESSION_INTERFACE, "Closed", this, SLOT(onSessionClosed()));

    Q_EMIT sessionCreated(m_sessionHandle);

    if (m_type == SessionType::RemoteDesktop) {
      selectSources();
    } else {
      selectDevices();
    }
  }
}

void PortalSessionProxy::selectSources()
{
  QDBusMessage msg = QDBusMessage::createMethodCall(SERVICE, PATH, REMOTE_DESKTOP_INTERFACE, "SelectSources");

  QVariantMap options;
  QString requestToken = "deskflow_sel_" + QUuid::createUuid().toString(QUuid::WithoutBrackets);
  options["handle_token"] = requestToken;

  options["types"] = uint(3); // Keyboard (1) | Pointer (2)

  msg << QVariant::fromValue(m_sessionHandle) << options;

  QString sender = m_bus.baseService().replace(":", "").replace(".", "_");
  QString requestPath = QString("/org/freedesktop/portal/desktop/request/%1/%2").arg(sender, requestToken);

  connectToRequest(requestPath);
  m_bus.asyncCall(msg);
}

void PortalSessionProxy::selectDevices()
{
  // InputCapture SelectDevices
  QDBusMessage msg = QDBusMessage::createMethodCall(SERVICE, PATH, INPUT_CAPTURE_INTERFACE, "SelectDevices");

  QVariantMap options;
  QString requestToken = "deskflow_sel_" + QUuid::createUuid().toString(QUuid::WithoutBrackets);
  options["handle_token"] = requestToken;

  msg << QVariant::fromValue(m_sessionHandle) << options;

  QString sender = m_bus.baseService().replace(":", "").replace(".", "_");
  QString requestPath = QString("/org/freedesktop/portal/desktop/request/%1/%2").arg(sender, requestToken);

  connectToRequest(requestPath);
  m_bus.asyncCall(msg);
}

void PortalSessionProxy::start()
{
  QString interface = (m_type == SessionType::RemoteDesktop) ? REMOTE_DESKTOP_INTERFACE : INPUT_CAPTURE_INTERFACE;

  QDBusMessage msg = QDBusMessage::createMethodCall(SERVICE, PATH, interface, "Start");

  QVariantMap options;
  QString requestToken = "deskflow_start_" + QUuid::createUuid().toString(QUuid::WithoutBrackets);
  options["handle_token"] = requestToken;

  msg << QVariant::fromValue(m_sessionHandle) << "" << options;

  QString sender = m_bus.baseService().replace(":", "").replace(".", "_");
  QString requestPath = QString("/org/freedesktop/portal/desktop/request/%1/%2").arg(sender, requestToken);

  connectToRequest(requestPath);
  m_bus.asyncCall(msg);
}

void PortalSessionProxy::onRequestResponse(const QDBusObjectPath &path, const QVariantMap &results)
{
  // disconnect immediately
  m_bus.disconnect(
      SERVICE, path.path(), REQUEST_INTERFACE, "Response", this, SLOT(onRequestResponse(QDBusObjectPath, QVariantMap))
  );

  uint response = results.value("response").toUInt();
  if (response != 0) {
    LOG_ERR(
        "PortalSessionProxy: Request failed or cancelled (response %u) for path %s", response,
        path.path().toUtf8().constData()
    );
    Q_EMIT sessionFailed("Portal request failed or cancelled");
    return;
  }

  // Determine which step finished based on path
  if (path.path().contains("deskflow_req_")) {
    handleSessionCreated(results);
  } else if (path.path().contains("deskflow_sel_")) {
    start();
  } else if (path.path().contains("deskflow_start_")) {
    handleSessionStarted(results);
  }
}

void PortalSessionProxy::handleSessionCreated(const QVariantMap &results)
{
  if (results.contains("session_handle")) {
    m_sessionHandle = QDBusObjectPath(results.value("session_handle").toString());
    LOG_DEBUG("PortalSessionProxy: Session handle: %s", m_sessionHandle.path().toUtf8().constData());

    // Listen for session closure
    m_bus.connect(SERVICE, m_sessionHandle.path(), SESSION_INTERFACE, "Closed", this, SLOT(onSessionClosed()));

    Q_EMIT sessionCreated(m_sessionHandle);

    if (m_type == SessionType::RemoteDesktop) {
      selectSources();
    } else {
      selectDevices();
    }
  }
}

void PortalSessionProxy::handleSessionStarted(const QVariantMap &results)
{
  if (results.contains("restore_token")) {
    m_restoreToken = results.value("restore_token").toString();
    LOG_DEBUG("PortalSessionProxy: Session started with restore token: %s", m_restoreToken.toUtf8().constData());
  }

  Q_EMIT sessionStarted(m_restoreToken);
}

} // namespace deskflow
