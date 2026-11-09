/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 - 2026 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2024 - 2025 Synergy App Ltd
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "CoreProcess.h"
#include "DiscoveryScope.h"
#include "common/CandidatePolicy.h"
#include "common/DesktopWorkspace.h"
#include "common/DisplayTopology.h"
#include "common/ServiceSession.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QPointer>
#include <QUuid>

#include "common/ExitCodes.h"
#include "gui/ipc/CoreIpcClient.h"
#include "gui/ipc/DaemonIpcClient.h"

#if defined(Q_OS_MACOS)
#include "OSXHelpers.h"
#endif

#ifdef Q_OS_LINUX
#include <signal.h>
#include <sys/prctl.h>
#endif

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QMetaEnum>
#include <QMutexLocker>
#include <QRegularExpression>

namespace deskflow::gui {

const int kRetryDelay = 1000;
const auto kLineSplitRegex = QRegularExpression("\r|\n|\r\n");

QString CoreProcess::processModeToString(const Settings::ProcessMode mode)
{
  return QVariant::fromValue(mode).toString().toLower();
}

QString CoreProcess::processStateToString(const CoreProcess::ProcessState state)
{
  return QVariant::fromValue(state).toString().toLower();
}

/**
 * @brief Wraps options that contain spaces in quotes
 *
 * Useful to handle things like paths with spaces (e.g. "C:\Program Files").
 *
 * Can also be used to create a representation of a command that can be pasted
 * into a terminal.
 */
QString CoreProcess::makeQuotedArgs(const QString &app, const QStringList &args)
{
  QStringList command = {app};
  command.append(args);

  static const auto quote = QStringLiteral("\"");
  static const auto space = QStringLiteral(" ");
  QStringList quoted;
  for (const auto &item : std::as_const(command)) {
    auto temp = item.simplified();
    if (const auto wrapped = (temp.startsWith(quote) && temp.endsWith(quote)); temp.contains(space) && !wrapped) {
      temp = QStringLiteral("%1%2%1").arg(quote, temp);
    }
    quoted.append(temp);
  }

  return quoted.join(space);
}

/**
 * @brief If IPv6, ensures the IP is surround in square brackets.
 */
QString CoreProcess::wrapIpv6(const QString &address)
{
  static const auto colon = QStringLiteral(":");
  static const auto openBracket = QStringLiteral("[");
  static const auto closeBracket = QStringLiteral("]");

  if (!address.contains(colon) || address.isEmpty()) {
    return address;
  }

  QString wrapped = address;

  if (!address.startsWith(openBracket)) {
    wrapped.prepend(openBracket);
  }

  if (!address.endsWith(closeBracket)) {
    wrapped.append(closeBracket);
  }

  return wrapped;
}

//
// CoreProcess
//

CoreProcess::CoreProcess(const ServerConfig &serverConfig)
    : m_serverConfig(serverConfig),
      m_daemonIpcClient{new ipc::DaemonIpcClient(this)}
{
  m_appPath = QStringLiteral("%1/%2").arg(QCoreApplication::applicationDirPath(), kCoreBinName);
  if (!QFile::exists(m_appPath)) {
    qCritical("core server binary does not exist");
  }

  connect(m_daemonIpcClient, &ipc::DaemonIpcClient::peerStopped, this, &CoreProcess::serviceStopped);
  connect(m_daemonIpcClient, &ipc::DaemonIpcClient::peerSessionRejected, this, [this](const QString &token) {
    if ((token == m_serviceStopToken && m_processState == ProcessState::Stopping) ||
        (!m_serviceContext.isEmpty() && token == deskflow::auth::Context::decode(m_serviceContext).session)) {
      m_recoveryBlocked = true;
      m_retryTimer.stop();
      setConnectionState(ConnectionState::Disconnected);
      Q_EMIT error(Error::StartFailed);
    }
  });
  connect(m_daemonIpcClient, &ipc::DaemonIpcClient::connected, this, &CoreProcess::daemonIpcClientConnected);
  connect(
      m_daemonIpcClient, &ipc::DaemonIpcClient::connectionFailed, this, &CoreProcess::daemonIpcClientConnectionFailed
  );
  connect(m_daemonIpcClient, &ipc::DaemonIpcClient::logPathReceived, this, &CoreProcess::setupDaemonLogTail);

  m_rebindDeadline.setSingleShot(true);
  m_rebindDeadline.setInterval(5000);
  connect(this, &CoreProcess::connectionAttemptStarting, this, [this](bool server) {
    if (server)
      observeServerInterfaces();
    else
      m_boundServerScope.reset();
  });
  connect(this, &CoreProcess::connectionIntentChanged, this, [this](bool wanted) {
    if (!wanted)
      m_boundServerScope.reset();
  });
  connect(&m_rebindDeadline, &QTimer::timeout, this, [this] {
    if (!m_serverRebind)
      return;
    m_serverRebind.reset();
    m_recoveryBlocked = true;
    Q_EMIT error(Error::StartFailed);
  });
  connect(this, &CoreProcess::processStateChanged, this, [this](auto) { finishServerRebind(); }, Qt::QueuedConnection);
  connect(&m_retryTimer, &QTimer::timeout, this, [this] {
    if (m_startRequested && !m_recoveryBlocked && !m_addressRecoveryHold &&
        m_processState == ProcessState::RetryPending) {
      start();
    } else {
      qDebug("retry cancelled, process state is not retry pending");
    }
  });
}

void CoreProcess::onProcessReadyReadStandardOutput()
{
  if (m_process) {
    handleLogLines(m_process->readAllStandardOutput());
  }
}

void CoreProcess::onProcessReadyReadStandardError()
{
  if (m_process) {
    handleLogLines(m_process->readAllStandardError());
  }
}

void CoreProcess::daemonIpcClientConnected()
{
  applyLogLevel();
  m_daemonIpcClient->requestLogPath();
}

void CoreProcess::checkExistingProcess()
{
  // A GUI crash may leave this namespace's old core alive. Its authentication
  // context belongs to the old GUI; do not adopt it or display stale success.
  if (!m_startRequested || ++m_existingCoreAttempts > 3) {
    m_recoveryBlocked = true;
    setProcessState(ProcessState::Stopped);
    Q_EMIT error(Error::StartFailed);
    return;
  }
  auto *client = new ipc::CoreIpcClient(this);
  bindExistingProcess(client);
  client->connectToServer();
}
void CoreProcess::bindExistingProcess(ipc::CoreIpcClient *client)
{
  const auto generation = m_runGeneration;
  const QPointer<ipc::CoreIpcClient> source = client;
  const auto terminal = std::make_shared<bool>(false);
  auto *deadline = new QTimer(client);
  deadline->setObjectName("existingCoreStopDeadline");
  deadline->setSingleShot(true);
  auto stop = [this, client, generation, terminal] {
    if (*terminal || m_recoveryBlocked || generation != m_runGeneration || !m_startRequested) {
      client->deleteLater();
      return;
    }
    qInfo("asking the previous core in this application namespace to stop before a new session");
    client->sendStop();
  };
  connect(client, &ipc::CoreIpcClient::connected, this, stop);
  connect(client, &ipc::CoreIpcClient::versionMismatch, this, stop);
  connect(client, &ipc::CoreIpcClient::serverShutdown, this, [this, client, generation, deadline, terminal] {
    if (*terminal)
      return;
    *terminal = true;
    deadline->stop();
    client->deleteLater();
    if (m_recoveryBlocked || generation != m_runGeneration || !m_startRequested)
      return;
    setProcessState(ProcessState::RetryPending);
    m_retryTimer.setSingleShot(true);
    m_retryTimer.start(kRetryDelay);
  });
  auto fail = [this, source, generation, terminal, deadline] {
    if (!source || *terminal)
      return;
    *terminal = true;
    deadline->stop();
    source->deleteLater();
    if (generation != m_runGeneration || !m_startRequested)
      return;
    m_recoveryBlocked = true;
    m_retryTimer.stop();
    setProcessState(ProcessState::Stopped);
    Q_EMIT error(Error::StartFailed);
  };
  connect(client, &ipc::CoreIpcClient::connectionFailed, this, fail);
  connect(deadline, &QTimer::timeout, this, fail);
  deadline->start(5000);
}

void CoreProcess::onProcessFinished(int exitCode, QProcess::ExitStatus status)
{
  using enum ProcessState;
  const bool wanted = m_startRequested && !m_recoveryBlocked && !m_addressRecoveryHold && m_processState == Started;
  invalidatePendingEvents();
  setConnectionState(ConnectionState::Disconnected);
  m_retryTimer.stop();
  if (!wanted) {
    setProcessState(Stopped);
    return;
  }
  if (exitCode == s_exitDuplicate && status == QProcess::NormalExit) {
    setProcessState(Stopped);
    checkExistingProcess();
    return;
  }
  const bool normalRestart = exitCode == s_exitSuccess && status == QProcess::NormalExit;
  const bool crashRestart = status == QProcess::CrashExit && m_crashRetries < 3;
  if (normalRestart || crashRestart) {
    const int delay = crashRestart ? kRetryDelay * (1 << m_crashRetries++) : kRetryDelay;
    setProcessState(RetryPending);
    m_retryTimer.setSingleShot(true);
    m_retryTimer.start(delay);
  } else {
    m_recoveryBlocked = true;
    setProcessState(Stopped);
    Q_EMIT error(Error::StartFailed);
  }
}

void CoreProcess::applyLogLevel()
{
  const auto processMode = Settings::value(Settings::Core::ProcessMode).value<Settings::ProcessMode>();
  if (processMode == ProcessMode::Service) {
    qDebug() << "setting daemon log level:" << Settings::logLevelText();
    m_daemonIpcClient->sendLogLevel(Settings::logLevelText());
  }
}

void CoreProcess::startForegroundProcess(const QStringList &args)
{
  using enum ProcessState;

  if (m_processState != Starting) {
    qCritical("not starting core desktop process, unexpected process state");
    return;
  }

  // only make quoted args for printing the command for convenience; so that the
  // core command can be easily copy/pasted to the terminal for testing.
  const auto quoted = makeQuotedArgs(m_appPath, args);
  qInfo("running command: %s", qPrintable(quoted));

#ifdef Q_OS_LINUX
  m_process->setChildProcessModifier([] {
    // the core process becomes orphaned when the gui process exits abruptly (e.g. with kill -9),
    // so ensure the os also kills the core when that happens to the gui.
    prctl(PR_SET_PDEATHSIG, SIGTERM);
  });
#endif

  if (m_mode == Settings::CoreMode::Client) {
    Q_EMIT clientConnectionStarting(
        Settings::value(Settings::Client::RemoteHost).toString(), Settings::value(Settings::Core::Port).toInt(),
        Settings::value(Settings::Security::TlsEnabled).toBool() &&
            Settings::value(Settings::Security::CheckPeers).toBool()
    );
  }
  m_process->start(m_appPath, args);

  if (m_process->waitForStarted()) {
    setProcessState(Started);
  } else {
    setProcessState(Stopped);
    Q_EMIT error(Error::StartFailed);
  }
}

void CoreProcess::startProcessFromDaemon()
{
  if (m_processState != ProcessState::Starting) {
    qCritical("not starting core process from daemon, unexpected process state");
    return;
  }

  const auto configFile = Settings::settingsFile();
  qInfo("sending start to daemon (config file: %s)", qPrintable(configFile));

  const auto generation = m_runGeneration;
  auto sendStart = [this, configFile, generation] {
    if (generation != m_runGeneration || m_processState != ProcessState::Starting)
      return;
    if (!m_serviceContext.isEmpty()) {
      const deskflow::candidate::ServiceSession request{configFile, deskflow::auth::Context::decode(m_serviceContext)};
      if (!request.valid()) {
        m_recoveryBlocked = true;
        setProcessState(ProcessState::Stopped);
        Q_EMIT error(Error::StartFailed);
        return;
      }
      m_daemonIpcClient->sendPeerSession(request.encode());
      Q_EMIT clientConnectionStarting(
          Settings::value(Settings::Client::RemoteHost).toString(), Settings::value(Settings::Core::Port).toInt(), true
      );
    } else {
      m_daemonIpcClient->sendConfigFile(configFile);
      m_daemonIpcClient->sendStartProcess();
    }
    setProcessState(ProcessState::Started);
  };

  if (m_daemonIpcClient->isConnected()) {
    sendStart();
  } else {
    connect(
        m_daemonIpcClient, &ipc::DaemonIpcClient::connected, this, sendStart,
        static_cast<Qt::ConnectionType>(Qt::SingleShotConnection | Qt::QueuedConnection)
    );
    m_daemonIpcClient->connectToServer();
  }
}

void CoreProcess::stopForegroundProcess()
{
  if (m_processState != ProcessState::Stopping) {
    qCritical("not stopping core desktop process, unexpected process state");
    return;
  }

  if (!m_process) {
    setProcessState(ProcessState::Stopped);
    return;
  }

  qInfo("stopping core desktop process");
  if (m_coreIpcClient && m_coreIpcClient->isConnected()) {
    qDebug("sending stop command to core process");
    m_coreIpcClient->sendStop();

    // Fallback: if the core doesn't confirm shutdown via IPC within the
    // timeout, force-terminate it so we never hang in the Stopping state.
    const auto generation = m_runGeneration;
    const QPointer<QProcess> stoppingProcess = m_process;
    QTimer::singleShot(kRetryDelay, this, [this, generation, stoppingProcess] {
      if (generation != m_runGeneration || !stoppingProcess || m_process != stoppingProcess ||
          m_processState != ProcessState::Stopping) {
        return;
      }

      qWarning("core process did not confirm graceful stop in time, forcing terminate");

      if (m_coreIpcClient) {
        m_coreIpcClient->disconnectFromServer();
        m_coreIpcClient->deleteLater();
        m_coreIpcClient = nullptr;
      }

      if (m_process && m_process->state() == QProcess::ProcessState::Running) {
        m_process->terminate();
      }
    });
    return;
  }

  if (m_process->state() == QProcess::ProcessState::Running) {
    qDebug("process is running, terminating");
    m_process->terminate();
  } else {
    qDebug("process is not running, skipping terminate");
    setProcessState(ProcessState::Stopped);
  }
}

void CoreProcess::stopProcessFromDaemon()
{
  if (m_processState != ProcessState::Stopping) {
    qCritical("not stopping core process from daemon, unexpected process state");
    return;
  }

  const auto generation = m_runGeneration;
  auto sendStop = [this, generation] {
    if (generation != m_runGeneration || m_processState != ProcessState::Stopping)
      return;
#if !defined(DESKFLOW_CANDIDATE_BUILD) && !defined(DESKFLOW_LAB_BUILD)
    m_daemonIpcClient->sendStopProcess();
    setProcessState(ProcessState::Stopped);
    return;
#endif
    m_serviceStopToken = QUuid::createUuid().toString(QUuid::Id128);
    m_daemonIpcClient->sendPeerStop(m_serviceStopToken);
    const auto token = m_serviceStopToken;
    QTimer::singleShot(5500, this, [this, generation, token] {
      if (generation == m_runGeneration && token == m_serviceStopToken && m_processState == ProcessState::Stopping) {
        m_recoveryBlocked = true;
        Q_EMIT error(Error::StartFailed);
      }
    });
  };

  if (m_daemonIpcClient->isConnected()) {
    sendStop();
  } else {
    connect(
        m_daemonIpcClient, &ipc::DaemonIpcClient::connected, this, sendStop,
        static_cast<Qt::ConnectionType>(Qt::SingleShotConnection | Qt::QueuedConnection)
    );
    m_daemonIpcClient->connectToServer();
  }
}

void CoreProcess::serviceStopped(const QString &token)
{
  if (!token.isEmpty() && token == m_serviceStopToken && m_processState == ProcessState::Stopping) {
    m_serviceStopToken.clear();
    setProcessState(ProcessState::Stopped);
  }
}

void CoreProcess::handleLogLines(const QString &text)
{
  const auto lines = text.split(kLineSplitRegex);
  for (const auto &line : lines) {
    if (line.isEmpty()) {
      continue;
    }

#if defined(Q_OS_MACOS)
    // HACK: macOS 10.13.4+ spamming error lines in logs making them
    // impossible to read and debug; giving users a red herring.
    if (line.contains("calling TIS/TSM in non-main thread environment")) {
      continue;
    }

    // the core process is not allowed to show the permission prompt
    // (called "notification permission") and the notification log line is emitted from
    // deep inside cocoa code in the core binary to stdout, so it can't be sent over
    // ipc from the core to the gui and instead the gui has to parse the core output.
    static const QString needle = "OSX Notification: ";
    if (line.contains(needle) && line.contains('|')) {
      const int delimiterPosition = line.indexOf('|');
      const int start = line.indexOf(needle);
      const QString title = line.mid(start + needle.length(), delimiterPosition - start - needle.length());
      const QString body = line.mid(delimiterPosition + 1, line.length() - delimiterPosition);
      if (!showOSXNotification(title, body)) {
        qDebug("osx notification was not shown");
      }
    }
#endif

    Q_EMIT logLine(line);
  }
}

void CoreProcess::start(std::optional<ProcessMode> processModeOption)
{
#ifdef DESKFLOW_LAB_BUILD
  qInfo("DeskflowLab: input core launch disabled");
  Q_EMIT inputExecutionUnavailable();
  return;
#endif
#ifdef DESKFLOW_CANDIDATE_BUILD
  if (!deskflow::candidate::securityProfile(
          Settings::settingsFile(), Settings::value(Settings::Security::Certificate).toString(),
          Settings::value(Settings::Security::TlsEnabled).toBool(),
          Settings::value(Settings::Security::CheckPeers).toBool()
      )) {
    Q_EMIT error(Error::StartFailed);
    return;
  }
#endif
  if (m_processState == ProcessState::Started || m_processState == ProcessState::Starting ||
      m_processState == ProcessState::Stopping) {
    qCritical("not starting core process, already started");
    return;
  }

  if (m_mode == Settings::CoreMode::None) {
    qCritical("not starting core process, no core mode set");
    return;
  }

  QMutexLocker locker(&m_processMutex);
  if (m_processState != ProcessState::RetryPending)
    m_crashRetries = 0;
  const bool newIntent = !m_startRequested || m_addressRecoveryHold || m_recoveryBlocked;
  m_startRequested = true;
  m_recoveryBlocked = false;
  m_addressRecoveryHold = false;
  ++m_restartRequest;
  if (newIntent) {
    m_existingCoreAttempts = 0;
    Q_EMIT connectionIntentChanged(true);
  }
  invalidatePendingEvents();
  const auto generation = m_runGeneration;

  const auto currentMode = Settings::value(Settings::Core::ProcessMode).value<ProcessMode>();
  const auto processMode = processModeOption.value_or(currentMode);
  const auto coreMode = QVariant::fromValue(m_mode).toString().toLower();

  qInfo().noquote() << QString("starting %1 process (%2 mode)").arg(coreMode, processModeToString(processMode));

  Q_EMIT connectionAttemptStarting(
      m_mode == Settings::CoreMode::Server, Settings::value(Settings::Client::RemoteHost).toString(),
      Settings::value(Settings::Core::Port).toInt(), Settings::value(Settings::Security::TlsEnabled).toBool(),
      Settings::value(Settings::Security::CheckPeers).toBool()
  );
  setProcessState(ProcessState::Starting);

#ifdef Q_OS_MACOS
  requestOSXNotificationPermission();
#endif

  setConnectionState(ConnectionState::Connecting);

  if (processMode == ProcessMode::Desktop) {
    m_process = new QProcess(this);
    const QPointer<QProcess> process = m_process;
    connect(m_process, &QProcess::finished, this, [this, process](int code, QProcess::ExitStatus status) {
      if (!process || m_process != process)
        return;
      onProcessFinished(code, status);
      process->deleteLater();
      if (m_process == process)
        m_process = nullptr;
    });
    connect(m_process, &QProcess::readyReadStandardOutput, this, [this, process, generation] {
      if (process && m_process == process && generation == m_runGeneration)
        onProcessReadyReadStandardOutput();
    });
    connect(m_process, &QProcess::readyReadStandardError, this, [this, process, generation] {
      if (process && m_process == process && generation == m_runGeneration)
        onProcessReadyReadStandardError();
    });
  }

  QStringList args = {coreMode};
  const bool scopedService =
      processMode == ProcessMode::Service && deskflow::candidate::settingsPath(Settings::settingsFile());
  m_serviceContext.clear();
  m_serviceStopToken.clear();
  if (!m_expectedPeer.isEmpty() &&
      ((!scopedService && processMode != ProcessMode::Desktop) || m_mode != Settings::CoreMode::Client ||
       !Settings::value(Settings::Security::TlsEnabled).toBool() ||
       !Settings::value(Settings::Security::CheckPeers).toBool())) {
    m_startRequested = false;
    m_recoveryBlocked = true;
    setProcessState(ProcessState::Stopped);
    setConnectionState(ConnectionState::Disconnected);
    Q_EMIT error(Error::StartFailed);
    return;
  }
  if ((processMode == ProcessMode::Desktop || scopedService) && m_mode == Settings::CoreMode::Client &&
      Settings::value(Settings::Security::TlsEnabled).toBool() &&
      Settings::value(Settings::Security::CheckPeers).toBool()) {
    const auto context = prepareAuthentication();
    args << "--peer-auth-context" << context.encode();
    if (scopedService)
      m_serviceContext = context.encode();
  }

  if (m_mode == Settings::CoreMode::Server) {
    const auto [hasNeededPermissions, configFilename] = persistServerConfig();
    if (configFilename.isEmpty()) {
      qCritical("not starting core process, no server config file");
      setProcessState(ProcessState::Stopped);
      setConnectionState(ConnectionState::Disconnected);
      Q_EMIT error(Error::StartFailed);
      return;
    }
    if (!hasNeededPermissions) {
      setProcessState(ProcessState::Stopped);
      setConnectionState(ConnectionState::Disconnected);
      Q_EMIT error(Error::StartFailed);
      return;
    }
    qInfo("core config file: %s", qPrintable(configFilename));
  }

  qDebug().noquote() << "log level:" << Settings::logLevelText();

  if (Settings::value(Settings::Log::ToFile).toBool()) {
    const auto logFile = Settings::value(Settings::Log::File).toString();
    QDir(QFileInfo(logFile).absolutePath()).mkpath(".");
    qInfo().noquote() << "log file:" << logFile;
  }

  // Wired before the start calls so it catches Started from both sync (desktop) and async (service) paths.
  connect(
      this, &CoreProcess::processStateChanged, this,
      [this, generation](ProcessState state) {
        if (generation != m_runGeneration || state != ProcessState::Started) {
          return;
        }

        // Delay briefly to give the core process time to start its IPC server.
        QTimer::singleShot(kRetryDelay, this, [this, generation] {
          if (generation != m_runGeneration || m_processState != ProcessState::Started) {
            return;
          }

          if (m_coreIpcClient) {
            m_coreIpcClient->disconnectFromServer();
            m_coreIpcClient->deleteLater();
            m_coreIpcClient = nullptr;
          }

          m_coreIpcClient = new ipc::CoreIpcClient(this);

          bindCoreIpcClient(m_coreIpcClient);

          m_coreIpcClient->connectToServer();
        });
      },
      static_cast<Qt::ConnectionType>(Qt::SingleShotConnection | Qt::QueuedConnection)
  );

  if (processMode == ProcessMode::Desktop) {
    startForegroundProcess(args);
  } else if (processMode == ProcessMode::Service) {
    startProcessFromDaemon();
  }

  m_lastProcessMode = processMode;
}

void CoreProcess::stop(std::optional<ProcessMode> processModeOption)
{
  stopInternal(processModeOption, false);
}
void CoreProcess::stopInternal(std::optional<ProcessMode> processModeOption, bool preserveIntent)
{
  QMutexLocker locker(&m_processMutex);
  if (!preserveIntent) {
    m_serverRebind.reset();
    m_rebindDeadline.stop();
    const bool hadIntent = m_startRequested;
    m_startRequested = false;
    m_addressRecoveryHold = false;
    if (hadIntent)
      Q_EMIT connectionIntentChanged(false);
  }
  ++m_restartRequest;
  invalidatePendingEvents();
  m_retryTimer.stop();

  const auto currentMode = Settings::value(Settings::Core::ProcessMode).value<ProcessMode>();
  const auto processMode = processModeOption.value_or(currentMode);

  qInfo("stopping core process (%s mode)", qPrintable(processModeToString(processMode)));

  if (m_coreIpcClient && processMode != ProcessMode::Desktop) {
    m_coreIpcClient->disconnectFromServer();
    m_coreIpcClient->deleteLater();
    m_coreIpcClient = nullptr;
  }

  if (m_processState == ProcessState::Starting) {
    if (m_process && m_process->state() != QProcess::NotRunning) {
      setProcessState(ProcessState::Stopping);
      stopForegroundProcess();
    } else
      setProcessState(ProcessState::Stopped);
  } else if (m_processState != ProcessState::Stopped) {
    setProcessState(ProcessState::Stopping);

    if (processMode == ProcessMode::Service) {
      stopProcessFromDaemon();
    } else if (processMode == ProcessMode::Desktop) {
      stopForegroundProcess();
    }

  } else {
    qWarning("core process already stopped");
  }

  setConnectionState(ConnectionState::Disconnected);
}

void CoreProcess::observeServerInterfaces(DiscoveryScopeSource source)
{
  if (m_mode != Settings::CoreMode::Server || !m_startRequested || m_recoveryBlocked || m_serverRebind)
    return;
  const auto binding = Settings::value(Settings::Core::Interface).toString();
  if (binding.isEmpty()) {
    m_boundServerScope.reset();
    return;
  }
  const auto available = source();
  if (m_boundServerScope && binding != m_boundServerScope->address.toString())
    m_boundServerScope.reset();
  if (!m_boundServerScope) {
    for (const auto &scope : available)
      if (scope.valid() && scope.address.toString() == binding) {
        m_boundServerScope = scope;
        return;
      }
    return; // No previously observed physical link: never guess one.
  }
  if (available.contains(*m_boundServerScope))
    return;
  QVector<DiscoveryScope> replacements;
  for (const auto &scope : available)
    if (scope.valid() && scope.index == m_boundServerScope->index && scope.name == m_boundServerScope->name &&
        scope.prefix == m_boundServerScope->prefix &&
        scope.address.isInSubnet(m_boundServerScope->address, scope.prefix))
      replacements.append(scope);
  if (replacements.size() == 1)
    rebindServer(*m_boundServerScope, replacements.front(), std::move(source));
}
bool CoreProcess::rebindServer(const DiscoveryScope &before, const DiscoveryScope &after, DiscoveryScopeSource source)
{
  if (!before.valid() || !after.valid() || before == after || before.index != after.index ||
      before.name != after.name || before.prefix != after.prefix || m_mode != Settings::CoreMode::Server ||
      !m_startRequested || m_recoveryBlocked || m_serverRebind ||
      Settings::value(Settings::Core::Interface).toString() != before.address.toString() || !source().contains(after))
    return false;
  const auto mask = 0xffffffffu >> before.prefix;
  if ((before.address.toIPv4Address() & ~mask) != (after.address.toIPv4Address() & ~mask))
    return false;
  m_serverRebind = qMakePair(before, after);
  m_rebindScopes = std::move(source);
  m_rebindDeadline.start();
  stopInternal(std::nullopt, true);
  // Let the same network-change turn update the selected discovery scope and
  // withdraw its old advertisement before a new start snapshots GUI settings.
  QTimer::singleShot(0, this, &CoreProcess::finishServerRebind);
  return true;
}
void CoreProcess::finishServerRebind()
{
  if (!m_serverRebind || !recoveryQuiescent())
    return;
  const auto change = *m_serverRebind;
  m_serverRebind.reset();
  m_rebindDeadline.stop();
  if (!m_startRequested || m_recoveryBlocked || m_mode != Settings::CoreMode::Server ||
      !m_rebindScopes().contains(change.second) ||
      Settings::value(Settings::Core::Interface).toString() != change.first.address.toString())
    return;
  Settings::setValue(Settings::Core::Interface, change.second.address.toString());
  start(); // Only after this owned listener has completed its stop.
}
bool CoreProcess::recoveryPermitted() const
{
  return m_startRequested && !m_recoveryBlocked && m_mode == Settings::CoreMode::Client &&
         (Settings::value(Settings::Core::ProcessMode).value<ProcessMode>() == ProcessMode::Desktop ||
          (Settings::value(Settings::Core::ProcessMode).value<ProcessMode>() == ProcessMode::Service &&
           deskflow::candidate::settingsPath(Settings::settingsFile()))) &&
         Settings::value(Settings::Security::TlsEnabled).toBool() &&
         Settings::value(Settings::Security::CheckPeers).toBool();
}
bool CoreProcess::recoveryQuiescent() const
{
  return m_processState == ProcessState::Stopped && (!m_process || m_process->state() == QProcess::NotRunning);
}
bool CoreProcess::holdForAddressRecovery()
{
  if (!recoveryPermitted() || m_connectionState == ConnectionState::Connected)
    return false;
  if (m_addressRecoveryHold)
    return true;
  m_addressRecoveryHold = true;
  stopInternal(std::nullopt, true);
  return true;
}
bool CoreProcess::resumeForAddressRecovery(const QString &fp, const QString &address, int port)
{
  if (!m_addressRecoveryHold || !recoveryPermitted() || !recoveryQuiescent() ||
      deskflow::auth::fingerprint(fp).isEmpty() || fp != m_expectedPeer || port < 1 || port > 65535 ||
      !DiscoveryScope::privateAddress(QHostAddress(address)) || QHostAddress(address).toString() != address)
    return false;
  Settings::setValue(Settings::Client::RemoteHost, address);
  Settings::setValue(Settings::Core::Port, port);
  setAddress(address);
  m_addressRecoveryHold = false;
  Q_EMIT recoveredConnectionRequested(fp, address, port);
  if (!m_startRequested)
    return false;
  start();
  return true;
}
void CoreProcess::restart()
{
  const auto processMode = Settings::value(Settings::Core::ProcessMode).value<ProcessMode>();
  const QPointer<QProcess> old = m_process;
  const bool wait = old && old->state() != QProcess::NotRunning;
  stop(m_lastProcessMode.value_or(processMode));
  // A later explicit stop invalidates this restart, even if exit arrives late.
  const auto request = m_restartRequest;
  if (wait) {
    connect(
        old, &QProcess::finished, this,
        [this, request](int, QProcess::ExitStatus) {
          if (request == m_restartRequest)
            start();
        },
        Qt::SingleShotConnection
    );
  } else
    start();
}

void CoreProcess::cleanup()
{
#ifdef DESKFLOW_LAB_BUILD
  return; // No input process exists in this build.
#endif
  qInfo("cleaning up core process");

  const auto isDesktop = Settings::value(Settings::Core::ProcessMode).value<ProcessMode>() == ProcessMode::Desktop;
  const auto isRunning = m_processState == ProcessState::Started;
  if (isDesktop && isRunning) {
    stop();
  }
}

QPair<bool, QString> CoreProcess::persistServerConfig() const
{
  if (Settings::value(Settings::Server::ExternalConfig).toBool()) {
    return {Settings::isServerConfigFileReadable(), Settings::value(Settings::Server::ExternalConfigFile).toString()};
  }

  const auto configFilePath = Settings::defaultValue(Settings::Server::ExternalConfigFile).toString();
  if (!m_serverConfig.save(configFilePath)) {
    qWarning() << "failed to open core config file for write:" << configFilePath;
    return {false, configFilePath};
  }
  return {Settings::isServerConfigFileReadable(), configFilePath};
}

void CoreProcess::setConnectionState(ConnectionState state)
{
  // Retry attempts can repeat Connecting without a state change. Diagnostics
  // need that observation; existing consumers still get changes only.
  Q_EMIT connectionStateObserved(state);
  if (m_connectionState == state) {
    return;
  }

  m_connectionState = state;
  Q_EMIT connectionStateChanged(state);
}

void CoreProcess::setProcessState(ProcessState state)
{
  if (m_processState == state) {
    return;
  }

  qDebug(
      "core process state changed: %s -> %s", //
      qPrintable(processStateToString(m_processState)), qPrintable(processStateToString(state))
  );
  m_processState = state;
  Q_EMIT processStateChanged(state);
}

QString CoreProcess::applyWorkspace(const QByteArray &bytes)
{
  using namespace deskflow::display;
  const auto state = WorkspaceState::decode(bytes);
  if (!state || !state->revision || m_workspacePending || !m_workspaceLive || m_workspaceLive->session.isEmpty() ||
      !m_workspaceLive->generation || !m_coreIpcClient || m_mode != Settings::CoreMode::Server || !m_startRequested ||
      m_recoveryBlocked || m_processState != ProcessState::Started)
    return {};
  WorkspaceApplyRequest request{
      QUuid::createUuid().toString(QUuid::Id128), m_workspaceLive->session, m_workspaceLive->generation, *state
  };
  if (!WorkspaceApplyRequest::decode(request.encode()))
    return {};
  m_workspacePending = request;
  if (!m_coreIpcClient->sendWorkspaceApply(request.encode())) {
    m_workspacePending.reset();
    return {};
  }
  QTimer::singleShot(5000, this, [this, id = request.id] {
    if (!m_workspacePending || m_workspacePending->id != id)
      return;
    m_workspacePending.reset();
    Q_EMIT workspaceApplyFinished(id, "unknown", "acknowledgement-timeout");
  });
  return request.id;
}

void CoreProcess::invalidatePendingEvents()
{
  if (m_workspacePending) {
    const auto id = m_workspacePending->id;
    m_workspacePending.reset();
    Q_EMIT workspaceApplyFinished(id, "unknown", "core-session-ended");
  }
  m_workspaceLive.reset();
  ++m_runGeneration;
  m_secureSocketVersion.clear();
  m_authGate.cancel();
  m_authRequired = false;
  m_peerVerified = false;
  m_authenticatedPeer.clear();
  Q_EMIT authenticationCancelled();
}

bool CoreProcess::setExpectedPeerFingerprint(const QString &value)
{
  if (!recoveryQuiescent() || (m_startRequested && !m_addressRecoveryHold))
    return false;
  const auto fp = deskflow::auth::fingerprint(value);
  if (!value.isEmpty() && fp.isEmpty())
    return false;
  m_expectedPeer = fp;
  return true;
}

deskflow::auth::Context CoreProcess::prepareAuthentication()
{
  deskflow::auth::Context context{QUuid::createUuid().toString(QUuid::Id128), m_expectedPeer, m_runGeneration};
  m_authGate.begin(context);
  m_authRequired = true;
  m_peerVerified = false;
  m_authenticatedPeer.clear();
  Q_EMIT authenticationStarting(
      context, Settings::value(Settings::Client::RemoteHost).toString(), Settings::value(Settings::Core::Port).toInt()
  );
  return context;
}
#ifdef DESKFLOW_LAB_BUILD
deskflow::auth::Context CoreProcess::labBeginAuthentication(const QString &expected)
{
  invalidatePendingEvents();
  m_expectedPeer = deskflow::auth::fingerprint(expected);
  m_mode = Settings::CoreMode::Client;
  m_startRequested = true;
  m_recoveryBlocked = false;
  m_processState = ProcessState::Started;
  return prepareAuthentication();
}
#endif

void CoreProcess::bindCoreIpcClient(ipc::CoreIpcClient *client)
{
  const auto generation = m_runGeneration;
  const QPointer<ipc::CoreIpcClient> source = client;
  // Disconnecting does not retract already queued deliveries. A generation
  // and source check is required even when the new process is also Started.
  connect(
      client, &ipc::CoreIpcClient::commandReceived, this,
      [this, generation, source](const QString &command, const QString &args) {
        if (!source || generation != m_runGeneration || source != m_coreIpcClient ||
            m_processState != ProcessState::Started)
          return;
        onCoreIpcMessageReceived(command, args);
      },
      Qt::QueuedConnection
  );
  connect(client, &ipc::CoreIpcClient::connectionFailed, this, [this, source, generation] {
    if (source && source == m_coreIpcClient && generation == m_runGeneration && !m_serviceContext.isEmpty() &&
        m_processState == ProcessState::Started) {
      m_coreIpcClient = nullptr;
      source->deleteLater();
      onProcessFinished(1, QProcess::CrashExit);
    }
  });
  connect(client, &ipc::CoreIpcClient::serverShutdown, this, [this, source] {
    if (!source)
      return;
    if (source == m_coreIpcClient) {
      m_coreIpcClient = nullptr;
      if (!m_serviceContext.isEmpty() && m_processState == ProcessState::Started)
        onProcessFinished(1, QProcess::CrashExit);
    }
    source->deleteLater();
  });
}

void CoreProcess::onCoreIpcMessageReceived(const QString &command, const QString &args)
{
  if (command == "workspaceApplyResult") {
    if (m_mode != Settings::CoreMode::Server || !m_startRequested || m_recoveryBlocked || !m_workspacePending ||
        !m_workspaceLive)
      return;
    const auto result = deskflow::display::WorkspaceApplyResult::decode(args.toUtf8());
    if (!result || !result->matches(*m_workspacePending) || result->session != m_workspaceLive->session ||
        result->generation != m_workspaceLive->generation)
      return;
    m_workspacePending.reset();
    Q_EMIT workspaceApplyFinished(result->id, result->accepted ? "applied" : "rejected", result->error);
  } else if (command == "workspaceSnapshot") {
    if (!m_startRequested || m_recoveryBlocked || m_processState != ProcessState::Started || args.size() > 14000)
      return;
    const auto message = QJsonDocument::fromJson(args.toUtf8()).object();
    if (message.size() != 2 || !message["device"].isString() || !message["workspace"].isObject())
      return;
    const auto name = message["device"].toString();
    const auto payload = QJsonDocument(message["workspace"].toObject()).toJson(QJsonDocument::Compact);
    const auto snapshot = deskflow::display::WorkspaceSnapshot::decode(payload);
    if (!snapshot || !snapshot->devices.contains(name))
      return;
    if (m_mode == Settings::CoreMode::Server && name == Settings::value(Settings::Core::ComputerName).toString()) {
      m_workspaceLive = *snapshot;
      if (m_workspacePending && (m_workspacePending->session != snapshot->session ||
                                 m_workspacePending->generation != snapshot->generation)) {
        const auto id = m_workspacePending->id;
        m_workspacePending.reset();
        Q_EMIT workspaceApplyFinished(id, "unknown", "connection-generation-changed");
      }
      Q_EMIT workspaceSnapshotReceived(name, payload, QString());
    } else if (m_mode == Settings::CoreMode::Client && m_authRequired && m_peerVerified &&
               !m_authenticatedPeer.isEmpty())
      Q_EMIT workspaceSnapshotReceived(name, payload, m_authenticatedPeer);
  } else if (command == "displayTopology") {
    if (!m_startRequested || m_recoveryBlocked || args.size() > 20000)
      return;
    const auto message = QJsonDocument::fromJson(args.toUtf8()).object();
    const auto name = message["device"].toString();
    static const QRegularExpression validDevice("\\A[a-zA-Z0-9_.-]{1,255}\\z");
    if (message.size() != 2 || !validDevice.match(name).hasMatch() || !message["topology"].isObject())
      return;
    const auto object = message["topology"].toObject();
    const auto payload = QJsonDocument(object).toJson(QJsonDocument::Compact);
    if (!object.isEmpty() && !deskflow::display::Topology::decode(payload))
      return;
    if (m_mode == Settings::CoreMode::Server)
      Q_EMIT displayTopologyReceived(name, object.isEmpty() ? QByteArray() : payload);
    else if (m_mode == Settings::CoreMode::Client && m_processState == ProcessState::Started && m_authRequired &&
             m_peerVerified && !m_authenticatedPeer.isEmpty())
      // DTOP is sent only over the existing verified 1.9 stream. IPC is bound
      // to this core generation; Started/rejection/pause clear the peer gate.
      // The name is display metadata, never evidence used to grant trust.
      Q_EMIT verifiedPeerNameReceived(m_authenticatedPeer, name.left(80));
  } else if (command == "monitorRoutingMode") {
    if (m_mode == Settings::CoreMode::Server && (args == "per-monitor" || args == "aggregate"))
      Q_EMIT monitorRoutingModeChanged(args == "per-monitor");
  } else if (command == "powerState") {
    if (m_startRequested && !m_recoveryBlocked && (args == "sleep" || args == "wake"))
      Q_EMIT powerStateChanged(args == "sleep");
  } else if (command == "peerAuthentication") {
    const auto report = deskflow::auth::Report::decode(args);
    if (!m_startRequested || m_recoveryBlocked || m_mode != Settings::CoreMode::Client ||
        m_processState != ProcessState::Started || !report || !m_authGate.consume(*report))
      return;
    m_peerVerified = report->outcome == deskflow::auth::Outcome::Verified;
    m_authenticatedPeer = m_peerVerified ? report->observed : QString();
    if (report->outcome != deskflow::auth::Outcome::Started && report->outcome != deskflow::auth::Outcome::Verified) {
      m_recoveryBlocked = true;
      m_retryTimer.stop();
      setConnectionState(ConnectionState::Disconnected);
    }
    Q_EMIT authenticationResult(*report);
  } else if (command == "connectionState") {
    const auto metaEnum = QMetaEnum::fromType<ConnectionState>();
    bool ok = false;
    const auto state = static_cast<ConnectionState>(metaEnum.keyToValue(args.toUtf8().constData(), &ok));
    if (!ok) {
      qWarning("core ipc got unknown connection state: %s", args.toUtf8().constData());
      return;
    }
    if (state == ConnectionState::Connected && (m_recoveryBlocked || (m_authRequired && !m_peerVerified)))
      return;
    if (state == ConnectionState::Disconnected && m_mode == Settings::CoreMode::Client) {
      m_authGate.endAttempt();
      m_peerVerified = false;
      m_authenticatedPeer.clear();
    }
    if (state == ConnectionState::Connected || state == ConnectionState::Listening) {
      m_crashRetries = 0;
      m_existingCoreAttempts = 0;
    }
    setConnectionState(state);
  } else if (command == "connectedClients") {
    const auto clients = args.isEmpty() ? QStringList() : args.split(",");
    Q_EMIT connectedClientsChanged(clients);
  } else if (command == "secureSocket") {
    Q_EMIT secureSocket(true);
    if (args != m_secureSocketVersion) {
      m_secureSocketVersion = args;
      Q_EMIT securityLevelChanged(args);
    }
  } else if (command == "unrecognisedClient") {
    Q_EMIT unrecognisedClient(args);
  } else if (command == "connectionRefused") {
    const auto metaEnum = QMetaEnum::fromType<deskflow::core::ConnectionRefusal>();
    bool ok = false;
    const auto reason =
        static_cast<deskflow::core::ConnectionRefusal>(metaEnum.keyToValue(args.toUtf8().constData(), &ok));
    if (ok) {
      if (m_mode == Settings::CoreMode::Client) {
        m_recoveryBlocked = true;
        m_retryTimer.stop();
      }
      Q_EMIT connectionRefused(reason);
    } else {
      qWarning("core ipc got unknown connection refusal: %s", args.toUtf8().constData());
    }
  } else if (command == "retryIn") {
    bool ok = false;
    const auto seconds = args.toInt(&ok);
    if (ok && seconds >= 0 && seconds <= 3600 && !m_recoveryBlocked)
      Q_EMIT retryIn(seconds);
  } else if (command == "peerFingerprint") {
    // Correlated clients must wait for the structured authentication decision;
    // an unverified observation cannot open a trust dialog for an old attempt.
    if (!m_authRequired)
      Q_EMIT peerFingerprint(args);
  } else if (command == "missingKeyboardLayouts") {
    Q_EMIT missingKeyboardLayouts(args);
  }
}

bool CoreProcess::checkSecureSocket(const QString &line)
{
  static const QString tlsCheckString = "network encryption protocol: ";
  const auto index = line.indexOf(tlsCheckString, 0, Qt::CaseInsensitive);
  if (index == -1) {
    return false;
  }

  Q_EMIT secureSocket(true);
  if (const auto ssv = line.mid(index + tlsCheckString.size()); ssv != m_secureSocketVersion) {
    m_secureSocketVersion = ssv;
    Q_EMIT securityLevelChanged(ssv);
  }

  return true;
}

QString CoreProcess::correctedAddress(const QString &address) const
{
  return wrapIpv6(address.simplified());
}

void CoreProcess::setupDaemonLogTail(const QString &logPath)
{
  qDebug() << "daemon log path:" << logPath;

  if (QFileInfo logFile(logPath); !logFile.isFile()) {
    auto file = QFile(logPath);
    if (!file.open(QFile::ReadWrite)) {
      qCritical() << "daemon log path file can not be written:" << logPath;
      return;
    }
    file.write(""); // Create an empty file
  }

  if (m_daemonFileTail) {
    m_daemonFileTail->setWatchedFile(logPath);
  } else {
    m_daemonFileTail = new FileTail(logPath, this);
    connect(m_daemonFileTail, &FileTail::newLine, this, &CoreProcess::handleLogLines);
  }
}

void CoreProcess::clearSettings()
{
  const auto processMode = Settings::value(Settings::Core::ProcessMode).value<ProcessMode>();
  if (processMode == ProcessMode::Desktop) {
    qDebug("no core settings to clear in desktop mode");
    return;
  }

  if (processMode != ProcessMode::Service) {
    qCritical("not clearing core settings, unexpected process mode");
    return;
  }

  qInfo("clearing core settings through daemon");
  m_daemonIpcClient->sendClearSettings();
}

void CoreProcess::retryDaemon()
{
  m_daemonIpcClient->connectToServer();
}

} // namespace deskflow::gui
