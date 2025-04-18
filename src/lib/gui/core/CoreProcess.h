/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 - 2025 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "common/Settings.h"
#include "gui/FileTail.h"
#include "gui/config/IServerConfig.h"
#include "gui/proxy/QProcessProxy.h"

#include <memory>

#include <QFileSystemWatcher>
#include <QMutex>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>

namespace deskflow::gui {

namespace ipc {
class DaemonIpcClient;
}

class CoreProcess : public QObject
{
  using ProcessMode = Settings::ProcessMode;
  using IServerConfig = deskflow::gui::IServerConfig;
  using QProcessProxy = deskflow::gui::proxy::QProcessProxy;

  Q_OBJECT

public:
  struct Deps
  {
    virtual ~Deps() = default;
    virtual QProcessProxy &process()
    {
      return m_process;
    }
    virtual QString appPath(const QString &name) const;
    virtual bool fileExists(const QString &path) const;

  private:
    QProcessProxy m_process;
  };

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
  enum class ConnectionState
  {
    Disconnected,
    Connecting,
    Connected,
    Listening
  };

  explicit CoreProcess(const IServerConfig &serverConfig, std::shared_ptr<Deps> deps = std::make_shared<Deps>());

  void extracted(QString &app, QStringList &args);
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
    m_address = address.trimmed();
  }
  void setMode(Settings::CoreMode mode)
  {
    m_mode = mode;
  }

signals:
  void starting();
  void error(Error error);
  void logLine(const QString &line);
  void connectionStateChanged(ConnectionState state);
  void processStateChanged(ProcessState state);
  void secureSocket(bool enabled);
  void daemonIpcClientConnectionFailed();

private slots:
  void onProcessFinished(int exitCode, QProcess::ExitStatus);
  void onProcessReadyReadStandardOutput();
  void onProcessReadyReadStandardError();
  void daemonIpcClientConnected();

private:
  void startForegroundProcess(const QString &app, const QStringList &args);
  void startProcessFromDaemon(const QString &app, const QStringList &args);
  void stopForegroundProcess() const;
  void stopProcessFromDaemon();
  bool addGenericArgs(QStringList &args, const ProcessMode processMode) const;
  bool addServerArgs(QStringList &args, QString &app);
  bool addClientArgs(QStringList &args, QString &app);
  QString persistServerConfig() const;
  QString modeString() const;
  QString processModeString() const;
  void setConnectionState(ConnectionState state);
  void setProcessState(ProcessState state);
  void checkLogLine(const QString &line);
  bool checkSecureSocket(const QString &line);
  void handleLogLines(const QString &text);
  QString correctedInterface() const;
  QString correctedAddress() const;
  QString requestDaemonLogPath();
  void persistLogDir();

#ifdef Q_OS_MAC
  void checkOSXNotification(const QString &line);
#endif

  const IServerConfig &m_serverConfig;
  std::shared_ptr<Deps> m_pDeps;
  QString m_address;
  ProcessState m_processState = ProcessState::Stopped;
  ConnectionState m_connectionState = ConnectionState::Disconnected;
  Settings::CoreMode m_mode = Settings::CoreMode::None;
  QMutex m_processMutex;
  QString m_secureSocketVersion = "";
  std::optional<ProcessMode> m_lastProcessMode = std::nullopt;
  QTimer m_retryTimer;
  int m_connections = 0;
  deskflow::gui::ipc::DaemonIpcClient *m_daemonIpcClient = nullptr;
  FileTail *m_daemonFileTail = nullptr;
};

} // namespace deskflow::gui
