/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "gui/config/IAppConfig.h"
#include "gui/config/IServerConfig.h"
#include "gui/ipc/QIpcClient.h"
#include "gui/proxy/QProcessProxy.h"

#include <QMutex>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <memory>

namespace deskflow::gui {

class CoreProcess : public QObject
{
  using IServerConfig = deskflow::gui::IServerConfig;
  using QProcessProxy = deskflow::gui::proxy::QProcessProxy;
  using IQIpcClient = deskflow::gui::ipc::IQIpcClient;

  Q_OBJECT

public:
  struct Deps
  {
    virtual ~Deps() = default;
    virtual QProcessProxy &process()
    {
      return m_process;
    }
    virtual IQIpcClient &ipcClient()
    {
      return m_ipcClient;
    }
    virtual QString appPath(const QString &name) const;
    virtual bool fileExists(const QString &path) const;
    virtual QString getProfileRoot() const;

  private:
    QProcessProxy m_process;
    QIpcClient m_ipcClient;
  };

  enum class Mode
  {
    None,
    Client,
    Server
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

  explicit CoreProcess(
      const IAppConfig &appConfig, const IServerConfig &serverConfig,
      std::shared_ptr<Deps> deps = std::make_shared<Deps>()
  );

  void extracted(QString &app, QStringList &args);
  void start(std::optional<ProcessMode> processMode = std::nullopt);
  void stop(std::optional<ProcessMode> processMode = std::nullopt);
  void restart();
  void cleanup();

  // getters
  Mode mode() const
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
  void setMode(Mode mode)
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

private slots:
  void onIpcClientServiceReady();
  void onIpcClientRead(const QString &text);
  void onIpcClientError(const QString &text) const;
  void onProcessFinished(int exitCode, QProcess::ExitStatus);
  void onProcessReadyReadStandardOutput();
  void onProcessReadyReadStandardError();

private:
  void startDesktop(const QString &app, const QStringList &args);
  void startService(const QString &app, const QStringList &args);
  void stopDesktop() const;
  void stopService();
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

#ifdef Q_OS_MAC
  void checkOSXNotification(const QString &line);
#endif

  const IAppConfig &m_appConfig;
  const IServerConfig &m_serverConfig;
  std::shared_ptr<Deps> m_pDeps;
  QString m_address;
  ProcessState m_processState = ProcessState::Stopped;
  ConnectionState m_connectionState = ConnectionState::Disconnected;
  Mode m_mode = Mode::None;
  QMutex m_processMutex;
  QString m_secureSocketVersion = "";
  std::optional<ProcessMode> m_lastProcessMode = std::nullopt;
  QTimer m_retryTimer;
};

} // namespace deskflow::gui
