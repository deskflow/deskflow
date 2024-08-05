/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Symless Ltd.
 * Copyright (C) 2008 Volker Lanz (vl@fidra.de)
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

#include "MainWindow.h"

#include "AboutDialog.h"
#include "ActivationDialog.h"
#include "ServerConfigDialog.h"
#include "SettingsDialog.h"
#include "TrayIcon.h"
#include "gui/ConfigScopes.h"
#include "gui/LicenseHandler.h"
#include "gui/TlsFingerprint.h"
#include "gui/VersionChecker.h"
#include "gui/constants.h"
#include "gui/license_notices.h"
#include "gui/messages.h"
#include "gui/styles.h"
#include "license/License.h"

#if defined(Q_OS_MAC)
#include "OSXHelpers.h"
#endif

#include <QApplication>
#include <QDesktopServices>
#include <QFileDialog>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QPushButton>
#include <QRegularExpression>
#include <QtCore>
#include <QtGui>
#include <QtNetwork>
#include <memory>

#if defined(Q_OS_MAC)
#include <ApplicationServices/ApplicationServices.h>
#endif

using namespace synergy::gui;
using namespace synergy::license;

static const int kRetryDelay = 1000;
static const int kDebugLogLevel = 1;

#if defined(Q_OS_MAC)

static const char *const kLightIconFiles[] = {
    ":/res/icons/64x64/synergy-light-disconnected.png",
    ":/res/icons/64x64/synergy-light-disconnected.png",
    ":/res/icons/64x64/synergy-light-connected.png",
    ":/res/icons/64x64/synergy-light-transfering.png",
    ":/res/icons/64x64/synergy-light-disconnected.png"};

static const char *const kDarkIconFiles[] = {
    ":/res/icons/64x64/synergy-dark-disconnected.png",
    ":/res/icons/64x64/synergy-dark-disconnected.png",
    ":/res/icons/64x64/synergy-dark-connected.png",
    ":/res/icons/64x64/synergy-dark-transfering.png",
    ":/res/icons/64x64/synergy-dark-disconnected.png"};

#endif

static const char *const kDefaultIconFiles[] = {
    ":/res/icons/16x16/synergy-disconnected.png",
    ":/res/icons/16x16/synergy-disconnected.png",
    ":/res/icons/16x16/synergy-connected.png",
    ":/res/icons/16x16/synergy-transfering.png",
    ":/res/icons/16x16/synergy-disconnected.png"};

MainWindow::MainWindow(ConfigScopes &configScopes, AppConfig &appConfig)
    : m_ConfigScopes(configScopes),
      m_AppConfig(appConfig),
      m_ServerConfig(appConfig, *this),
      m_ServerConnection(*this),
      m_ClientConnection(*this),
      m_TlsUtility(appConfig, m_LicenseHandler.license()),
      m_WindowSaveTimer(this) {

  setupUi(this);
  setupControls();
  connectSlots();

  // handled by `onCreated`
  emit created();
}

MainWindow::~MainWindow() {
  try {
    if (m_AppConfig.processMode() == ProcessMode::kDesktop) {
      m_ExpectedRunningState = RuningState::Stopped;
      stopDesktop();
    }
  } catch (const std::exception &e) {
    qFatal("failed to stop core on main window close: %s", e.what());
  }

  try {
    saveWindow();
  } catch (const std::exception &e) {
    qFatal("failed to save window on main window close: %s", e.what());
  }
}

void MainWindow::restoreWindow() {

  const auto &windowSize = m_AppConfig.mainWindowSize();
  if (windowSize.has_value()) {
    qDebug("restoring main window size");
    resize(windowSize.value());
  }

  const auto &windowPosition = m_AppConfig.mainWindowPosition();
  if (windowPosition.has_value()) {
    qDebug("restoring main window position");
    move(windowPosition.value());
  }

  // give the window chance to restore its size and position before the window
  // size and position are saved. this prevents the window from being saved
  // with the wrong size and position.
  m_SaveWindow = true;
}

void MainWindow::saveWindow() {
  if (!m_SaveWindow) {
    qDebug("not yet ready to save window size and position, skipping");
    return;
  }

  qDebug("saving window size and position");
  m_AppConfig.setMainWindowSize(size());
  m_AppConfig.setMainWindowPosition(pos());
  m_ConfigScopes.save();
}

void MainWindow::setupControls() {
  createMenuBar();
  secureSocket(false);

  m_pLabelUpdate->setStyleSheet(kStyleNoticeLabel);
  m_pLabelUpdate->hide();

  m_pLabelNotice->setStyleSheet(kStyleNoticeLabel);
  m_pLabelNotice->hide();

  m_pLabelIpAddresses->setText(
      QString("This computer's IP addresses: %1").arg(getIPAddresses()));

  if (m_AppConfig.lastVersion() != SYNERGY_VERSION) {
    m_AppConfig.setLastVersion(SYNERGY_VERSION);
  }

  if (kLicensingEnabled) {
    m_pActivate->setVisible(true);
  }

#if defined(Q_OS_MAC)

  m_pRadioGroupServer->setAttribute(Qt::WA_MacShowFocusRect, 0);
  m_pRadioGroupClient->setAttribute(Qt::WA_MacShowFocusRect, 0);

#endif
}

//////////////////////////////////////////////////////////////////////////////
// Begin slots
//////////////////////////////////////////////////////////////////////////////

// remember: using queued connection allows the render loop to run before
// executing the slot. the default is to instantly call the slot when the
// signal is emitted from the thread that owns the receiver's object.
void MainWindow::connectSlots() const {

  connect(this, &MainWindow::created, this, &MainWindow::onCreated);

  connect(
      this, &MainWindow::shown, this, &MainWindow::onShown,
      Qt::QueuedConnection);

  connect(
      &m_ConfigScopes, &ConfigScopes::saving, this,
      &MainWindow::onConfigScopesSaving, Qt::DirectConnection);

  connect(
      &m_AppConfig, &AppConfig::tlsChanged, this,
      &MainWindow::onAppConfigTlsChanged);

  connect(
      &m_AppConfig, &AppConfig::screenNameChanged, this,
      &MainWindow::onAppConfigScreenNameChanged);

  connect(
      &m_AppConfig, &AppConfig::invertConnectionChanged, this,
      &MainWindow::onAppConfigInvertConnection);

  connect(
      &m_LicenseHandler, &LicenseHandler::serialKeyChanged, this,
      &MainWindow::onLicenseHandlerSerialKeyChanged);

  connect(
      &m_LicenseHandler, &LicenseHandler::invalidLicense, this,
      &MainWindow::onLicenseHandlerInvalidLicense);

  connect(
      &m_IpcClient, &QIpcClient::readLogLine, this,
      &MainWindow::onIpcClientReadLogLine);

  connect(
      &m_IpcClient, &QIpcClient::errorMessage, this,
      &MainWindow::onIpcClientErrorMessage);

  connect(
      &m_IpcClient, &QIpcClient::infoMessage, this,
      &MainWindow::onIpcClientInfoMessage);

  connect(m_pActionMinimize, &QAction::triggered, this, &MainWindow::hide);

  connect(m_pActionRestore, &QAction::triggered, this, &MainWindow::showNormal);

  connect(
      m_pActionStartCore, &QAction::triggered, this,
      &MainWindow::onActionStartCoreTriggered);

  connect(
      m_pActionStopCore, &QAction::triggered, this,
      &MainWindow::onActionStopCoreTriggered);

  connect(m_pActionQuit, &QAction::triggered, qApp, &QCoreApplication::quit);

  connect(
      &m_VersionChecker, &VersionChecker::updateFound, this,
      &MainWindow::onVersionCheckerUpdateFound);

  connect(
      &m_WindowSaveTimer, &QTimer::timeout, this,
      &MainWindow::onWindowSaveTimerTimeout);

  connect(
      &m_TrayIcon, &TrayIcon::activated, this,
      &MainWindow::onTrayIconActivated);
}

void MainWindow::onAppAboutToQuit() { m_ConfigScopes.save(); }

void MainWindow::onCreated() {

#if defined(Q_OS_WIN)

  // TODO: only connect permenantly to ipc when switching to service mode.
  // if switching from service to desktop, connect only to stop the service
  // and don't retry.
  m_IpcClient.connectToHost();

#endif

  m_ConfigScopes.signalReady();

  applyCloseToTray();

  if (kLicensingEnabled && !m_AppConfig.serialKey().isEmpty()) {
    m_LicenseHandler.changeSerialKey(m_AppConfig.serialKey());
  }

  updateScreenName();
  applyConfig();
  restoreWindow();
}

void MainWindow::onShown() {
  if (kLicensingEnabled) {
    const auto &license = m_LicenseHandler.license();
    if (!m_AppConfig.activationHasRun() || !license.isValid() ||
        license.isExpired()) {
      showActivationDialog();
    }
  }
}

void MainWindow::onLicenseHandlerSerialKeyChanged(const QString &serialKey) {
  setWindowTitle(m_LicenseHandler.productName());
  showLicenseNotice();

  if (m_AppConfig.serialKey() != serialKey) {
    m_AppConfig.setSerialKey(serialKey);
    m_ConfigScopes.save();
  }
}

void MainWindow::onLicenseHandlerInvalidLicense() {
  stopCore();
  showActivationDialog();
}

void MainWindow::onConfigScopesSaving() { m_ServerConfig.commit(); }

void MainWindow::onAppConfigTlsChanged() {
  updateLocalFingerprint();
  if (m_TlsUtility.isAvailableAndEnabled()) {
    m_TlsUtility.generateCertificate(true);
  }
}

void MainWindow::onIpcClientReadLogLine(const QString &text) {
  processCoreLogLine(text);
}

void MainWindow::onIpcClientErrorMessage(const QString &text) {
  appendLogError(text);
}

void MainWindow::onIpcClientInfoMessage(const QString &text) {
  appendLogInfo(text);
}

void MainWindow::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason) {

  // HACK: setting the main window icon to disconnected when the tray is
  // activated seems odd.
  setIcon(CoreState::Disconnected);

  if (reason == QSystemTrayIcon::DoubleClick) {
    if (isVisible()) {
      hide();
    } else {
      showNormal();
      activateWindow();
    }
  }
}

void MainWindow::onCoreProcessReadyReadStandardOutput() {
  if (m_pCoreProcess) {
    QString text(m_pCoreProcess->readAllStandardOutput());
    for (QString line : text.split(QRegularExpression("\r|\n|\r\n"))) {
      if (!line.isEmpty()) {
        processCoreLogLine(line);
      }
    }
  }
}

void MainWindow::onCoreProcessReadyReadStandardError() {
  if (m_pCoreProcess) {
    processCoreLogLine(m_pCoreProcess->readAllStandardError());
  }
}

void MainWindow::onVersionCheckerUpdateFound(const QString &version) {
  const auto link = QString(kLinkDownload).arg(kUrlDownload, kColorWhite);
  const auto text =
      QString("A new version is available (v%1). %2").arg(version, link);

  m_pLabelUpdate->show();
  m_pLabelUpdate->setText(text);
}

void MainWindow::onActionStartCoreTriggered() {
  m_ClientConnection.setCheckConnection(true);
  startCore();
}

void MainWindow::onCoreProcessRetryStart() {
  // This function is only called after a failed start
  // Only start synergy if the current state is pending retry
  if (m_CoreState == CoreState::PendingRetry) {
    startCore();
  }
}

void MainWindow::onActionStopCoreTriggered() { stopCore(); }

void MainWindow::onAppConfigScreenNameChanged() { updateScreenName(); }

void MainWindow::onAppConfigInvertConnection() { applyConfig(); }

void MainWindow::onCoreProcessFinished(int exitCode, QProcess::ExitStatus) {
  if (exitCode == 0) {
    appendLogInfo("process exited normally");
  } else {
    appendLogError(QString("process exited with error code: %1").arg(exitCode));
  }

  if (m_ExpectedRunningState == RuningState::Started) {

    if (m_CoreState != CoreState::PendingRetry) {
      QTimer::singleShot(
          kRetryDelay, this, &MainWindow::onCoreProcessRetryStart);
      appendLogInfo("detected process not running, auto restarting");
    } else {
      appendLogInfo("detected process not running, already auto restarting");
    }

    setCoreState(CoreState::PendingRetry);
  } else {
    setCoreState(CoreState::Disconnected);
  }
}

bool MainWindow::on_m_pActionSave_triggered() {
  QString fileName =
      QFileDialog::getSaveFileName(this, QString("Save configuration as..."));

  if (!fileName.isEmpty() && !m_ServerConfig.save(fileName)) {
    QMessageBox::warning(
        this, QString("Save failed"),
        QString("Could not save configuration to file."));
    return true;
  }

  return false;
}

void MainWindow::on_m_pActionAbout_triggered() {
  AboutDialog about(this);
  about.exec();
}

void MainWindow::on_m_pActionHelp_triggered() {
  QDesktopServices::openUrl(QUrl(kUrlHelp));
}

void MainWindow::on_m_pActionSettings_triggered() {
  auto result =
      SettingsDialog(this, m_AppConfig, m_LicenseHandler.license()).exec();
  if (result == QDialog::Accepted) {
    m_ConfigScopes.save();

    applyConfig();
    applyCloseToTray();

    if (isCoreActive()) {
      restartCore();
    }
  }
}

void MainWindow::on_m_pButtonConfigureServer_clicked() {
  showConfigureServer();
}

void MainWindow::on_m_pActivate_triggered() { showActivationDialog(); }

void MainWindow::on_m_pLineEditHostname_returnPressed() {
  m_pButtonConnect->click();
}

void MainWindow::on_m_pLineEditClientIp_returnPressed() {
  m_pButtonConnectToClient->click();
}

void MainWindow::on_m_pButtonApply_clicked() {
  m_ClientConnection.setCheckConnection(true);
  restartCore();
}

void MainWindow::on_m_pLabelComputerName_linkActivated(const QString &) {
  m_pActionSettings->trigger();
}

void MainWindow::on_m_pLabelFingerprint_linkActivated(const QString &) {
  QMessageBox::information(
      this, "SSL/TLS fingerprint", TlsFingerprint::local().readFirst());
}

void MainWindow::on_m_pRadioGroupServer_clicked(bool) {
  enableServer(true);
  enableClient(false);
  m_ConfigScopes.save();
}

void MainWindow::on_m_pRadioGroupClient_clicked(bool) {
  enableClient(true);
  enableServer(false);
  m_ConfigScopes.save();
}

void MainWindow::on_m_pButtonConnect_clicked() { on_m_pButtonApply_clicked(); }

void MainWindow::on_m_pButtonConnectToClient_clicked() {
  on_m_pButtonApply_clicked();
}

void MainWindow::onWindowSaveTimerTimeout() { saveWindow(); }

//////////////////////////////////////////////////////////////////////////////
// End slots
//////////////////////////////////////////////////////////////////////////////

void MainWindow::resizeEvent(QResizeEvent *event) {
  QMainWindow::resizeEvent(event);

  // postpone save so that settings are not written every delta change.
  m_WindowSaveTimer.setSingleShot(true);
  m_WindowSaveTimer.start(1000);
}

void MainWindow::moveEvent(QMoveEvent *event) {
  QMainWindow::moveEvent(event);

  // postpone save so that settings are not written every delta change.
  m_WindowSaveTimer.setSingleShot(true);
  m_WindowSaveTimer.start(1000);
}

void MainWindow::open() {

  std::vector<QAction *> trayMenu = {
      m_pActionStartCore, m_pActionStopCore, nullptr,      m_pActionMinimize,
      m_pActionRestore,   nullptr,           m_pActionQuit};

  m_TrayIcon.create(trayMenu);

  if (m_AppConfig.autoHide()) {
    hide();
  } else {
    showNormal();
  }

  m_VersionChecker.checkLatest();

  // only start if user has previously started. this stops the gui from
  // auto hiding before the user has configured synergy (which of course
  // confuses first time users, who think synergy has crashed).
  if (m_AppConfig.startedBefore() &&
      m_AppConfig.processMode() == ProcessMode::kDesktop) {
    startCore();
  }
}

void MainWindow::setStatus(const QString &status) {
  m_pStatusLabel->setText(status);
}

void MainWindow::createMenuBar() {
  m_pMenuBar = new QMenuBar(this);
  m_pMenuFile = new QMenu("File", m_pMenuBar);
  m_pMenuEdit = new QMenu("Edit", m_pMenuBar);
  m_pMenuWindow = new QMenu("Window", m_pMenuBar);
  m_pMenuHelp = new QMenu("Help", m_pMenuBar);

  m_pMenuBar->addAction(m_pMenuFile->menuAction());
  m_pMenuBar->addAction(m_pMenuEdit->menuAction());
#if !defined(Q_OS_MAC)
  m_pMenuBar->addAction(m_pMenuWindow->menuAction());
#endif
  m_pMenuBar->addAction(m_pMenuHelp->menuAction());

  m_pMenuFile->addAction(m_pActionStartCore);
  m_pMenuFile->addAction(m_pActionStopCore);
  m_pMenuFile->addSeparator();
  m_pMenuFile->addAction(m_pActivate);
  m_pMenuFile->addSeparator();
  m_pMenuFile->addAction(m_pActionSave);
  m_pMenuFile->addSeparator();
  m_pMenuFile->addAction(m_pActionQuit);
  m_pMenuEdit->addAction(m_pActionSettings);
  m_pMenuWindow->addAction(m_pActionMinimize);
  m_pMenuWindow->addAction(m_pActionRestore);
  m_pMenuHelp->addAction(m_pActionAbout);
  m_pMenuHelp->addAction(m_pActionHelp);

  setMenuBar(m_pMenuBar);
}

void MainWindow::applyConfig() {
  enableServer(m_AppConfig.serverGroupChecked());
  enableClient(m_AppConfig.clientGroupChecked());

  m_pLineEditHostname->setText(m_AppConfig.serverHostname());
  m_pLineEditClientIp->setText(m_ServerConfig.getClientAddress());
}

void MainWindow::applyCloseToTray() const {
  QApplication::setQuitOnLastWindowClosed(!m_AppConfig.closeToTray());
}

void MainWindow::saveSettings() {
  m_AppConfig.setServerGroupChecked(m_pRadioGroupServer->isChecked());
  m_AppConfig.setClientGroupChecked(m_pRadioGroupClient->isChecked());
  m_AppConfig.setServerHostname(m_pLineEditHostname->text());
  m_ServerConfig.setClientAddress(m_pLineEditClientIp->text());

  m_ConfigScopes.save();
}

void MainWindow::setIcon(CoreState state) {
  QIcon icon;
  auto index = static_cast<int>(state);

#ifdef Q_OS_MAC
  switch (getOSXIconsTheme()) {
  case IconsTheme::ICONS_DARK:
    icon.addFile(kDarkIconFiles[index]);
    break;
  case IconsTheme::ICONS_LIGHT:
    icon.addFile(kLightIconFiles[index]);
    break;
  case IconsTheme::ICONS_TEMPLATE:
  default:
    icon.addFile(kDarkIconFiles[index]);
    icon.setIsMask(true);
    break;
  }
#else
  icon.addFile(kDefaultIconFiles[index]);
#endif

  m_TrayIcon.setIcon(icon);
}

void MainWindow::appendLogInfo(const QString &text) {
  qInfo("%s", text.toStdString().c_str());

  processCoreLogLine(getTimeStamp() + " INFO: " + text);
}

void MainWindow::appendLogDebug(const QString &text) {
  qDebug("%s", text.toStdString().c_str());

  if (m_AppConfig.logLevel() >= kDebugLogLevel) {
    processCoreLogLine(getTimeStamp() + " DEBUG: " + text);
  }
}

void MainWindow::appendLogError(const QString &text) {
  qCritical("%s", text.toStdString().c_str());
  processCoreLogLine(getTimeStamp() + " ERROR: " + text);
}

void MainWindow::processCoreLogLine(const QString &text) {
  foreach (QString line, text.split(QRegularExpression("\r|\n|\r\n"))) {
    if (line.isEmpty()) {
      continue;
    }

    // only start if there is no active service running
    if (line.contains("service status: idle") && m_AppConfig.startedBefore()) {
      startCore();
    }

    // HACK: macOS 10.13.4+ spamming error lines in logs making them
    // impossible to read and debug; giving users a red herring.
    if (line.contains("calling TIS/TSM in non-main thread environment")) {
      continue;
    }

    m_pLogOutput->appendPlainText(line);
    updateFromLogLine(line);
  }
}

void MainWindow::updateFromLogLine(const QString &line) {
  // TODO: This shouldn't be updating from log needs a better way of doing this
  checkConnected(line);
  checkFingerprint(line);
  checkSecureSocket(line);

  // subprocess (synergys, synergyc) is not allowed to show notifications
  // process the log from it and show notificatino from synergy instead
#ifdef Q_OS_MAC
  checkOSXNotification(line);
#endif

  if (kLicensingEnabled) {
    checkLicense(line);
  }
}

void MainWindow::checkConnected(const QString &line) {
  // TODO: implement ipc connection state messages to replace this hack.
  if (m_pRadioGroupServer->isChecked()) {
    m_ServerConnection.update(line);
    m_pLabelServerState->updateServerState(line);
  } else {
    m_ClientConnection.update(line);
    m_pLabelClientState->updateClientState(line);
  }

  if (line.contains("connected to server") || line.contains("has connected")) {
    setCoreState(CoreState::Connected);

    if (isVisible()) {
      showFirstRunMessage();
      showDevThanksMessage();
    }

  } else if (line.contains("started server")) {
    setCoreState(CoreState::Listening);
  } else if (
      line.contains("disconnected from server") ||
      line.contains("process exited")) {
    setCoreState(CoreState::Disconnected);
  } else if (line.contains("connecting to")) {
    setCoreState(CoreState::Connecting);
  }
}

void MainWindow::checkLicense(const QString &line) {
  if (line.contains("trial has expired")) {
    showActivationDialog();
  }
}

void MainWindow::checkFingerprint(const QString &line) {
  QRegularExpression re(".*server fingerprint: ([A-F0-9:]+)");
  auto match = re.match(line);
  if (!match.hasMatch()) {
    return;
  }

  auto fingerprint = match.captured(1);
  if (TlsFingerprint::trustedServers().isTrusted(fingerprint)) {
    return;
  }

  static bool messageBoxAlreadyShown = false;

  if (!messageBoxAlreadyShown) {
    onActionStopCoreTriggered();

    messageBoxAlreadyShown = true;
    QMessageBox::StandardButton fingerprintReply = QMessageBox::information(
        this, QString("Security question"),
        QString(
            "You are connecting to a server. Here is it's fingerprint:\n\n"
            "%1\n\n"
            "Compare this fingerprint to the one on your server's screen."
            "If the two don't match exactly, then it's probably not the server "
            "you're expecting (it could be a malicious user).\n\n"
            "To automatically trust this fingerprint for future "
            "connections, click Yes. To reject this fingerprint and "
            "disconnect from the server, click No.")
            .arg(fingerprint),
        QMessageBox::Yes | QMessageBox::No);

    if (fingerprintReply == QMessageBox::Yes) {
      // restart core process after trusting fingerprint.
      TlsFingerprint::trustedServers().trust(fingerprint);
      startCore();
    }

    messageBoxAlreadyShown = false;
  }
}

bool MainWindow::checkSecureSocket(const QString &line) {
  static const QString tlsCheckString = "network encryption protocol: ";
  const auto index = line.indexOf(tlsCheckString, 0, Qt::CaseInsensitive);
  if (index == -1) {
    return false;
  }

  secureSocket(true);
  m_SecureSocketVersion = line.mid(index + tlsCheckString.size());
  return true;
}

#ifdef Q_OS_MAC
void MainWindow::checkOSXNotification(const QString &line) {
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
      appendLogInfo("OSX notification was not shown");
    }
  }
}
#endif

QString MainWindow::getTimeStamp() const {
  QDateTime current = QDateTime::currentDateTime();
  return '[' + current.toString(Qt::ISODate) + ']';
}

void MainWindow::restartCore() {
  stopCore();
  startCore();
}

void MainWindow::showEvent(QShowEvent *event) {
  QMainWindow::showEvent(event);
  emit shown();
}

void MainWindow::closeEvent(QCloseEvent *event) {
  if (!m_AppConfig.closeToTray()) {
    return;
  }

  qDebug("window will hide to tray");
  if (!m_AppConfig.showCloseReminder()) {
    return;
  }

  messages::showCloseReminder(this);
  m_AppConfig.setShowCloseReminder(false);
  m_ConfigScopes.save();
}

void MainWindow::showFirstRunMessage() {
  if (m_AppConfig.startedBefore()) {
    return;
  }

  m_AppConfig.setStartedBefore(true);
  m_ConfigScopes.save();

  const auto isServer = coreMode() == CoreMode::Server;
  messages::showFirstRunMessage(
      this, m_AppConfig.closeToTray(), m_AppConfig.enableService(), isServer);
}

void MainWindow::showDevThanksMessage() {
  if (!m_AppConfig.showDevThanks()) {
    qDebug("skipping dev thanks message, disabled in settings");
    return;
  }

  if (kLicensingEnabled) {
    qDebug("licensing enabled, skipping dev thanks message");
    return;
  }

  m_AppConfig.setShowDevThanks(false);
  m_ConfigScopes.save();

  messages::showDevThanks(this, kProductName);
}

void MainWindow::startCore() {
  appendLogInfo(QString("starting core %1 process").arg(coreModeString()));

  saveSettings();

#ifdef Q_OS_MAC
  requestOSXNotificationPermission();
#endif

  if (kLicensingEnabled) {
    const auto &license = m_LicenseHandler.license();
    if (license.isExpired() && showActivationDialog() == QDialog::Rejected) {
      appendLogDebug("starting core process");
      return;
    }
  }

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

  // put a space between last log output and new instance.
  if (!m_pLogOutput->toPlainText().isEmpty())
    onIpcClientReadLogLine("");

  if ((coreMode() == CoreMode::Client && !clientArgs(args, app)) ||
      (coreMode() == CoreMode::Server && !serverArgs(args, app))) {
    onActionStopCoreTriggered();
    return;
  }

  if (mode == ProcessMode::kDesktop) {
    connect(
        m_pCoreProcess.get(), &QProcess::finished, this,
        &MainWindow::onCoreProcessFinished);
    connect(
        m_pCoreProcess.get(), &QProcess::readyReadStandardOutput, this,
        &MainWindow::onCoreProcessReadyReadStandardOutput);
    connect(
        m_pCoreProcess.get(), &QProcess::readyReadStandardError, this,
        &MainWindow::onCoreProcessReadyReadStandardError);
  }

  if (m_AppConfig.logLevel() >= kDebugLogLevel) {
    appendLogInfo(QString("command: %1 %2").arg(app, args.join(" ")));
  }

  appendLogInfo("log level: " + m_AppConfig.logLevelText());

  if (m_AppConfig.logToFile())
    appendLogInfo("log file: " + m_AppConfig.logFilename());

  if (mode == ProcessMode::kDesktop) {
    m_pCoreProcess->start(app, args);
    if (!m_pCoreProcess->waitForStarted()) {
      show();
      QMessageBox::warning(
          this, QString("Program can not be started"),
          QString(
              QString(
                  "The executable<br><br>%1<br><br>could not be successfully "
                  "started, although it does exist. Please check if you have "
                  "sufficient permissions to run this program.")
                  .arg(app)));
      return;
    }
  } else if (mode == ProcessMode::kService) {
    QString command(app + " " + args.join(" "));
    m_IpcClient.sendCommand(command, m_AppConfig.elevateMode());
  }
}

bool MainWindow::clientArgs(QStringList &args, QString &app) {
  app = appPath(m_AppConfig.coreClientName());

  if (!QFile::exists(app)) {
    show();
    QMessageBox::warning(
        this, QString("Synergy client not found"),
        QString("The executable for the synergy client does not exist."));
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
    if (m_pLineEditHostname->text().isEmpty()) {
      show();
      QMessageBox::warning(
          this, QString("IP/hostname is empty"),
          QString("Please enter a server hostname or IP address."));
      return false;
    }

    QString hostName = m_pLineEditHostname->text();
    // if interface is IPv6 - ensure that ip is in square brackets
    if (hostName.count(':') > 1) {
      if (hostName[0] != '[') {
        hostName.insert(0, '[');
      }
      if (hostName[hostName.size() - 1] != ']') {
        hostName.push_back(']');
      }
    }
    args << hostName + ":" + QString::number(m_AppConfig.port());
  }

  return true;
}

QString MainWindow::configFilename() {
  QString configFullPath;
  if (m_AppConfig.useExternalConfig()) {
    configFullPath = m_AppConfig.configFile();
  } else {
    QStringList errors;
    for (auto path :
         {QStandardPaths::AppDataLocation, QStandardPaths::AppConfigLocation}) {
      auto configDirPath = QStandardPaths::writableLocation(path);
      if (!QDir().mkpath(configDirPath)) {
        errors.push_back(QString("Failed to create config folder \"%1\"")
                             .arg(configDirPath));
        continue;
      }

      QFile configFile(configDirPath + "/LastConfig.cfg");
      if (!configFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        errors.push_back(
            QString("File:\"%1\" Error:%2")
                .arg(configFile.fileName(), configFile.errorString()));
        continue;
      }

      m_ServerConfig.save(configFile);
      configFile.close();
      configFullPath = configFile.fileName();

      break;
    }

    if (configFullPath.isEmpty()) {
      QMessageBox::critical(
          this, QString("Cannot write configuration file"), errors.join('\n'));
    }
  }

  return configFullPath;
}

QString MainWindow::address() const {
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

QString MainWindow::appPath(const QString &name) const {
  QDir dir(QCoreApplication::applicationDirPath());
  return dir.filePath(name);
}

bool MainWindow::serverArgs(QStringList &args, QString &app) {
  app = appPath(m_AppConfig.coreServerName());

  if (!QFile::exists(app)) {
    QMessageBox::warning(
        this, QString("Synergy server not found"),
        QString("The executable for the synergy server does not exist."));
    return false;
  }

  if (m_AppConfig.invertConnection() && m_pLineEditClientIp->text().isEmpty()) {
    QMessageBox::warning(
        this, QString("Client IP address or name is empty"),
        QString("Please fill in a client IP address or name."));
    return false;
  }

  if (m_AppConfig.logToFile()) {
    m_AppConfig.persistLogDir();

    args << "--log" << m_AppConfig.logFilename();
  }

  QString configFilename = this->configFilename();
  if (configFilename.isEmpty()) {
    return false;
  }

  args << "-c" << configFilename << "--address" << address();
  appendLogInfo("config file: " + configFilename);

  if (kLicensingEnabled && !m_AppConfig.serialKey().isEmpty()) {
    args << "--serial-key" << m_AppConfig.serialKey();
  }

  return true;
}

void MainWindow::stopCore() {
  appendLogDebug("stopping core process");

  m_ExpectedRunningState = RuningState::Stopped;

  if (m_AppConfig.processMode() == ProcessMode::kService) {
    stopService();
  } else if (m_AppConfig.processMode() == ProcessMode::kDesktop) {
    stopDesktop();
  }

  setCoreState(CoreState::Disconnected);

  // reset so that new connects cause auto-hide.
  m_AlreadyHidden = false;
}

void MainWindow::stopService() {
  // send empty command to stop service from laucning anything.
  m_IpcClient.sendCommand("", m_AppConfig.elevateMode());
}

void MainWindow::stopDesktop() {
  QMutexLocker locker(&m_StopDesktopMutex);
  if (!m_pCoreProcess) {
    return;
  }

  appendLogInfo("stopping synergy desktop process");

  if (m_pCoreProcess->isOpen()) {
    m_pCoreProcess->close();
  }

  m_pCoreProcess->reset();
}

void MainWindow::setCoreState(CoreState state) {
  // always assume connection is not secure when connection changes
  // to anything except connected. the only way the padlock shows is
  // when the correct TLS version string is detected.
  if (state != CoreState::Connected) {
    secureSocket(false);
  }

  if (m_CoreState == state)
    return;

  if ((state == CoreState::Connected) || (state == CoreState::Connecting) ||
      (state == CoreState::Listening) || (state == CoreState::PendingRetry)) {
    disconnect(
        m_pButtonToggleStart, &QPushButton::clicked, m_pActionStartCore,
        &QAction::trigger);
    connect(
        m_pButtonToggleStart, &QPushButton::clicked, m_pActionStopCore,
        &QAction::trigger);

    m_pButtonToggleStart->setText(QString("&Stop"));
    m_pButtonApply->setEnabled(true);

    m_pActionStartCore->setEnabled(false);
    m_pActionStopCore->setEnabled(true);

  } else if (state == CoreState::Disconnected) {
    disconnect(
        m_pButtonToggleStart, &QPushButton::clicked, m_pActionStopCore,
        &QAction::trigger);
    connect(
        m_pButtonToggleStart, &QPushButton::clicked, m_pActionStartCore,
        &QAction::trigger);

    m_pButtonToggleStart->setText(QString("&Start"));
    m_pButtonApply->setEnabled(false);

    m_pActionStartCore->setEnabled(true);
    m_pActionStopCore->setEnabled(false);
  }

  switch (state) {
    using enum CoreState;

  case Listening: {
    if (coreMode() == CoreMode::Server) {
      setStatus("Synergy is waiting for clients");
    }

    break;
  }
  case Connected: {
    if (m_SecureSocket) {
      setStatus(
          QString("Synergy is connected (with %1)").arg(m_SecureSocketVersion));
    } else {
      setStatus("Synergy is running (without TLS encryption)");
    }
    break;
  }
  case Connecting:
    setStatus("Synergy is starting...");
    break;
  case PendingRetry:
    setStatus("There was an error, retrying...");
    break;
  case Disconnected:
    setStatus("Synergy is not running");
    break;
  }

  setIcon(state);

  m_CoreState = state;
}

void MainWindow::setVisible(bool visible) {
  QMainWindow::setVisible(visible);
  m_pActionMinimize->setEnabled(visible);
  m_pActionRestore->setEnabled(!visible);

#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1070 // lion
  // dock hide only supported on lion :(
  ProcessSerialNumber psn = {0, kCurrentProcess};
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
  GetCurrentProcess(&psn);
#pragma GCC diagnostic pop
  if (visible)
    TransformProcessType(&psn, kProcessTransformToForegroundApplication);
  else
    TransformProcessType(&psn, kProcessTransformToBackgroundApplication);
#endif
}

MainWindow::CoreMode MainWindow::coreMode() const {
  using enum CoreMode;

  auto serverChecked = m_pRadioGroupServer->isChecked();
  auto clientChecked = m_pRadioGroupClient->isChecked();

  if (serverChecked) {
    return Server;
  } else if (clientChecked) {
    return Client;
  } else {
    return None;
  }
}

QString MainWindow::coreModeString() const {
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

QString MainWindow::getIPAddresses() const {
  QStringList result;
  bool hinted = false;
  const auto localnet = QHostAddress::parseSubnet("192.168.0.0/16");
  const QList<QHostAddress> addresses = QNetworkInterface::allAddresses();

  for (const auto &address : addresses) {
    if (address.protocol() == QAbstractSocket::IPv4Protocol &&
        address != QHostAddress(QHostAddress::LocalHost) &&
        !address.isInSubnet(QHostAddress::parseSubnet("169.254.0.0/16"))) {

      // usually 192.168.x.x is a useful ip for the user, so indicate
      // this by making it bold.
      if (!hinted && address.isInSubnet(localnet)) {
        QString format = R"(<span style="color:%1;">%2</span>)";
        result.append(format.arg(kColorTertiary, address.toString()));
        hinted = true;
      } else {
        result.append(address.toString());
      }
    }
  }

  if (result.isEmpty()) {
    result.append("Unknown");
  }

  return result.join(", ");
}

void MainWindow::showLicenseNotice() {
  const auto &license = m_LicenseHandler.license();
  const bool timeLimited = license.isTimeLimited();

  m_pLabelNotice->setVisible(timeLimited);
  if (timeLimited) {
    auto notice = licenseNotice(m_LicenseHandler.license());
    this->m_pLabelNotice->setText(notice);
  }
}

void MainWindow::updateLocalFingerprint() {
  bool fingerprintExists = false;
  try {
    fingerprintExists = TlsFingerprint::local().fileExists();
  } catch (const std::exception &e) {
    qDebug() << e.what();
    qFatal("failed to check if fingerprint exists");
  }

  if (m_AppConfig.tlsEnabled() && fingerprintExists &&
      m_pRadioGroupServer->isChecked()) {
    m_pLabelFingerprint->setVisible(true);
  } else {
    m_pLabelFingerprint->setVisible(false);
  }
}

QString MainWindow::productName() const {
  if (kLicensingEnabled) {
    return m_LicenseHandler.productName();
  } else if (!kProductName.isEmpty()) {
    return kProductName;
  } else {
    qFatal("product name not set");
  }
}

void MainWindow::updateWindowTitle() { setWindowTitle(productName()); }

void MainWindow::autoAddScreen(const QString name) {

  if (m_ActivationDialogRunning) {
    // add this screen to the pending list if the activation dialog is running.
    m_PendingClientNames.append(name);
    return;
  }

  int r = m_ServerConfig.autoAddScreen(name);
  if (r != kAutoAddScreenOk) {
    switch (r) {
    case kAutoAddScreenManualServer:
      showConfigureServer(QString("Please add the server (%1) to the grid.")
                              .arg(m_AppConfig.screenName()));
      break;

    case kAutoAddScreenManualClient:
      showConfigureServer(QString("Please drag the new client screen (%1) "
                                  "to the desired position on the grid.")
                              .arg(name));
      break;
    }
  }
}

bool MainWindow::isCoreActive() const {
  using enum CoreState;

  auto state = m_CoreState;
  return (state == Connected) || (state == Connecting) || (state == Listening);
}

void MainWindow::showConfigureServer(const QString &message) {
  ServerConfigDialog dialog(this, serverConfig(), m_AppConfig);
  dialog.message(message);
  if ((dialog.exec() == QDialog::Accepted) && isCoreActive()) {
    restartCore();
  }
}

int MainWindow::showActivationDialog() {
  if (m_ActivationDialogRunning) {
    return QDialog::Rejected;
  }

  ActivationDialog activationDialog(this, m_AppConfig, m_LicenseHandler);

  m_ActivationDialogRunning = true;
  int result = activationDialog.exec();
  m_ActivationDialogRunning = false;

  if (result == QDialog::Accepted) {
    m_AppConfig.setActivationHasRun(true);
    m_ConfigScopes.save();
  }

  if (!m_PendingClientNames.empty()) {
    foreach (const QString &name, m_PendingClientNames) {
      autoAddScreen(name);
    }

    m_PendingClientNames.clear();
  }
  return result;
}

QString MainWindow::getProfileRootForArg() const {
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

void MainWindow::secureSocket(bool secureSocket) {
  m_SecureSocket = secureSocket;
  if (secureSocket) {
    m_pLabelPadlock->show();
  } else {
    m_pLabelPadlock->hide();
  }
}

void MainWindow::updateScreenName() {
  m_pLabelComputerName->setText(
      QString("This computer's name: %1 "
              R"((<a href="#" style="color: %2">change</a>))")
          .arg(m_AppConfig.screenName())
          .arg(kColorSecondary));
  m_ServerConfig.updateServerName();
}

void MainWindow::enableServer(bool enable) {
  qDebug(enable ? "server enabled" : "server disabled");
  m_AppConfig.setServerGroupChecked(enable);
  m_pRadioGroupServer->setChecked(enable);
  m_pWidgetServer->setEnabled(enable);
  m_pWidgetServerInverse->setVisible(m_AppConfig.invertConnection());

  if (enable) {
    m_pButtonToggleStart->setEnabled(true);
    m_pActionStartCore->setEnabled(true);
  }
}

void MainWindow::enableClient(bool enable) {
  qDebug(enable ? "client enabled" : "client disabled");
  m_AppConfig.setClientGroupChecked(enable);
  m_pRadioGroupClient->setChecked(enable);
  m_pWidgetClient->setEnabled(enable);
  m_pWidgetClient->setVisible(!m_AppConfig.invertConnection());

  if (enable) {
    m_pButtonToggleStart->setEnabled(true);
    m_pActionStartCore->setEnabled(true);
  }
}
