/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 - 2025 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "CoreProcess.h"

#include "common/ExitCodes.h"
#include "common/Settings.h"
#include "gui/ipc/DaemonIpcClient.h"
#include "tls/TlsUtility.h"

#if defined(Q_OS_MAC)
#include "OSXHelpers.h"
#endif

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QMessageBox>
#include <QMutexLocker>
#include <QProcess>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTimer>

namespace deskflow::gui {

const int kRetryDelay = 1000;
const auto kServerConfigFilename = QStringLiteral("%1-server.conf").arg(kAppId);
const auto kLineSplitRegex = QRegularExpression("\r|\n|\r\n");

//
// free functions
//

QString processModeToString(Settings::ProcessMode mode)
{
  switch (mode) {
  case Settings::ProcessMode::Desktop:
    return "desktop";
  case Settings::ProcessMode::Service:
    return "service";
  default:
    qFatal("invalid process mode");
    abort();
  }
}

QString processStateToString(CoreProcess::ProcessState state)
{
  using enum CoreProcess::ProcessState;

  switch (state) {
  case Starting:
    return "starting";
  case Started:
    return "started";
  case Stopping:
    return "stopping";
  case Stopped:
    return "stopped";
  case RetryPending:
    return "retry pending";
  default:
    qFatal("invalid process state");
    abort();
  }
}

/**
 * @brief Wraps options that contain spaces in quotes
 *
 * Useful to handle things like paths with spaces (e.g. "C:\Program Files").
 *
 * Can also be used to create a representation of a command that can be pasted
 * into a terminal.
 */
QString makeQuotedArgs(const QString &app, const QStringList &args)
{
  QStringList command;
  command << app;
  command << args;

  QStringList quoted;
  for (const auto &arg : std::as_const(command)) {
    if (arg.contains(' ')) {
      quoted << QString("\"%1\"").arg(arg);
    } else {
      quoted << arg;
    }
  }

  return quoted.join(" ");
}

/**
 * @brief If IPv6, ensures the IP is surround in square brackets.
 */
QString wrapIpv6(const QString &address)
{
  if (!address.contains(':') || address.isEmpty()) {
    return address;
  }

  QString wrapped = address;

  if (address[0] != '[') {
    wrapped.insert(0, '[');
  }

  if (address[address.size() - 1] != ']') {
    wrapped.push_back(']');
  }

  return address;
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

  if (exitCode == s_exitDuplicate) {
    setProcessState(Stopped);
    qWarning("desktop process is already running");
    return;
  }

  if (exitCode != s_exitSuccess) {
    qWarning("desktop process exited with code: %d", exitCode);
  } else {
    qDebug("desktop process exited normally");
  }

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

#if defined(Q_OS_MAC)
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
  using enum Settings::CoreMode;

  QMutexLocker locker(&m_processMutex);

  const auto currentMode = Settings::value(Settings::Core::ProcessMode).value<ProcessMode>();
  const auto processMode = processModeOption.value_or(currentMode);

  qInfo().noquote(
  ) << QString("starting core %1 process (%2 mode)").arg(modeString(), processModeToString(processMode));

  if (m_processState == ProcessState::Started) {
    qCritical("core process already started");
    return;
  }

  // allow external listeners to abort the start process (e.g. licensing issue).
  setProcessState(ProcessState::Starting);
  Q_EMIT starting();
  if (m_processState == ProcessState::Stopped) {
    qDebug("core process start was cancelled by listener");
    return;
  }

#ifdef Q_OS_MAC
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

  QStringList args;

  addGenericArgs(args);

  if (mode() == Server) {
    args.prepend(QStringLiteral("server"));
    if (!addServerArgs(args))
      qWarning("failed to add server args for core process, aborting start");
  } else if (mode() == Client) {
    args.prepend(QStringLiteral("client"));
    if (!addClientArgs(args))
      qWarning("failed to add client args for core process, aborting start");
  } else {
    qFatal("core started without mode");
    return;
  }

  qDebug().noquote() << "log level:" << Settings::logLevelText();

  if (Settings::value(Settings::Log::ToFile).toBool())
    qInfo().noquote() << "log file:" << Settings::value(Settings::Log::File).toString();

  if (processMode == ProcessMode::Desktop) {
    startForegroundProcess(args);
  } else if (processMode == ProcessMode::Service) {
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

  if (m_lastProcessMode != processMode) {
    if (processMode == ProcessMode::Desktop) {
      qDebug("process mode changed to desktop, stopping service process");
      stop(ProcessMode::Service);
    } else if (processMode == ProcessMode::Service) {
      qDebug("process mode changed to service, stopping desktop process");
      stop(ProcessMode::Desktop);
    } else {
      qFatal("invalid process mode");
    }
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

bool CoreProcess::addGenericArgs(QStringList &args) const
{
  args << "--debug" << Settings::logLevelText();

  args << "--name" << Settings::value(Settings::Core::ScreenName).toString();

  if (Settings::value(Settings::Security::TlsEnabled).toBool()) {
    args << "--enable-crypto";
  }

  if (Settings::value(Settings::Core::PreventSleep).toBool()) {
    args << "--prevent-sleep";
  }

  return true;
}

bool CoreProcess::addServerArgs(QStringList &args)
{
  if (Settings::value(Settings::Log::ToFile).toBool()) {
    persistLogDir();
    args << "--log" << Settings::value(Settings::Log::File).toString();
  }

  if (!Settings::value(Settings::Security::CheckPeers).toBool()) {
    args << "--disable-client-cert-check";
  }

  QString configFilename = persistServerConfig();
  if (configFilename.isEmpty()) {
    qFatal("config file name empty for server args");
    return false;
  }

  // the address arg is dual purpose; when in listening mode, it's the address
  // that the server listens on. when tcp sockets are inverted, it connects to
  // that address. this is a bit confusing, and there should be probably be
  // different args for different purposes.
  args << "--address" << correctedInterface();

  args << "-c" << configFilename;
  qInfo("core config file: %s", qPrintable(configFilename));
  // bizarrely, the tls cert path arg was being given to the core client.
  // since it's not clear why (it is only needed for the server), this has now
  // been moved to server args.
  if (Settings::value(Settings::Security::TlsEnabled).toBool()) {
    if (TlsUtility tlsUtility(this); !tlsUtility.persistCertificate()) {
      qCritical("failed to persist tls certificate");
      return false;
    }
    args << "--tls-cert" << Settings::value(Settings::Security::Certificate).toString();
  }

  return true;
}

bool CoreProcess::addClientArgs(QStringList &args)
{
  if (Settings::value(Settings::Log::ToFile).toBool()) {
    persistLogDir();
    args << "--log" << Settings::value(Settings::Log::File).toString();
  }

  if (Settings::value(Settings::Client::LanguageSync).toBool()) {
    args << "--sync-language";
  }

  if (Settings::value(Settings::Client::InvertScrollDirection).toBool()) {
    args << "--invert-scroll";
  }

  if (correctedAddress().isEmpty()) {
    Q_EMIT error(Error::AddressMissing);
    qDebug("address is missing for client args");
    return false;
  }

  args << correctedAddress() + ":" + Settings::value(Settings::Core::Port).toString();

  return true;
}

QString CoreProcess::persistServerConfig() const
{
  if (Settings::value(Settings::Server::ExternalConfig).toBool()) {
    return Settings::value(Settings::Server::ExternalConfigFile).toString();
  }

  const auto configFilePath = QStringLiteral("%1/%2").arg(Settings::settingsPath(), kServerConfigFilename);
  QFile configFile(configFilePath);
  if (!configFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    qFatal("failed to open core config file for write: %s", qPrintable(configFile.fileName()));
  }

  m_serverConfig.save(configFile);
  configFile.close();
  return configFile.fileName();
}

QString CoreProcess::modeString() const
{
  switch (m_mode) {
  case Settings::CoreMode::Server:
    return "server";
  case Settings::CoreMode::Client:
    return "client";
  default:
    qFatal("invalid core mode");
    return "";
  }
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
#ifdef Q_OS_MAC
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

#ifdef Q_OS_MAC
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

QString CoreProcess::correctedInterface() const
{
  const QString interface = wrapIpv6(Settings::value(Settings::Core::Interface).toString());
  const auto port = Settings::value(Settings::Core::Port).toString();
  return QStringLiteral("%1:%2").arg(interface, port);
}

QString CoreProcess::correctedAddress() const
{
  return wrapIpv6(m_address);
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

void CoreProcess::persistLogDir() const
{
  QDir(QFileInfo(Settings::value(Settings::Log::File).toString()).absolutePath()).mkpath(".");
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
