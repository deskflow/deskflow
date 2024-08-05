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

#include <QCoreApplication>
#include <QFile>
#include <QMessageBox>
#include <QMutexLocker>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTimer>
#include <qlogging.h>

namespace synergy::gui {

const int kRetryDelay = 1000;
const auto kLastConfigFilename = "LastConfig.cfg";
const auto kLineSplitRegex = QRegularExpression("\r|\n|\r\n");

CoreProcess::CoreProcess(AppConfig &appConfig, IServerConfig &serverConfig)
    : m_AppConfig(appConfig),
      m_ServerConfig(serverConfig) {

  connect(
      &m_IpcClient, &QIpcClient::readLogLine, this,
      &CoreProcess::onIpcClientReadLogLine);

  connect(
      &m_IpcClient, &QIpcClient::errorMessage, this,
      &CoreProcess::onIpcClientErrorMessage);

  connect(
      &m_IpcClient, &QIpcClient::infoMessage, this,
      &CoreProcess::onIpcClientInfoMessage);

#if defined(Q_OS_WIN)

  // TODO: only connect permenantly to ipc when switching to service mode.
  // if switching from service to desktop, connect only to stop the service
  // and don't retry.
  m_IpcClient.connectToHost();

#endif
}

CoreProcess::~CoreProcess() {
  try {
    if (m_AppConfig.processMode() == ProcessMode::kDesktop) {
      m_ExpectedRunningState = RuningState::Stopped;
      stopDesktop();
    }
  } catch (const std::exception &e) {
    qFatal("failed to stop core on destruction: %s", e.what());
  }
}

void CoreProcess::onCoreProcessRetryStart() {
  if (m_CoreState == CoreState::PendingRetry) {
    startCore();
  }
}

void CoreProcess::onIpcClientReadLogLine(const QString &line) {

  if (line.contains("connected to server") || line.contains("has connected")) {
    setCoreState(CoreState::Connected);

  } else if (line.contains("started server")) {
    setCoreState(CoreState::Listening);
  } else if (
      line.contains("disconnected from server") ||
      line.contains("process exited")) {
    setCoreState(CoreState::Disconnected);
  } else if (line.contains("connecting to")) {
    setCoreState(CoreState::Connecting);
  }

  emit logLine(line);
}

void CoreProcess::onIpcClientErrorMessage(const QString &text) {
  emit logError(text);
}

void CoreProcess::onIpcClientInfoMessage(const QString &text) {
  emit logInfo(text);
}

void CoreProcess::onCoreProcessFinished(int exitCode, QProcess::ExitStatus) {
  if (exitCode == 0) {
    emit logInfo("process exited normally");
  } else {
    emit logError(QString("process exited with error code: %1").arg(exitCode));
  }

  if (m_ExpectedRunningState == RuningState::Started) {

    if (m_CoreState != CoreState::PendingRetry) {
      QTimer::singleShot(
          kRetryDelay, this, &CoreProcess::onCoreProcessRetryStart);
      emit logInfo("detected process not running, auto restarting");
    } else {
      emit logInfo("detected process not running, already auto restarting");
    }

    setCoreState(CoreState::PendingRetry);
  } else {
    setCoreState(CoreState::Disconnected);
  }
}

void CoreProcess::onCoreProcessReadyReadStandardOutput() {
  if (!m_pCoreProcess) {
    return;
  }

  QString text = m_pCoreProcess->readAllStandardOutput();
  for (auto line : text.split(kLineSplitRegex)) {
    const auto trimmed = line.trimmed();

    if (trimmed.isEmpty()) {
      continue;
    }

    // TODO: should this go in stderr handling?
    // HACK: macOS 10.13.4+ spamming error lines in logs making them
    // impossible to read and debug; giving users a red herring.
    if (trimmed.contains("calling TIS/TSM in non-main thread environment")) {
      continue;
    }

    // only start if there is no active service running
    if (trimmed.contains("service status: idle") &&
        m_AppConfig.startedBefore()) {
      startCore();
    }

    emit logLine(trimmed);
  }
}

void CoreProcess::onCoreProcessReadyReadStandardError() {
  if (!m_pCoreProcess) {
    return;
  }

  QString text = m_pCoreProcess->readAllStandardError();
  for (auto line : text.split(kLineSplitRegex)) {
    const auto trimmed = line.trimmed();

    if (trimmed.isEmpty()) {
      continue;
    }

    emit logError(trimmed);
  }
}

void CoreProcess::startCore() {
  emit logInfo(QString("starting core %1 process").arg(coreModeString()));

#ifdef Q_OS_MAC
  requestOSXNotificationPermission();
#endif

  m_ExpectedRunningState = RuningState::Started;
  setCoreState(CoreState::Connecting);

  QString app;
  QStringList args;

  args << "-f"
       << "--no-tray"
       << "--debug" << m_AppConfig.logLevelText();

  args << "--name" << m_AppConfig.screenName();

  ProcessMode mode = m_AppConfig.processMode();

  if (mode == ProcessMode::kDesktop) {
    m_pCoreProcess = std::make_unique<QProcess>(this);
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
    if (m_AppConfig.elevateMode() == ElevateAsNeeded) {
      args << "--stop-on-desk-switch";
    }
#endif
  }

#ifndef Q_OS_LINUX

  if (m_ServerConfig.enableDragAndDrop()) {
    args << "--enable-drag-drop";
  }

#endif

#if defined(Q_OS_WIN)
  if (m_AppConfig.tlsEnabled()) {
    args << "--enable-crypto";
    args << "--tls-cert" << m_AppConfig.tlsCertPath();
  }

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

#else
  if (m_AppConfig.tlsEnabled()) {
    args << "--enable-crypto";
    args << "--tls-cert" << m_AppConfig.tlsCertPath();
  }
#endif

  if (m_AppConfig.preventSleep()) {
    args << "--prevent-sleep";
  }

  if ((coreMode() == CoreMode::Client && !clientArgs(args, app)) ||
      (coreMode() == CoreMode::Server && !serverArgs(args, app))) {
    qDebug("failed to get args for core process, aborting start");
    return;
  }

  if (mode == ProcessMode::kDesktop) {
    connect(
        m_pCoreProcess.get(), &QProcess::finished, this,
        &CoreProcess::onCoreProcessFinished);
    connect(
        m_pCoreProcess.get(), &QProcess::readyReadStandardOutput, this,
        &CoreProcess::onCoreProcessReadyReadStandardOutput);
    connect(
        m_pCoreProcess.get(), &QProcess::readyReadStandardError, this,
        &CoreProcess::onCoreProcessReadyReadStandardError);
  }

  if (m_AppConfig.logLevel() >= kDebugLogLevel) {
    emit logInfo(QString("command: %1 %2").arg(app, args.join(" ")));
  }

  emit logInfo("log level: " + m_AppConfig.logLevelText());

  if (m_AppConfig.logToFile())
    emit logInfo("log file: " + m_AppConfig.logFilename());

  if (mode == ProcessMode::kDesktop) {
    m_pCoreProcess->start(app, args);
    if (!m_pCoreProcess->waitForStarted()) {
      emit error(Error::StartFailed);
    }
  } else if (mode == ProcessMode::kService) {
    QString command(app + " " + args.join(" "));
    m_IpcClient.sendCommand(command, m_AppConfig.elevateMode());
  }
}

void CoreProcess::stopCore() {
  emit logInfo("stopping core process");

  m_ExpectedRunningState = RuningState::Stopped;

  if (m_AppConfig.processMode() == ProcessMode::kService) {
    stopService();
  } else if (m_AppConfig.processMode() == ProcessMode::kDesktop) {
    stopDesktop();
  }

  setCoreState(CoreState::Disconnected);
}

void CoreProcess::stopService() {
  // send empty command to stop service from laucning anything.
  m_IpcClient.sendCommand("", m_AppConfig.elevateMode());
}

void CoreProcess::stopDesktop() {
  QMutexLocker locker(&m_StopDesktopMutex);
  if (!m_pCoreProcess) {
    return;
  }

  emit logInfo("stopping synergy desktop process");

  if (m_pCoreProcess->isOpen()) {
    m_pCoreProcess->close();
  }

  m_pCoreProcess->reset();
}

QString CoreProcess::appPath(const QString &name) const {
  QDir dir(QCoreApplication::applicationDirPath());
  return dir.filePath(name);
}

bool CoreProcess::serverArgs(QStringList &args, QString &app) {
  app = appPath(m_AppConfig.coreServerName());

  if (!QFile::exists(app)) {
    qFatal("core server binary does not exist.");
    return false;
  }

  if (m_AppConfig.invertConnection() && m_Address.isEmpty()) {
    emit error(Error::AddressMissing);
    return false;
  }

  if (m_AppConfig.logToFile()) {
    m_AppConfig.persistLogDir();

    args << "--log" << m_AppConfig.logFilename();
  }

  QString configFilename = persistConfig();
  if (configFilename.isEmpty()) {
    return false;
  }

  args << "-c" << configFilename << "--address" << address();
  emit logInfo("config file: " + configFilename);

  if (kLicensingEnabled && !m_AppConfig.serialKey().isEmpty()) {
    args << "--serial-key" << m_AppConfig.serialKey();
  }

  return true;
}

bool CoreProcess::clientArgs(QStringList &args, QString &app) {
  app = appPath(m_AppConfig.coreClientName());

  if (!QFile::exists(app)) {
    qFatal("core client binary does not exist.");
    return false;
  }

  if (m_AppConfig.logToFile()) {
    m_AppConfig.persistLogDir();
    args << "--log" << m_AppConfig.logFilename();
  }

  if (m_AppConfig.languageSync()) {
    args << "--sync-language";
  }

  if (m_AppConfig.invertScrollDirection()) {
    args << "--invert-scroll";
  }

  if (m_AppConfig.invertConnection()) {
    args << "--host";
    args << ":" + QString::number(m_AppConfig.port());
  } else {
    if (m_Address.isEmpty()) {
      emit error(Error::AddressMissing);
      return false;
    }

    // if interface is IPv6 - ensure that ip is in square brackets
    if (m_Address.count(':') > 1) {
      if (m_Address[0] != '[') {
        m_Address.insert(0, '[');
      }
      if (m_Address[m_Address.size() - 1] != ']') {
        m_Address.push_back(']');
      }
    }
    args << m_Address + ":" + QString::number(m_AppConfig.port());
  }

  return true;
}

QString CoreProcess::persistConfig() const {
  QString configFullPath;
  if (m_AppConfig.useExternalConfig()) {
    return m_AppConfig.configFile();
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

    m_ServerConfig.save(configFile);
    configFile.close();
    return configFile.fileName();
  }

  qFatal("failed to persist config file");
  return "";
}

QString CoreProcess::address() const {
  QString i = m_AppConfig.networkInterface();
  // if interface is IPv6 - ensure that ip is in square brackets
  if (i.count(':') > 1) {
    if (i[0] != '[') {
      i.insert(0, '[');
    }
    if (i[i.size() - 1] != ']') {
      i.push_back(']');
    }
  }
  return (!i.isEmpty() ? i : "") + ":" + QString::number(m_AppConfig.port());
}

QString CoreProcess::coreModeString() const {
  using enum CoreMode;

  switch (coreMode()) {
  case Server:
    return "server";
  case Client:
    return "client";
  default:
    qFatal("invalid core mode");
  }
}

void CoreProcess::setCoreState(CoreState state) {
  if (m_CoreState == state) {
    return;
  }

  m_CoreState = state;
  emit stateChanged(state);
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

bool CoreProcess::isCoreActive() const {
  using enum CoreState;
  auto state = m_CoreState;
  return (state == Connected) || (state == Connecting) || (state == Listening);
}

} // namespace synergy::gui
