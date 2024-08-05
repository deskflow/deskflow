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
  enum class CoreMode { None, Client, Server };
  enum class Error { AddressMissing, StartFailed };
  enum class RuningState { Started, Stopped };
  enum class CoreState {
    Disconnected,
    Connecting,
    Connected,
    Listening,
    PendingRetry
  };

  explicit CoreProcess(AppConfig &appConfig, IServerConfig &serverConfig);
  ~CoreProcess() override;

  void startCore();
  void stopCore();
  void stopService();
  void stopDesktop();

  // getters
  CoreMode coreMode() const { return m_CoreMode; }
  bool isCoreActive() const;

  // setters
  void setAddress(const QString &address) { m_Address = address.trimmed(); }
  void setMode(CoreMode mode) { m_CoreMode = mode; }

signals:
  void error(Error error);
  void logLine(const QString &line);
  void logInfo(const QString &message);
  void logError(const QString &message);
  void stateChanged(CoreState state);

private slots:
  void onIpcClientReadLogLine(const QString &text);
  void onIpcClientErrorMessage(const QString &text);
  void onIpcClientInfoMessage(const QString &text);
  void onCoreProcessFinished(int exitCode, QProcess::ExitStatus);
  void onCoreProcessRetryStart();
  void onCoreProcessReadyReadStandardOutput();
  void onCoreProcessReadyReadStandardError();

private:
  QString appPath(const QString &name) const;
  bool serverArgs(QStringList &args, QString &app);
  bool clientArgs(QStringList &args, QString &app);
  QString persistConfig() const;
  QString address() const;
  QString coreModeString() const;
  void setCoreState(CoreState state);
  QString getProfileRootForArg() const;

  AppConfig &m_AppConfig;
  IServerConfig &m_ServerConfig;
  QString m_Address;
  std::unique_ptr<QProcess> m_pCoreProcess;
  RuningState m_ExpectedRunningState = RuningState::Stopped;
  CoreState m_CoreState = CoreState::Disconnected;
  QIpcClient m_IpcClient;
  CoreMode m_CoreMode = CoreMode::None;
  QMutex m_StopDesktopMutex;
};

} // namespace synergy::gui
