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

#include "IAppConfig.h"
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
  enum class ProcessState { Starting, Started, Stopping, Stopped };
  enum class ConnectionState { Disconnected, Connecting, Connected, Listening };

  explicit CoreProcess(IAppConfig &appConfig, IServerConfig &serverConfig);
  ~CoreProcess() override;

  void extracted(QString &app, QStringList &args);
  void start(std::optional<ProcessMode> processMode = std::nullopt);
  void stop(std::optional<ProcessMode> processMode = std::nullopt);
  void restart();

  // getters
  Mode mode() const { return m_mode; }
  QString secureSocketVersion() const { return m_secureSocketVersion; }
  bool isStarted() const { return m_processState == ProcessState::Started; }
  ProcessState processState() const { return m_processState; }
  ConnectionState connectionState() const { return m_connectionState; }

  // setters
  void setAddress(const QString &address) { m_address = address.trimmed(); }
  void setMode(Mode mode) { m_mode = mode; }

signals:
  void starting();
  void error(Error error);
  void logLine(const QString &line);
  void connectionStateChanged(ConnectionState state);
  void processStateChanged(ProcessState state);
  void secureSocket(bool enabled);

private slots:
  void onIpcClientServiceReady();
  void onIpcClientRead(const QString &text);
  void onIpcClientError(const QString &text);
  void onProcessFinished(int exitCode, QProcess::ExitStatus);
  void onProcessReadyReadStandardOutput();
  void onProcessReadyReadStandardError();

private:
  void startDesktop(const QString &app, const QStringList &args);
  void startService(const QString &app, const QStringList &args);
  void stopDesktop();
  void stopService();
  QString appPath(const QString &name) const;
  bool serverArgs(QStringList &args, QString &app);
  bool clientArgs(QStringList &args, QString &app);
  QString persistConfig() const;
  QString address() const;
  QString modeString() const;
  QString processModeString() const;
  void setConnectionState(ConnectionState state);
  void setProcessState(ProcessState state);
  QString getProfileRootForArg() const;
  void checkLogLine(const QString &line);
  bool checkSecureSocket(const QString &line);
  void handleLogLines(const QString &text);

#ifdef Q_OS_MAC
  void checkOSXNotification(const QString &line);
#endif

  IAppConfig &m_appConfig;
  IServerConfig &m_serverConfig;
  QString m_address;
  std::unique_ptr<QProcess> m_pProcess;
  ProcessState m_processState = ProcessState::Stopped;
  ConnectionState m_connectionState = ConnectionState::Disconnected;
  QIpcClient m_ipcClient;
  Mode m_mode = Mode::None;
  QMutex m_processMutex;
  QString m_secureSocketVersion = "";
  std::optional<ProcessMode> m_lastProcessMode = std::nullopt;
};

} // namespace synergy::gui
