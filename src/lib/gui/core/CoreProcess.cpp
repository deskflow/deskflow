/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 - 2026 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2024 - 2025 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "CoreProcess.h"

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
    qFatal("core server binary does not exist");
    return;
  }

  connect(m_daemonIpcClient, &ipc::DaemonIpcClient::connected, this, &CoreProcess::daemonIpcClientConnected);
  connect(
      m_daemonIpcClient, &ipc::DaemonIpcClient::connectionFailed, this, &CoreProcess::daemonIpcClientConnectionFailed
  );
  connect(m_daemonIpcClient, &ipc::DaemonIpcClient::logPathReceived, this, &CoreProcess::setupDaemonLogTail);

  connect(&m_retryTimer, &QTimer::timeout, this, [this] {
    if (m_processState == ProcessState::RetryPending) {
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

void CoreProcess::onProcessFinished(int exitCode, QProcess::ExitStatus)
{
  using enum ProcessState;
  setConnectionState(ConnectionState::Disconnected);

  if (m_retryTimer.isActive()) {
    m_retryTimer.stop();
  }

  if (exitCode != s_exitSuccess) {
    setProcessState(Stopped);
    if (exitCode == s_exitDuplicate)
      qWarning("desktop process is already running");
    else
      qWarning("desktop process exited with code: %d", exitCode);
    return;
  }

  qDebug("desktop process exited normally");

  if (const auto wasStarted = m_processState == Started; wasStarted) {
    qDebug("desktop process was running, retrying in %d ms", kRetryDelay);
    setProcessState(RetryPending);
    m_retryTimer.setSingleShot(true);
    m_retryTimer.start(kRetryDelay);
  } else {
    setProcessState(Stopped);
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
    qFatal("core process must be in starting state");
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
    qFatal("core process must be in starting state");
  }

  const auto configFile = Settings::settingsFile();
  qInfo("sending start to daemon (config file: %s)", qPrintable(configFile));

  auto sendStart = [this, configFile] {
    m_daemonIpcClient->sendConfigFile(configFile);
    m_daemonIpcClient->sendStartProcess();
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

void CoreProcess::stopForegroundProcess() const
{
  if (m_processState != ProcessState::Stopping) {
    qFatal("core process must be in stopping state");
  }

  if (!m_process) {
    qFatal("process not set, cannot stop");
  }

  qInfo("stopping core desktop process");

  if (m_process->state() == QProcess::ProcessState::Running) {
    qDebug("process is running, closing");
    m_process->close();
  } else {
    qDebug("process is not running, skipping terminate");
  }
}

void CoreProcess::stopProcessFromDaemon()
{
  if (m_processState != ProcessState::Stopping) {
    qFatal("core process must be in stopping state");
  }

  auto sendStop = [this] {
    m_daemonIpcClient->sendStopProcess();
    setProcessState(ProcessState::Stopped);
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
  if (m_processState == ProcessState::Started) {
    qCritical("core process already started");
    return;
  }

  if (m_mode == Settings::CoreMode::None) {
    qFatal("set core mode before starting");
    return;
  }

  QMutexLocker locker(&m_processMutex);

  const auto currentMode = Settings::value(Settings::Core::ProcessMode).value<ProcessMode>();
  const auto processMode = processModeOption.value_or(currentMode);
  const auto coreMode = QVariant::fromValue(m_mode).toString().toLower();

  qInfo().noquote() << QString("starting %1 process (%2 mode)").arg(coreMode, processModeToString(processMode));

  setProcessState(ProcessState::Starting);

#ifdef Q_OS_MACOS
  requestOSXNotificationPermission();
#endif

  setConnectionState(ConnectionState::Connecting);

  if (processMode == ProcessMode::Desktop) {
    m_process = new QProcess(this);
    connect(m_process, &QProcess::finished, this, &CoreProcess::onProcessFinished, Qt::UniqueConnection);
    connect(
        m_process, &QProcess::readyReadStandardOutput, this, &CoreProcess::onProcessReadyReadStandardOutput,
        Qt::UniqueConnection
    );
    connect(
        m_process, &QProcess::readyReadStandardError, this, &CoreProcess::onProcessReadyReadStandardError,
        Qt::UniqueConnection
    );
  }

  QStringList args = {coreMode};

  if (m_mode == Settings::CoreMode::Server) {
    const auto [hasNeededPermissions, configFilename] = persistServerConfig();
    if (configFilename.isEmpty()) {
      qFatal("config file name empty for server args");
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
      [this](ProcessState state) {
        if (state != ProcessState::Started) {
          return;
        }

        // Delay briefly to give the core process time to start its IPC server.
        QTimer::singleShot(kRetryDelay, this, [this] {
          if (m_processState != ProcessState::Started) {
            return;
          }

          m_coreIpcClient = new ipc::CoreIpcClient(this);
          connect(m_coreIpcClient, &ipc::CoreIpcClient::commandReceived, this, &CoreProcess::onCoreIpcMessageReceived);
          connect(m_coreIpcClient, &ipc::CoreIpcClient::connected, this, [] { qInfo("connected to core ipc server"); });
          connect(m_coreIpcClient, &ipc::CoreIpcClient::connectionFailed, this, [] {
            qWarning("failed to establish core ipc connection");
          });

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
  QMutexLocker locker(&m_processMutex);

  const auto currentMode = Settings::value(Settings::Core::ProcessMode).value<ProcessMode>();
  const auto processMode = processModeOption.value_or(currentMode);

  qInfo("stopping core process (%s mode)", qPrintable(processModeToString(processMode)));

  if (m_coreIpcClient) {
    m_coreIpcClient->disconnectFromServer();
    m_coreIpcClient->deleteLater();
    m_coreIpcClient = nullptr;
  }

  if (m_processState == ProcessState::Starting) {
    qDebug("core process is starting, cancelling");
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

void CoreProcess::restart()
{
  qDebug("restarting core process");

  const auto processMode = Settings::value(Settings::Core::ProcessMode).value<ProcessMode>();

  if (m_lastProcessMode != std::nullopt && m_lastProcessMode != processMode) {
    const auto debugMessage =
        QStringLiteral("process mode changed to %1, stopping %2 process")
            .arg(processModeToString(processMode), processModeToString(m_lastProcessMode.value()));
    qDebug().noquote() << debugMessage;
    stop(m_lastProcessMode);
  } else {
    // in service mode: though there is technically no need to stop the service
    // before restarting it, it does make for cleaner process state tracking,
    // especially if something goes wrong with starting the service.
    stop();
  }

  start();
}

void CoreProcess::cleanup()
{
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
  QFile configFile(configFilePath);
  if (!configFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    qWarning() << "failed to open core config file for write:" << configFilePath;
    return {false, configFile.fileName()};
  }

  m_serverConfig.save(configFile);
  configFile.close();
  return {Settings::isServerConfigFileReadable(), configFile.fileName()};
}

void CoreProcess::setConnectionState(ConnectionState state)
{
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

void CoreProcess::onCoreIpcMessageReceived(const QString &command, const QString &args)
{
  if (command == "connectionState") {
    const auto metaEnum = QMetaEnum::fromType<ConnectionState>();
    bool ok = false;
    const auto state = static_cast<ConnectionState>(metaEnum.keyToValue(args.toUtf8().constData(), &ok));
    if (!ok) {
      qWarning("core ipc got unknown connection state: %s", args.toUtf8().constData());
      return;
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
      Q_EMIT connectionRefused(reason);
    } else {
      qWarning("core ipc got unknown connection refusal: %s", args.toUtf8().constData());
    }
  } else if (command == "retryIn") {
    Q_EMIT retryIn(args.toInt());
  } else if (command == "peerFingerprint") {
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
    qFatal("invalid process mode");
  }

  qInfo("clearing core settings through daemon");
  m_daemonIpcClient->sendClearSettings();
}

void CoreProcess::retryDaemon()
{
  m_daemonIpcClient->connectToServer();
}

} // namespace deskflow::gui
