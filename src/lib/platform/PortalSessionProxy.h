/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 *
 * Portal Session Proxy - Qt DBus implementation for XDG Desktop Portal Session creation
 * and persistence (restore tokens).
 */

#pragma once

#include <QDBusConnection>
#include <QDBusObjectPath>
#include <QObject>
#include <QString>
#include <QVariantMap>

namespace deskflow {

/**
 * @brief QtDBus proxy for Portal Session management
 *
 * This class provides a direct D-Bus interface for creating RemoteDesktop
 * and InputCapture sessions. This is necessary for session persistence
 * (restore tokens) on systems with older libportal versions (e.g. 0.7.1).
 */
class PortalSessionProxy : public QObject
{
  Q_OBJECT

public:
  enum class SessionType
  {
    RemoteDesktop,
    InputCapture
  };

  explicit PortalSessionProxy(SessionType type, QObject *parent = nullptr);
  virtual ~PortalSessionProxy();

  /**
   * @brief Create a new portal session
   * @param restoreToken Optional token from a previous session
   * @return true if the request was sent successfully
   */
  bool createSession(const QString &restoreToken = QString());

  /**
   * @brief Get the session handle once created
   */
  QDBusObjectPath sessionHandle() const
  {
    return m_sessionHandle;
  }

  /**
   * @brief Get the restore token once session is started
   */
  QString restoreToken() const
  {
    return m_restoreToken;
  }

Q_SIGNALS:
  void sessionCreated(const QDBusObjectPath &sessionHandle);
  void sessionStarted(const QString &restoreToken);
  void sessionFailed(const QString &error);
  void sessionClosed();

private Q_SLOTS:
  void onRequestResponse(const QDBusObjectPath &path, const QVariantMap &results);
  void onSessionClosed();

private:
  void selectSources();
  void selectDevices();
  void start();
  void handleSessionCreated(const QVariantMap &results);
  void handleSessionStarted(const QVariantMap &results);
  QString setupRequestPath();
  void connectToRequest(const QString &path);

  SessionType m_type;
  QDBusConnection m_bus;
  QDBusObjectPath m_sessionHandle;
  QString m_restoreToken;

  static const char *SERVICE;
  static const char *PATH;
  static const char *REMOTE_DESKTOP_INTERFACE;
  static const char *INPUT_CAPTURE_INTERFACE;
  static const char *SESSION_INTERFACE;
  static const char *REQUEST_INTERFACE;
};

} // namespace deskflow
