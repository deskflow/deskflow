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

#include "CoreProcess.h"

#include "constants.h"
#include "gui/config/IAppConfig.h"
#include "gui/core/CoreTool.h"

#if defined(Q_OS_MAC)
#include "OSXHelpers.h"
#endif

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QMessageBox>
#include <QMutexLocker>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTimer>

namespace synergy::gui {

const int kRetryDelay = 1000;
const auto kLastConfigFilename = "LastConfig.cfg";
const auto kLineSplitRegex = QRegularExpression("\r|\n|\r\n");

//
// free functions
//

QString processModeToString(ProcessMode mode) {
  using enum ProcessMode;

  switch (mode) {
  case kDesktop:
    return "desktop";
  case kService:
    return "service";
  default:
    qFatal("invalid process mode");
    return "";
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
QString makeQuotedArgs(const QString &app, const QStringList &args) {
  QStringList command;
  command << app;
  command << args;

  QStringList quoted;
  for (const auto &arg : command) {
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
QString wrapIpv6(const QString &address) {
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
// CoreProcess::Deps
//

QString CoreProcess::Deps::appPath(const QString &name) const {
  QDir dir(QCoreApplication::applicationDirPath());
  return dir.filePath(name);
}

bool CoreProcess::Deps::fileExists(const QString &path) const {
  return QFile::exists(path);
}

QString CoreProcess::Deps::getProfileRoot() const {
  CoreTool coreTool;
  QDir appDir = coreTool.getProfileDir();

  // the core expects the profile root dir, not the app-specific profile dir.
  if (!appDir.cdUp()) {
    qFatal("failed to cd up to profile root dir");
  }

  return appDir.absolutePath();
}

//
// CoreProcess
//

CoreProcess::CoreProcess(
    IAppConfig &appConfig, IServerConfig &serverConfig,
    std::shared_ptr<Deps> deps)
    : m_appConfig(appConfig),
      m_serverConfig(serverConfig),
      m_pDeps(deps) {

  connect(
      &m_pDeps->ipcClient(), &QIpcClient::read, this, //
      &CoreProcess::onIpcClientRead);

  connect(
      &m_pDeps->ipcClient(), &QIpcClient::serviceReady, this, //
      &CoreProcess::onIpcClientServiceReady);

  connect(
      &m_pDeps->process(), &QProcessProxy::finished, this,
      &CoreProcess::onProcessFinished);

  connect(
      &m_pDeps->process(), &QProcessProxy::readyReadStandardOutput, this,
      &CoreProcess::onProcessReadyReadStandardOutput);

  connect(
      &m_pDeps->process(), &QProcessProxy::readyReadStandardError, this,
      &CoreProcess::onProcessReadyReadStandardError);
}

void CoreProcess::onIpcClientServiceReady() {
  if (m_processState == ProcessState::Starting) {
    qDebug("service ready, continuing core process start");
    start();
  } else if (m_processState == ProcessState::Stopping) {
    qDebug("service ready, continuing core process stop");
    stop();
  } else {
    qCritical("service ready, but process state is not starting or stopping");
  }
}

void CoreProcess::onIpcClientError(const QString &text) const {
  qCritical().noquote() << text;

  if (m_appConfig.processMode() != ProcessMode::kService) {
    // if not meant to be in service mode and there is an ipc connection error,
    // then abandon the ipc client connection.
    m_pDeps->ipcClient().disconnectFromHost();
  }
}

void CoreProcess::onIpcClientRead(const QString &text) { handleLogLines(text); }

void CoreProcess::onProcessReadyReadStandardOutput() {
  if (m_pDeps->process()) {
    handleLogLines(m_pDeps->process().readAllStandardOutput());
  }
}

void CoreProcess::onProcessReadyReadStandardError() {
  if (m_pDeps->process()) {
    handleLogLines(m_pDeps->process().readAllStandardError());
  }
}

void CoreProcess::onProcessFinished(int exitCode, QProcess::ExitStatus) {
  const auto wasStarted = m_processState == ProcessState::Started;

  setProcessState(ProcessState::Stopped);
  setConnectionState(ConnectionState::Disconnected);

  if (exitCode == 0) {
    qDebug("desktop process exited normally");
  } else {
    qWarning("desktop process exited with error code: %d", exitCode);
  }

  if (wasStarted) {
    qDebug("desktop process was running, retrying in %d ms", kRetryDelay);
    QTimer::singleShot(kRetryDelay, [this] { start(); });
  }
}

void CoreProcess::startDesktop(const QString &app, const QStringList &args) {
  using enum ProcessState;

  if (m_processState != Starting) {
    qFatal("core process must be in starting state");
  }

  // only make quoted args for printing the command for convenience; so that the
  // core command can be easily copy/pasted to the terminal for testing.
  const auto quoted = makeQuotedArgs(app, args);
  qInfo("running command: %s", qPrintable(quoted));

  m_pDeps->process().start(app, args);

  if (m_pDeps->process().waitForStarted()) {
    setProcessState(Started);
  } else {
    setProcessState(Stopped);
    emit error(Error::StartFailed);
  }
}

void CoreProcess::startService(const QString &app, const QStringList &args) {
  using enum ProcessState;

  if (m_processState != Starting) {
    qFatal("core process must be in starting state");
  }

  if (!m_pDeps->ipcClient().isConnected()) {
    // when service state changes, start will be called again.
    qDebug("cannot start process, ipc not connected, connecting instead");
    m_pDeps->ipcClient().connectToHost();
    return;
  }

  QString commandQuoted = makeQuotedArgs(app, args);

  qInfo("running command: %s", qPrintable(commandQuoted));
  m_pDeps->ipcClient().sendCommand(commandQuoted, m_appConfig.elevateMode());
  setProcessState(Started);
}

void CoreProcess::stopDesktop() const {
  if (m_processState != ProcessState::Stopping) {
    qFatal("core process must be in stopping state");
  }

  if (!m_pDeps->process()) {
    qFatal("process not set, cannot stop");
  }

  qInfo("stopping core desktop process");

  if (m_pDeps->process().state() == QProcess::ProcessState::Running) {
    qDebug("process is running, closing");
    m_pDeps->process().close();
  } else {
    qDebug("process is not running, skipping terminate");
  }
}

void CoreProcess::stopService() {
  if (m_processState != ProcessState::Stopping) {
    qFatal("core process must be in stopping state");
  }

  if (!m_pDeps->ipcClient().isConnected()) {
    qDebug("cannot stop process, ipc not connected");
    return;
  }

  m_pDeps->ipcClient().sendCommand("", m_appConfig.elevateMode());
  setProcessState(ProcessState::Stopped);
}

void CoreProcess::handleLogLines(const QString &text) {
  for (const auto &line : text.split(kLineSplitRegex)) {
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
    emit logLine(line);
  }
}

void CoreProcess::start(std::optional<ProcessMode> processModeOption) {
  QMutexLocker locker(&m_processMutex);

  const auto processMode =
      processModeOption.value_or(m_appConfig.processMode());

  qInfo(
      "starting core %s process (%s mode)", qPrintable(modeString()),
      qPrintable(processModeToString(processMode)));

  if (m_processState == ProcessState::Started) {
    qCritical("core process already started");
    return;
  }

  // allow external listeners to abort the start process (e.g. licensing issue).
  setProcessState(ProcessState::Starting);
  emit starting();
  if (m_processState == ProcessState::Stopped) {
    qDebug("core process start was cancelled by listener");
    return;
  }

#ifdef Q_OS_MAC
  requestOSXNotificationPermission();
#endif

  setConnectionState(ConnectionState::Connecting);

  QString app;
  QStringList args;

  args << "-f"
       << "--no-tray"
       << "--debug" << m_appConfig.logLevelText();

  args << "--name" << m_appConfig.screenName();

  if (processMode == ProcessMode::kDesktop) {
    m_pDeps->process().create();
  } else {
    // tell client/server to talk to daemon through ipc.
    args << "--ipc";

#if defined(Q_OS_WIN)
    // tell the client/server to shut down when a ms windows desk
    // is switched; this is because we may need to elevate or not
    // based on which desk the user is in (login always needs
    // elevation, where as default desk does not).
    // Note that this is only enabled when synergy is set to elevate
    // 'as needed' (e.g. on a UAC dialog popup) in order to prevent
    // unnecessary restarts when synergy was started elevated or
    // when it is not allowed to elevate. In these cases restarting
    // the server is fruitless.
    if (m_appConfig.elevateMode() == ElevateMode::kAutomatic) {
      args << "--stop-on-desk-switch";
    }
#endif
  }

#ifndef Q_OS_LINUX

  if (m_serverConfig.enableDragAndDrop()) {
    args << "--enable-drag-drop";
  }

#endif

  if (m_appConfig.tlsEnabled()) {
    args << "--enable-crypto";
    args << "--tls-cert" << m_appConfig.tlsCertPath();
  }

#if defined(Q_OS_WIN)
  // on windows, the profile directory changes depending on the user that
  // launched the process (e.g. when launched with elevation). setting the
  // profile dir on launch ensures it uses the same profile dir is used
  // no matter how its relaunched.
  args << "--profile-dir" << m_pDeps->getProfileRoot();
#endif

  if (m_appConfig.preventSleep()) {
    args << "--prevent-sleep";
  }

  if ((mode() == Mode::Client && !clientArgs(args, app)) ||
      (mode() == Mode::Server && !serverArgs(args, app))) {
    qDebug("failed to get args for core process, aborting start");
    return;
  }

  qDebug("log level: %s", qPrintable(m_appConfig.logLevelText()));

  if (m_appConfig.logToFile())
    qInfo("log file: %s", qPrintable(m_appConfig.logFilename()));

  if (processMode == ProcessMode::kDesktop) {
    startDesktop(app, args);
  } else if (processMode == ProcessMode::kService) {
    startService(app, args);
  }

  m_lastProcessMode = processMode;
}

void CoreProcess::stop(std::optional<ProcessMode> processModeOption) {
  QMutexLocker locker(&m_processMutex);

  const auto processMode =
      processModeOption.value_or(m_appConfig.processMode());

  qInfo(
      "stopping core process (%s mode)",
      qPrintable(processModeToString(processMode)));

  if (m_processState == ProcessState::Starting) {
    qDebug("core process is starting, cancelling");
    setProcessState(ProcessState::Stopped);
  } else if (m_processState != ProcessState::Stopped) {
    setProcessState(ProcessState::Stopping);

    if (processMode == ProcessMode::kService) {
      stopService();
    } else if (processMode == ProcessMode::kDesktop) {
      stopDesktop();
    }

  } else {
    qWarning("core process already stopped");
  }

  setConnectionState(ConnectionState::Disconnected);
}

void CoreProcess::restart() {
  using enum ProcessMode;

  qDebug("restarting core process");

  const auto processMode = m_appConfig.processMode();

  if (m_lastProcessMode != processMode) {
    if (processMode == kDesktop) {
      qDebug("process mode changed to desktop, stopping service process");
      stop(kService);
    } else if (processMode == kService) {
      qDebug("process mode changed to service, stopping desktop process");
      stop(kDesktop);
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

void CoreProcess::cleanup() {
  qInfo("cleaning up core process");

  const auto isDesktop = m_appConfig.processMode() == ProcessMode::kDesktop;
  const auto isRunning = m_processState == ProcessState::Started;
  if (isDesktop && isRunning) {
    stop();
  }

  m_pDeps->ipcClient().disconnectFromHost();
}

bool CoreProcess::serverArgs(QStringList &args, QString &app) {
  app = m_pDeps->appPath(m_appConfig.coreServerName());

  if (!m_pDeps->fileExists(app)) {
    qFatal("core server binary does not exist");
    return false;
  }

  if (m_appConfig.logToFile()) {
    m_appConfig.persistLogDir();

    args << "--log" << m_appConfig.logFilename();
  }

  QString configFilename = persistConfig();
  if (configFilename.isEmpty()) {
    qFatal("config file name empty for server args");
    return false;
  }

  if (m_appConfig.invertConnection()) {
    qDebug("inverting server connection");

    if (correctedAddress().isEmpty()) {
      emit error(Error::AddressMissing);
      qDebug("address is missing for server args");
      return false;
    }
  }

  // the address arg is dual purpose; when in listening mode, it's the address
  // that the server listens on. when tcp sockets are inverted, it connects to
  // that address. this is a bit confusing, and there should be probably be
  // different args for different purposes.
  args << "--address" << correctedInterface();

  args << "-c" << configFilename;
  qInfo("core config file: %s", qPrintable(configFilename));

  if (kEnableActivation && !m_appConfig.serialKey().isEmpty()) {
    args << "--serial-key" << m_appConfig.serialKey();
  }

  return true;
}

bool CoreProcess::clientArgs(QStringList &args, QString &app) {
  app = m_pDeps->appPath(m_appConfig.coreClientName());

  if (!m_pDeps->fileExists(app)) {
    qFatal("core client binary does not exist");
    return false;
  }

  if (m_appConfig.logToFile()) {
    m_appConfig.persistLogDir();
    args << "--log" << m_appConfig.logFilename();
  }

  if (m_appConfig.languageSync()) {
    args << "--sync-language";
  }

  if (m_appConfig.invertScrollDirection()) {
    args << "--invert-scroll";
  }

  if (m_appConfig.invertConnection()) {
    qDebug("inverting client connection");
    args << "--host";
    args << ":" + QString::number(m_appConfig.port());
  } else {

    if (correctedAddress().isEmpty()) {
      emit error(Error::AddressMissing);
      qDebug("address is missing for client args");
      return false;
    }

    args << correctedAddress() + ":" + QString::number(m_appConfig.port());
  }

  return true;
}

QString CoreProcess::persistConfig() const {
  QString configFullPath;
  if (m_appConfig.useExternalConfig()) {
    return m_appConfig.configFile();
  }

  for (auto path :
       {QStandardPaths::AppDataLocation, QStandardPaths::AppConfigLocation}) {
    auto configDirPath = QStandardPaths::writableLocation(path);
    if (!QDir().mkpath(configDirPath)) {
      qWarning("failed to create config folder: %s", qPrintable(configDirPath));
      continue;
    }

    QFile configFile(configDirPath + "/" + kLastConfigFilename);
    if (!configFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
      qWarning(
          "failed to open core config file: %s",
          qPrintable(configFile.fileName()));
      continue;
    }

    m_serverConfig.save(configFile);
    configFile.close();
    return configFile.fileName();
  }

  qFatal("failed to persist config file");
  return "";
}

QString CoreProcess::modeString() const {
  using enum Mode;

  switch (m_mode) {
  case Server:
    return "server";
  case Client:
    return "client";
  default:
    qFatal("invalid core mode");
    return "";
  }
}

void CoreProcess::setConnectionState(ConnectionState state) {
  if (m_connectionState == state) {
    return;
  }

  m_connectionState = state;
  emit connectionStateChanged(state);
}

void CoreProcess::setProcessState(ProcessState state) {
  if (m_processState == state) {
    return;
  }

  m_processState = state;
  emit processStateChanged(state);
}

void CoreProcess::checkLogLine(const QString &line) {
  using enum ConnectionState;

  if (line.contains("connected to server") || line.contains("has connected")) {
    setConnectionState(Connected);

  } else if (line.contains("started server")) {
    setConnectionState(Listening);
  } else if (
      line.contains("disconnected from server") ||
      line.contains("process exited")) {
    setConnectionState(Disconnected);
  } else if (line.contains("connecting to")) {
    setConnectionState(Connecting);
  }

  checkSecureSocket(line);

  // subprocess (synergys, synergyc) is not allowed to show notifications
  // process the log from it and show notification from synergy instead
#ifdef Q_OS_MAC
  checkOSXNotification(line);
#endif
}

bool CoreProcess::checkSecureSocket(const QString &line) {
  static const QString tlsCheckString = "network encryption protocol: ";
  const auto index = line.indexOf(tlsCheckString, 0, Qt::CaseInsensitive);
  if (index == -1) {
    return false;
  }

  emit secureSocket(true);
  m_secureSocketVersion = line.mid(index + tlsCheckString.size());
  return true;
}

#ifdef Q_OS_MAC
void CoreProcess::checkOSXNotification(const QString &line) {
  static const QString needle = "OSX Notification: ";
  if (line.contains(needle) && line.contains('|')) {
    int delimiterPosition = line.indexOf('|');
    int start = line.indexOf(needle);
    QString title = line.mid(
        start + needle.length(), delimiterPosition - start - needle.length());
    QString body =
        line.mid(delimiterPosition + 1, line.length() - delimiterPosition);
    if (!showOSXNotification(title, body)) {
      qDebug("osx notification was not shown");
    }
  }
}
#endif

QString CoreProcess::correctedInterface() const {
  QString interface = wrapIpv6(m_appConfig.networkInterface());
  return interface + ":" + QString::number(m_appConfig.port());
}

QString CoreProcess::correctedAddress() const { return wrapIpv6(m_address); }

} // namespace synergy::gui
