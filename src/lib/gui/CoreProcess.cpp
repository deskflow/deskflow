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

#include "AppConfig.h"
#include "constants.h"
#include <qlogging.h>

#if defined(Q_OS_MAC)
#include "OSXHelpers.h"
#endif

#include <QCoreApplication>
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

CoreProcess::CoreProcess(AppConfig &appConfig, IServerConfig &serverConfig)
    : m_appConfig(appConfig),
      m_serverConfig(serverConfig) {

  connect(
      &m_ipcClient, &QIpcClient::read, this, //
      &CoreProcess::onIpcClientRead);

  connect(
      &m_ipcClient, &QIpcClient::serviceReady, this, //
      &CoreProcess::onIpcClientServiceReady);
}

CoreProcess::~CoreProcess() {
  try {
    if (m_appConfig.processMode() == ProcessMode::kDesktop) {
      stop();
    }
  } catch (const std::exception &e) {
    qFatal("failed to stop core on destruction: %s", e.what());
  }
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

void CoreProcess::onIpcClientError(const QString &text) {
  qCritical().noquote() << text;

  if (m_appConfig.processMode() != ProcessMode::kService) {
    // if not meant to be in service mode and there is an ipc connection error,
    // then abandon the ipc client connection.
    m_ipcClient.disconnectFromHost();
  }
}

void CoreProcess::onIpcClientRead(const QString &text) { handleLogLines(text); }

void CoreProcess::onProcessReadyReadStandardOutput() {
  if (m_pProcess) {
    handleLogLines(m_pProcess->readAllStandardOutput());
  }
}

void CoreProcess::onProcessReadyReadStandardError() {
  if (m_pProcess) {
    handleLogLines(m_pProcess->readAllStandardError());
  }
}

void CoreProcess::onProcessFinished(int exitCode, QProcess::ExitStatus) {
  const auto wasStarted = m_processState == ProcessState::Started;

  setProcessState(ProcessState::Stopped);
  setConnectionState(ConnectionState::Disconnected);

  m_pProcess->reset();

  if (exitCode == 0) {
    qDebug("desktop process exited normally");
  } else {
    qCritical("desktop process exited with error code: %d", exitCode);
  }

  if (wasStarted) {
    qDebug("desktop process was running, retrying in %d ms", kRetryDelay);
    QTimer::singleShot(kRetryDelay, [this] { start(); });
  }
}

void CoreProcess::startDesktop(const QString &app, const QStringList &args) {
  if (m_processState != ProcessState::Starting) {
    qFatal("core process must be in starting state");
  }

  m_pProcess->start(app, args);
  if (m_pProcess->waitForStarted()) {
    setProcessState(ProcessState::Started);
  } else {
    setProcessState(ProcessState::Stopped);
    emit error(Error::StartFailed);
  }
}

void CoreProcess::startService(const QString &app, const QStringList &args) {
  if (m_processState != ProcessState::Starting) {
    qFatal("core process must be in starting state");
  }

  if (!m_ipcClient.isConnected()) {
    // when service state changes, start will be called again.
    qDebug("cannot start process, ipc not connected, connecting instead");
    m_ipcClient.connectToHost();
    return;
  }

  QStringList command;

  // wrap app in quotes to handle spaces in paths (e.g. "C:\Program Files").
  command << QString(R"("%1")").arg(app);

  for (const auto &arg : args) {
    if (arg.startsWith("-")) {
      command << arg;
    } else {
      // wrap opt args in quotes to handle spaces in paths.
      command << QString(R"("%1")").arg(arg);
    }
  }

  m_ipcClient.sendCommand(command.join(" "), m_appConfig.elevateMode());
  setProcessState(ProcessState::Started);
}

void CoreProcess::stopDesktop() {
  if (m_processState != ProcessState::Stopping) {
    qFatal("core process must be in stopping state");
  }

  if (!m_pProcess) {
    qFatal("process not set, cannot stop");
  }

  qInfo("stopping core desktop process");

  if (m_pProcess->state() == QProcess::ProcessState::Running) {
    qDebug("process is running, closing");
    m_pProcess->close();
  } else {
    qDebug("process is not running, skipping terminate");
  }
}

void CoreProcess::stopService() {
  if (m_processState != ProcessState::Stopping) {
    qFatal("core process must be in stopping state");
  }

  if (!m_ipcClient.isConnected()) {
    qDebug("cannot stop process, ipc not connected");
    return;
  }

  m_ipcClient.sendCommand("", m_appConfig.elevateMode());
  setProcessState(ProcessState::Stopped);
}

void CoreProcess::handleLogLines(const QString &text) {
  for (auto line : text.split(kLineSplitRegex)) {
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
      qPrintable(processModeString()));

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
    m_pProcess = std::make_unique<QProcess>(this);
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
    if (m_appConfig.elevateMode() == ElevateAsNeeded) {
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
  try {
    // on windows, the profile directory changes depending on the user that
    // launched the process (e.g. when launched with elevation). setting the
    // profile dir on launch ensures it uses the same profile dir is used
    // no matter how its relaunched.
    args << "--profile-dir" << getProfileRootForArg();
  } catch (const std::exception &e) {
    qDebug() << e.what();
    qFatal("failed to get profile dir, skipping arg");
  }
#endif

  if (m_appConfig.preventSleep()) {
    args << "--prevent-sleep";
  }

  if ((mode() == Mode::Client && !clientArgs(args, app)) ||
      (mode() == Mode::Server && !serverArgs(args, app))) {
    qDebug("failed to get args for core process, aborting start");
    return;
  }

  if (processMode == ProcessMode::kDesktop) {
    connect(
        m_pProcess.get(), &QProcess::finished, this,
        &CoreProcess::onProcessFinished);
    connect(
        m_pProcess.get(), &QProcess::readyReadStandardOutput, this,
        &CoreProcess::onProcessReadyReadStandardOutput);
    connect(
        m_pProcess.get(), &QProcess::readyReadStandardError, this,
        &CoreProcess::onProcessReadyReadStandardError);
  }

  if (m_appConfig.logLevel() >= kDebugLogLevel) {
    qInfo("command: %s %s", qPrintable(app), qPrintable(args.join(" ")));
  }

  // qInfo("log level: " + m_appConfig.logLevelText());
  qInfo("log level: %s", qPrintable(m_appConfig.logLevelText()));

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

  qInfo("stopping core process (%s mode)", qPrintable(processModeString()));

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
  qDebug("restarting core process");

  const auto processMode = m_appConfig.processMode();

  if (m_lastProcessMode != processMode) {
    if (processMode == ProcessMode::kDesktop) {
      qDebug("process mode changed to desktop, stopping service process");
      stop(ProcessMode::kService);
    } else if (processMode == ProcessMode::kService) {
      qDebug("process mode changed to service, stopping desktop process");
      stop(ProcessMode::kDesktop);
    }
  }

  // in service mode: though there is technically no need to stop the service
  // before restarting it, it does make for cleaner process state tracking,
  // especially if something goes wrong with starting the service.
  stop();
  start();
}

QString CoreProcess::appPath(const QString &name) const {
  QDir dir(QCoreApplication::applicationDirPath());
  return dir.filePath(name);
}

bool CoreProcess::serverArgs(QStringList &args, QString &app) {
  app = appPath(m_appConfig.coreServerName());

  if (!QFile::exists(app)) {
    qFatal("core server binary does not exist.");
    return false;
  }

  if (m_appConfig.invertConnection() && m_address.isEmpty()) {
    emit error(Error::AddressMissing);
    return false;
  }

  if (m_appConfig.logToFile()) {
    m_appConfig.persistLogDir();

    args << "--log" << m_appConfig.logFilename();
  }

  QString configFilename = persistConfig();
  if (configFilename.isEmpty()) {
    return false;
  }

  args << "-c" << configFilename << "--address" << address();
  qInfo("config file: %s", qPrintable(configFilename));

  if (kEnableActivation && !m_appConfig.serialKey().isEmpty()) {
    args << "--serial-key" << m_appConfig.serialKey();
  }

  return true;
}

bool CoreProcess::clientArgs(QStringList &args, QString &app) {
  app = appPath(m_appConfig.coreClientName());

  if (!QFile::exists(app)) {
    qFatal("core client binary does not exist.");
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
    args << "--host";
    args << ":" + QString::number(m_appConfig.port());
  } else {
    if (m_address.isEmpty()) {
      emit error(Error::AddressMissing);
      return false;
    }

    // if interface is IPv6 - ensure that ip is in square brackets
    if (m_address.count(':') > 1) {
      if (m_address[0] != '[') {
        m_address.insert(0, '[');
      }
      if (m_address[m_address.size() - 1] != ']') {
        m_address.push_back(']');
      }
    }
    args << m_address + ":" + QString::number(m_appConfig.port());
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
          "failed to open config file: %s", qPrintable(configFile.fileName()));
      continue;
    }

    m_serverConfig.save(configFile);
    configFile.close();
    return configFile.fileName();
  }

  qFatal("failed to persist config file");
  return "";
}

QString CoreProcess::address() const {
  QString i = m_appConfig.networkInterface();
  // if interface is IPv6 - ensure that ip is in square brackets
  if (i.count(':') > 1) {
    if (i[0] != '[') {
      i.insert(0, '[');
    }
    if (i[i.size() - 1] != ']') {
      i.push_back(']');
    }
  }
  return (!i.isEmpty() ? i : "") + ":" + QString::number(m_appConfig.port());
}

QString CoreProcess::modeString() const {
  using enum Mode;

  switch (mode()) {
  case Server:
    return "server";
  case Client:
    return "client";
  default:
    qFatal("invalid core mode");
    return "";
  }
}

QString CoreProcess::processModeString() const {
  using enum ProcessMode;

  switch (m_appConfig.processMode()) {
  case kDesktop:
    return "desktop";
  case kService:
    return "service";
  default:
    qFatal("invalid process mode");
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

QString CoreProcess::getProfileRootForArg() const {
  CoreInterface coreInterface;
  QString dir = coreInterface.getProfileDir();

  // HACK: strip our app name since we're returning the root dir.
#if defined(Q_OS_WIN)
  dir.replace("\\Synergy", "");
#else
  dir.replace("/.synergy", "");
#endif

  return dir;
}

void CoreProcess::checkLogLine(const QString &line) {

  if (line.contains("connected to server") || line.contains("has connected")) {
    setConnectionState(ConnectionState::Connected);

  } else if (line.contains("started server")) {
    setConnectionState(ConnectionState::Listening);
  } else if (
      line.contains("disconnected from server") ||
      line.contains("process exited")) {
    setConnectionState(ConnectionState::Disconnected);
  } else if (line.contains("connecting to")) {
    setConnectionState(ConnectionState::Connecting);
  }

  checkSecureSocket(line);

  // subprocess (synergys, synergyc) is not allowed to show notifications
  // process the log from it and show notificatino from synergy instead
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
  static const QString OSXNotificationSubstring = "OSX Notification: ";
  if (line.contains(OSXNotificationSubstring) && line.contains('|')) {
    int delimterPosition = line.indexOf('|');
    int notificationStartPosition = line.indexOf(OSXNotificationSubstring);
    QString title = line.mid(
        notificationStartPosition + OSXNotificationSubstring.length(),
        delimterPosition - notificationStartPosition -
            OSXNotificationSubstring.length());
    QString body =
        line.mid(delimterPosition + 1, line.length() - delimterPosition);
    if (!showOSXNotification(title, body)) {
      qDebug("OSX notification was not shown");
    }
  }
}
#endif

} // namespace synergy::gui
