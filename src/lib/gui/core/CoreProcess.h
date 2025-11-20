/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2024 - 2025 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "common/Settings.h"
#include "gui/FileTail.h"
#include "gui/config/IServerConfig.h"

#include <QMutex>
#include <QObject>
#include <QProcess>
#include <QTimer>

namespace deskflow::gui {

namespace ipc {
class DaemonIpcClient;
}

class CoreProcess : public QObject
{
  using ProcessMode = Settings::ProcessMode;
  using IServerConfig = deskflow::gui::IServerConfig;

  Q_OBJECT

public:
  enum class Error
  {
    AddressMissing,
    StartFailed
  };
  enum class ProcessState
  {
    Starting,
    Started,
    Stopping,
    Stopped,
    RetryPending
  };
  Q_ENUM(ProcessState)

  enum class ConnectionState
  {
    Disconnected,
    Connecting,
    Connected,
    Listening
  };

  explicit CoreProcess(const IServerConfig &serverConfig);

  void start(std::optional<ProcessMode> processMode = std::nullopt);
  void stop(std::optional<ProcessMode> processMode = std::nullopt);
  void restart();
  void cleanup();
  void applyLogLevel();
  void clearSettings();
  void retryDaemon();

  // getters
  Settings::CoreMode mode() const
  {
    return m_mode;
  }
  QString secureSocketVersion() const
  {
    return m_secureSocketVersion;
  }
  bool isStarted() const
  {
    return m_processState == ProcessState::Started;
  }
  ProcessState processState() const
  {
    return m_processState;
  }
  ConnectionState connectionState() const
  {
    return m_connectionState;
  }

  // setters
  void setAddress(const QString &address)
  {
    m_address = correctedAddress(address);
  }
  void setMode(Settings::CoreMode mode)
  {
    m_mode = mode;
  }

Q_SIGNALS:
  void error(deskflow::gui::CoreProcess::Error error);
  void logLine(const QString &line);
  void connectionStateChanged(deskflow::gui::CoreProcess::ConnectionState state);
  void processStateChanged(deskflow::gui::CoreProcess::ProcessState state);
  void secureSocket(bool enabled);
  void daemonIpcClientConnectionFailed();

private Q_SLOTS:
  void onProcessFinished(int exitCode, QProcess::ExitStatus);
  void onProcessReadyReadStandardOutput();
  void onProcessReadyReadStandardError();
  void daemonIpcClientConnected();

private:
  void startForegroundProcess(const QStringList &args);
  void startProcessFromDaemon(const QStringList &args);
  void stopForegroundProcess() const;
  void stopProcessFromDaemon();
  QString persistServerConfig() const;
  void setConnectionState(ConnectionState state);
  void setProcessState(ProcessState state);
  void checkLogLine(const QString &line);
  bool checkSecureSocket(const QString &line);
  void handleLogLines(const QString &text);
  QString correctedAddress(const QString &address) const;
  QString requestDaemonLogPath();
  static QString makeQuotedArgs(const QString &app, const QStringList &args);
  static QString processModeToString(const Settings::ProcessMode mode);
  static QString processStateToString(const CoreProcess::ProcessState state);
  static QString wrapIpv6(const QString &address);

#ifdef Q_OS_MACOS
  void checkOSXNotification(const QString &line);
#endif

  const IServerConfig &m_serverConfig;
  QString m_address;
  ProcessState m_processState = ProcessState::Stopped;
  ConnectionState m_connectionState = ConnectionState::Disconnected;
  Settings::CoreMode m_mode = Settings::CoreMode::None;
  QMutex m_processMutex;
  QString m_secureSocketVersion;
  std::optional<ProcessMode> m_lastProcessMode = std::nullopt;
  QTimer m_retryTimer;
  int m_connections = 0;
  deskflow::gui::ipc::DaemonIpcClient *m_daemonIpcClient = nullptr;
  FileTail *m_daemonFileTail = nullptr;
  QProcess *m_process = nullptr;
  QString m_appPath;
};

} // namespace deskflow::gui
