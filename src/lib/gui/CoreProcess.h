/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2024 Symless Ltd.
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "AppConfig.h"
#include "IServerConfig.h"
#include "QIpcClient.h"

#include <QMutex>
#include <QObject>
#include <QProcess>
#include <QString>
#include <QStringList>

namespace synergy::gui {

class CoreProcess : public QObject {
  using IServerConfig = synergy::gui::IServerConfig;
  Q_OBJECT

public:
  enum class Mode { None, Client, Server };
  enum class Error { AddressMissing, StartFailed };
  enum class ProcessState { Started, Stopped };
  enum class ConnectionState {
    Disconnected,
    Connecting,
    Connected,
    Listening,
    PendingRetry
  };

  explicit CoreProcess(AppConfig &appConfig, IServerConfig &serverConfig);
  ~CoreProcess() override;

  void start();
  void stop();
  void stopService();
  void stopDesktop();

  // getters
  Mode mode() const { return m_mode; }
  bool isActive() const;
  QString secureSocketVersion() const { return m_secureSocketVersion; }

  // setters
  void setAddress(const QString &address) { m_address = address.trimmed(); }
  void setMode(Mode mode) { m_mode = mode; }

signals:
  void error(Error error);
  void logLine(const QString &line);
  void logInfo(const QString &message);
  void logError(const QString &message);
  void stateChanged(ConnectionState state);
  void secureSocket(bool enabled);

private slots:
  void onIpcClientRead(const QString &text);
  void onIpcClientErrorMessage(const QString &text);
  void onIpcClientInfoMessage(const QString &text);
  void onProcessFinished(int exitCode, QProcess::ExitStatus);
  void onProcessRetryStart();
  void onProcessReadyReadStandardOutput();
  void onProcessReadyReadStandardError();

private:
  QString appPath(const QString &name) const;
  bool serverArgs(QStringList &args, QString &app);
  bool clientArgs(QStringList &args, QString &app);
  QString persistConfig() const;
  QString address() const;
  QString coreModeString() const;
  void setCoreState(ConnectionState state);
  QString getProfileRootForArg() const;
  void checkLogLine(const QString &line);
  bool checkSecureSocket(const QString &line);
  bool checkOSXNotification(const QString &line);
  void handleLogLines(const QString &text);

  AppConfig &m_appConfig;
  IServerConfig &m_serverConfig;
  QString m_address;
  std::unique_ptr<QProcess> m_pProcess;
  ProcessState m_expectedProcessState = ProcessState::Stopped;
  ConnectionState m_state = ConnectionState::Disconnected;
  QIpcClient m_ipcClient;
  Mode m_mode = Mode::None;
  QMutex m_stopDesktopMutex;
  QString m_secureSocketVersion = "";
};

} // namespace synergy::gui
