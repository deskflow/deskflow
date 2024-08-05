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
#include "gui/ConfigScopes.h"
#include "gui/CoreProcess.h"
#include "gui/LicenseHandler.h"
#include "gui/TlsFingerprint.h"
#include "gui/TrayIcon.h"
#include "gui/VersionChecker.h"
#include "gui/constants.h"
#include "gui/license_notices.h"
#include "gui/messages.h"
#include "gui/styles.h"
#include "license/License.h"

#if defined(Q_OS_MAC)
#include "gui/OSXHelpers.h"
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

#if defined(Q_OS_MAC)
#include <ApplicationServices/ApplicationServices.h>
#endif

using namespace synergy::gui;
using namespace synergy::license;

using CoreMode = CoreProcess::Mode;
using CoreState = CoreProcess::ConnectionState;

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
      m_CoreProcess(appConfig, m_ServerConfig),
      m_ServerConnection(*this, appConfig, m_ServerConfig),
      m_ClientConnection(*this, appConfig),
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
      &m_CoreProcess, &CoreProcess::error, this,
      &MainWindow::onCoreProcessError);

  connect(
      &m_CoreProcess, &CoreProcess::logLine, this,
      &MainWindow::onCoreProcessLogLine);

  connect(
      &m_CoreProcess, &CoreProcess::logInfo, this,
      &MainWindow::onCoreProcessLogInfo);

  connect(
      &m_CoreProcess, &CoreProcess::logError, this,
      &MainWindow::onCoreProcessLogError);

  connect(
      &m_CoreProcess, &CoreProcess::stateChanged, this,
      &MainWindow::onCoreProcessStateChanged);

  connect(
      &m_CoreProcess, &CoreProcess::secureSocket, this,
      &MainWindow::onCoreProcessSecureSocket);

  connect(
      &m_LicenseHandler, &LicenseHandler::serialKeyChanged, this,
      &MainWindow::onLicenseHandlerSerialKeyChanged);

  connect(
      &m_LicenseHandler, &LicenseHandler::invalidLicense, this,
      &MainWindow::onLicenseHandlerInvalidLicense);

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

  connect(
      &m_ServerConnection, &ServerConnection::configureClient, this,
      &MainWindow::onServerConnectionConfigureClient);
}

void MainWindow::onAppAboutToQuit() { m_ConfigScopes.save(); }

void MainWindow::onCreated() {

  setIcon(CoreState::Disconnected);

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
  m_CoreProcess.stop();
  showActivationDialog();
}

void MainWindow::onConfigScopesSaving() { m_ServerConfig.commit(); }

void MainWindow::onAppConfigTlsChanged() {
  updateLocalFingerprint();
  if (m_TlsUtility.isAvailableAndEnabled()) {
    m_TlsUtility.generateCertificate(true);
  }
}

void MainWindow::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason) {

  if (reason == QSystemTrayIcon::DoubleClick) {
    if (isVisible()) {
      hide();
    } else {
      showNormal();
      activateWindow();
    }
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

void MainWindow::onActionStopCoreTriggered() { m_CoreProcess.stop(); }

void MainWindow::onAppConfigScreenNameChanged() { updateScreenName(); }

void MainWindow::onAppConfigInvertConnection() { applyConfig(); }

void MainWindow::onCoreProcessError(CoreProcess::Error error) {
  if (error == CoreProcess::Error::AddressMissing) {
    QMessageBox::warning(
        this, QString("Address missing"),
        QString(
            "Please enter the hostname or IP address of the other computer."));
  } else if (error == CoreProcess::Error::StartFailed) {
    show();
    QMessageBox::warning(
        this, QString("Core cannot be started"),
        "The Core executable could not be successfully started, "
        "although it does exist. "
        "Please check if you have sufficient permissions to run this program.");
  }
}

void MainWindow::onCoreProcessLogLine(const QString &line) {
  handleLogLine(line);
}

void MainWindow::onCoreProcessLogInfo(const QString &message) {
  appendLogInfo(message);
}

void MainWindow::onCoreProcessLogError(const QString &message) {
  appendLogError(message);
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

    if (m_CoreProcess.isActive()) {
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

void MainWindow::on_m_pLineEditHostname_textChanged(const QString &text) {
  m_CoreProcess.setAddress(text);
}

void MainWindow::on_m_pLineEditClientIp_textChanged(const QString &text) {
  m_CoreProcess.setAddress(text);
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

void MainWindow::onServerConnectionConfigureClient(const QString &clientName) {
  ServerConfigDialog dialog(this, m_ServerConfig, m_AppConfig);
  if (dialog.addClient(clientName) && dialog.exec() == QDialog::Accepted) {
    restartCore();
  }
}

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

void MainWindow::startCore() {

  if (kLicensingEnabled) {
    const auto &license = m_LicenseHandler.license();
    if (license.isExpired() && showActivationDialog() == QDialog::Rejected) {
      qDebug("license expired, not starting core");
      return;
    }
  }

  saveSettings();
  m_CoreProcess.start();
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

  handleLogLine(getTimeStamp() + " INFO: " + text);
}

void MainWindow::appendLogDebug(const QString &text) {
  qDebug("%s", text.toStdString().c_str());

  if (m_AppConfig.logLevel() >= kDebugLogLevel) {
    handleLogLine(getTimeStamp() + " DEBUG: " + text);
  }
}

void MainWindow::appendLogError(const QString &text) {
  qCritical("%s", text.toStdString().c_str());
  handleLogLine(getTimeStamp() + " ERROR: " + text);
}

void MainWindow::handleLogLine(const QString &line) {
  m_pLogOutput->appendPlainText(line);
  updateFromLogLine(line);
}

void MainWindow::updateFromLogLine(const QString &line) {
  checkConnected(line);
  checkFingerprint(line);

  if (kLicensingEnabled) {
    checkLicense(line);
  }
}

void MainWindow::checkConnected(const QString &line) {
  if (m_pRadioGroupServer->isChecked()) {
    m_ServerConnection.update(line);
    m_pLabelServerState->updateServerState(line);
  } else {
    m_ClientConnection.update(line);
    m_pLabelClientState->updateClientState(line);
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
    m_CoreProcess.stop();

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

QString MainWindow::getTimeStamp() const {
  QDateTime current = QDateTime::currentDateTime();
  return '[' + current.toString(Qt::ISODate) + ']';
}

void MainWindow::restartCore() {
  m_CoreProcess.stop();
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

  const auto isServer = m_CoreProcess.mode() == CoreMode::Server;
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

void MainWindow::onCoreProcessSecureSocket(bool enabled) {
  secureSocket(enabled);
}

void MainWindow::onCoreProcessStateChanged(CoreState state) {
  // always assume connection is not secure when connection changes
  // to anything except connected. the only way the padlock shows is
  // when the correct TLS version string is detected.
  if (state != CoreState::Connected) {
    secureSocket(false);
  } else if (isVisible()) {
    showFirstRunMessage();
    showDevThanksMessage();
  }

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
    if (m_CoreProcess.mode() == CoreMode::Server) {
      setStatus("Synergy is waiting for clients");
    }

    break;
  }
  case Connected: {
    if (m_SecureSocket) {
      setStatus(
          QString("Synergy is connected (with %1)").arg(m_CoreProcess.secureSocketVersion()));
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

void MainWindow::showConfigureServer(const QString &message) {
  ServerConfigDialog dialog(this, serverConfig(), m_AppConfig);
  dialog.message(message);
  if ((dialog.exec() == QDialog::Accepted) && m_CoreProcess.isActive()) {
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
    m_CoreProcess.setMode(CoreProcess::Mode::Server);
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
    m_CoreProcess.setMode(CoreProcess::Mode::Client);
  }
}
