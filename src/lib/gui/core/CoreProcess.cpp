/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2024 - 2025 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "CoreProcess.h"

#include "common/ExitCodes.h"
#include "gui/ipc/DaemonIpcClient.h"

#if defined(Q_OS_MACOS)
#include "OSXHelpers.h"
#endif

#include <QCoreApplication>
#include <QDir>
#include <QFile>
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

CoreProcess::CoreProcess(const IServerConfig &serverConfig)
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

  const auto logPath = requestDaemonLogPath();
  if (!logPath.isEmpty()) {
    if (m_daemonFileTail != nullptr) {
      disconnect(m_daemonFileTail, &FileTail::newLine, this, &CoreProcess::handleLogLines);
      m_daemonFileTail->deleteLater();
    }

    qDebug() << "daemon log path:" << logPath;
    m_daemonFileTail = new FileTail(logPath, this);
    connect(m_daemonFileTail, &FileTail::newLine, this, &CoreProcess::handleLogLines);
  }
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
    if (!m_daemonIpcClient->sendLogLevel(Settings::logLevelText())) {
      qWarning() << "failed to set daemon ipc log level";
    }
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

  m_process->start(m_appPath, args);

  if (m_process->waitForStarted()) {
    setProcessState(Started);
  } else {
    setProcessState(Stopped);
    Q_EMIT error(Error::StartFailed);
  }
}

void CoreProcess::startProcessFromDaemon(const QStringList &args)
{
  if (m_processState != ProcessState::Starting) {
    qFatal("core process must be in starting state");
  }

  QString commandQuoted = makeQuotedArgs(m_appPath, args);

  qInfo("running command: %s", qPrintable(commandQuoted));

  if (!m_daemonIpcClient->sendStartProcess(commandQuoted, Settings::value(Settings::Daemon::Elevate).toBool())) {
    qWarning("cannot start process, ipc command failed");
    return;
  }

  setProcessState(ProcessState::Started);
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

  if (!m_daemonIpcClient->sendStopProcess()) {
    qWarning("cannot stop process, ipc command failed");
    return;
  }

  setProcessState(ProcessState::Stopped);
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
#endif

    checkLogLine(line);
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
    const auto configFilename = persistServerConfig();
    if (configFilename.isEmpty()) {
      qFatal("config file name empty for server args");
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

  if (processMode == ProcessMode::Desktop) {
    startForegroundProcess(args);
  } else if (processMode == ProcessMode::Service) {
    args.append({QStringLiteral("--settings"), Settings::settingsFile()});
    startProcessFromDaemon(args);
  }

  m_lastProcessMode = processMode;
}

void CoreProcess::stop(std::optional<ProcessMode> processModeOption)
{
  QMutexLocker locker(&m_processMutex);

  const auto currentMode = Settings::value(Settings::Core::ProcessMode).value<ProcessMode>();
  const auto processMode = processModeOption.value_or(currentMode);

  qInfo("stopping core process (%s mode)", qPrintable(processModeToString(processMode)));

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

QString CoreProcess::persistServerConfig() const
{
  if (Settings::value(Settings::Server::ExternalConfig).toBool()) {
    return Settings::value(Settings::Server::ExternalConfigFile).toString();
  }

  const auto configFilePath = Settings::defaultValue(Settings::Server::ExternalConfigFile).toString();
  QFile configFile(configFilePath);
  if (!configFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    qFatal("failed to open core config file for write: %s", qPrintable(configFile.fileName()));
  }

  m_serverConfig.save(configFile);
  configFile.close();
  return configFile.fileName();
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

void CoreProcess::checkLogLine(const QString &line)
{
  using enum ConnectionState;

  if (line.contains("connected to server") || line.contains("has connected")) {
    m_connections++;
    setConnectionState(Connected);
  } else if (line.contains("started server")) {
    m_connections = 0;
    setConnectionState(Listening);
  } else if (line.contains("disconnected from server") || line.contains("process exited")) {
    m_connections = 0;
    setConnectionState(Disconnected);
  } else if (line.contains("connecting to")) {
    setConnectionState(Connecting);
  } else if (line.contains("has disconnected")) {
    m_connections--;
    if (m_connections < 1) {
      setConnectionState(Listening);
    }
  }

  checkSecureSocket(line);

  // server and client processes are not allowed to show notifications.
  // process the log from it and show notification from deskflow instead.
#ifdef Q_OS_MACOS
  checkOSXNotification(line);
#endif
}

bool CoreProcess::checkSecureSocket(const QString &line)
{
  static const QString tlsCheckString = "network encryption protocol: ";
  const auto index = line.indexOf(tlsCheckString, 0, Qt::CaseInsensitive);
  if (index == -1) {
    return false;
  }

  Q_EMIT secureSocket(true);
  m_secureSocketVersion = line.mid(index + tlsCheckString.size());
  return true;
}

#ifdef Q_OS_MACOS
void CoreProcess::checkOSXNotification(const QString &line)
{
  static const QString needle = "OSX Notification: ";
  if (line.contains(needle) && line.contains('|')) {
    int delimiterPosition = line.indexOf('|');
    int start = line.indexOf(needle);
    QString title = line.mid(start + needle.length(), delimiterPosition - start - needle.length());
    QString body = line.mid(delimiterPosition + 1, line.length() - delimiterPosition);
    if (!showOSXNotification(title, body)) {
      qDebug("osx notification was not shown");
    }
  }
}
#endif

QString CoreProcess::correctedAddress(const QString &address) const
{
  return wrapIpv6(address.simplified());
}

QString CoreProcess::requestDaemonLogPath()
{
  qDebug() << "requesting daemon log path";
  const auto logPath = m_daemonIpcClient->requestLogPath();
  if (logPath.isEmpty()) {
    qCritical() << "failed to get daemon log path";
    return QString();
  }

  if (QFileInfo logFile(logPath); !logFile.exists() || !logFile.isFile()) {
    qWarning() << "daemon log path file does not exist:" << logPath;
    return QString();
  }

  return logPath;
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
  if (m_daemonIpcClient->connectToServer()) {
    qInfo("successfully reconnected to daemon");
  }
}

} // namespace deskflow::gui
