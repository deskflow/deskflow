/*
 * Deskflow -- mouse and keyboard sharing utility
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
#include "common/constants.h"
#include "gui/Logger.h"
#include "gui/TrayIcon.h"
#include "gui/VersionChecker.h"
#include "gui/config/ConfigScopes.h"
#include "gui/constants.h"
#include "gui/core/CoreProcess.h"
#include "gui/diagnostic.h"
#include "gui/dialogs/SettingsDialog.h"
#include "gui/license/LicenseHandler.h"
#include "gui/license/license_notices.h"
#include "gui/license/license_utils.h"
#include "gui/messages.h"
#include "gui/string_utils.h"
#include "gui/styles.h"
#include "gui/tls/TlsFingerprint.h"
#include "license/License.h"
#include "platform/wayland.h"

#if defined(Q_OS_MAC)
#include "gui/OSXHelpers.h"
#endif
#if defined(Q_OS_LINUX)
#include "config.h"
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
#include <QScrollBar>
#include <QtCore>
#include <QtGui>
#include <QtNetwork>

#if defined(Q_OS_MAC)
#include <ApplicationServices/ApplicationServices.h>
#endif

using namespace deskflow::gui;
using namespace deskflow::license;
using namespace deskflow::gui::license;

using CoreMode = CoreProcess::Mode;
using CoreConnectionState = CoreProcess::ConnectionState;
using CoreProcessState = CoreProcess::ProcessState;

const auto kIconFile16 = ":/icons/16x16/tray.png";

#ifdef Q_OS_MAC
const auto kLightIconFile = ":/icons/64x64/tray-light.png";
const auto kDarkIconFile = ":/icons/64x64/tray-dark.png";
#endif // Q_OS_MAC

MainWindow::MainWindow(ConfigScopes &configScopes, AppConfig &appConfig)
    : m_ConfigScopes(configScopes),
      m_AppConfig(appConfig),
      m_ServerConfig(appConfig, *this),
      m_CoreProcess(appConfig, m_ServerConfig, m_LicenseHandler.license()),
      m_ServerConnection(
          this, appConfig, m_ServerConfig, m_ServerConfigDialogState),
      m_ClientConnection(this, appConfig),
      m_TlsUtility(appConfig, m_LicenseHandler.license()),
      m_WindowSaveTimer(this) {

  setupUi(this);
  createMenuBar();
  setupControls();
  connectSlots();

  // handled by `onCreated`
  emit created();
}

MainWindow::~MainWindow() { m_CoreProcess.cleanup(); }

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
  } else {
    // center main window in middle of screen
    const auto screen = QGuiApplication::primaryScreen();
    QRect screenGeometry = screen->geometry();
    move(screenGeometry.center() - rect().center());
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
  if (!isActivationEnabled()) {
    updateWindowTitle();
  }

  if (!isLicensedProduct()) {
    m_pActionHelp->setText("Report a bug");
    m_pActionActivate->setText("Purchase");
  } else if (!isActivationEnabled()) {
    m_pActionActivate->setVisible(false);
  }

  secureSocket(false);
  updateLocalFingerprint();

  m_pLabelUpdate->setStyleSheet(kStyleNoticeLabel);
  m_pLabelUpdate->hide();

  m_pLabelNotice->setStyleSheet(kStyleNoticeLabel);
  m_pLabelNotice->hide();

  m_pLabelIpAddresses->setText(
      QString("This computer's IP addresses: %1").arg(getIPAddresses()));

  if (m_AppConfig.lastVersion() != DESKFLOW_VERSION) {
    m_AppConfig.setLastVersion(DESKFLOW_VERSION);
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
void MainWindow::connectSlots() {

  connect(
      &Logger::instance(), &Logger::newLine, this, //
      [this](const QString &line) { handleLogLine(line); });

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
      &m_CoreProcess, &CoreProcess::starting, this,
      &MainWindow::onCoreProcessStarting, Qt::DirectConnection);

  connect(
      &m_CoreProcess, &CoreProcess::error, this,
      &MainWindow::onCoreProcessError);

  connect(
      &m_CoreProcess, &CoreProcess::logLine, //
      [this](const QString &line) { handleLogLine(line); });

  connect(
      &m_CoreProcess, &CoreProcess::processStateChanged, this,
      &MainWindow::onCoreProcessStateChanged);

  connect(
      &m_CoreProcess, &CoreProcess::connectionStateChanged, this,
      &MainWindow::onCoreConnectionStateChanged);

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

  connect(
      m_pActionRestore, &QAction::triggered, //
      [this]() { showAndActivate(); });

  connect(m_pActionQuit, &QAction::triggered, qApp, [this] {
    qDebug("quitting application");
    m_Quitting = true;
    QApplication::quit();
  });

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

  connect(
      &m_ServerConnection, &ServerConnection::messageShowing, this,
      [this]() { showAndActivate(); });

  connect(
      &m_ClientConnection, &ClientConnection::messageShowing, this,
      [this]() { showAndActivate(); });
}

void MainWindow::onAppAboutToQuit() {
  if (m_SaveOnExit) {
    m_ConfigScopes.save();
  }
}

void MainWindow::onCreated() {

  setIcon();

  m_ConfigScopes.signalReady();

  applyCloseToTray();

  if (isActivationEnabled() && !m_AppConfig.serialKey().isEmpty()) {
    m_LicenseHandler.changeSerialKey(m_AppConfig.serialKey());
  }

  updateScreenName();
  applyConfig();
  restoreWindow();

  qDebug().noquote() << "active settings path:"
                     << m_ConfigScopes.activeFilePath();
}

void MainWindow::onShown() {
  if (isActivationEnabled()) {
    const auto &license = m_LicenseHandler.license();
    if (!m_AppConfig.activationHasRun() || !license.isValid() ||
        license.isExpired()) {
      showActivationDialog();
    }
  }

  // if a critical error was shown just before the main window (i.e. on app
  // load), it will be hidden behind the main window. therefore we need to raise
  // it up in front of the main window.
  // HACK: because the `onShown` event happens just as the window is shown, the
  // message box has a chance of being raised under the main window. to solve
  // this we delay the error dialog raise by a split second. this seems a bit
  // hacky and fragile, so maybe there's a better approach.
  const auto kCriticalDialogDelay = 100;
  QTimer::singleShot(
      kCriticalDialogDelay, [] { messages::raiseCriticalDialog(); });
}

void MainWindow::onLicenseHandlerSerialKeyChanged(const QString &serialKey) {
  updateWindowTitle();
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
    m_TlsUtility.generateCertificate();
  }
}

void MainWindow::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason) {

  if (reason == QSystemTrayIcon::DoubleClick) {
    if (isVisible()) {
      hide();
    } else {
      showAndActivate();
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

void MainWindow::on_m_pActionStartCore_triggered() {
  m_ClientConnection.setShowMessage();
  m_CoreProcess.start();
}

void MainWindow::on_m_pActionStopCore_triggered() {
  qDebug("stopping core process");
  m_CoreProcess.stop();
}

void MainWindow::on_m_pActionTestFatalError_triggered() const {
  qFatal("test fatal error");
}

void MainWindow::on_m_pActionTestCriticalError_triggered() const {
  qCritical("test critical error");
}

void MainWindow::on_m_pActionClearSettings_triggered() {
  if (!messages::showClearSettings(this)) {
    qDebug("clear settings cancelled");
    return;
  }

  m_CoreProcess.stop();

  m_Quitting = true;
  m_SaveOnExit = false;
  diagnostic::clearSettings(m_ConfigScopes, true);
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

void MainWindow::on_m_pActionHelp_triggered() const {
  if (isLicensedProduct()) {
    QDesktopServices::openUrl(QUrl(kUrlHelp));
  } else {
    QDesktopServices::openUrl(QUrl(kUrlBugReport));
  }
}

void MainWindow::on_m_pActionSettings_triggered() {
  auto dialog = SettingsDialog(
      this, m_AppConfig, m_ServerConfig, m_LicenseHandler.license(),
      m_CoreProcess);

  if (dialog.exec() == QDialog::Accepted) {
    m_ConfigScopes.save();

    applyConfig();
    applyCloseToTray();

    if (m_CoreProcess.isStarted()) {
      m_CoreProcess.restart();
    }
  }
}

void MainWindow::on_m_pButtonConfigureServer_clicked() {
  showConfigureServer();
}

void MainWindow::on_m_pActionActivate_triggered() {
  if (isLicensedProduct()) {
    showActivationDialog();
  } else {
    QDesktopServices::openUrl(QUrl(kUrlPurchase));
  }
}

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
  m_ClientConnection.setShowMessage();
  m_CoreProcess.restart();
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
  m_ServerConfigDialogState.setVisible(true);
  ServerConfigDialog dialog(this, m_ServerConfig, m_AppConfig);
  if (dialog.addClient(clientName) && dialog.exec() == QDialog::Accepted) {
    m_CoreProcess.restart();
  }
  m_ServerConfigDialogState.setVisible(false);
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
    showAndActivate();
  }

  if (!m_AppConfig.enableUpdateCheck().has_value()) {
    m_AppConfig.setEnableUpdateCheck(messages::showUpdateCheckOption(this));
  }

  if (m_AppConfig.enableUpdateCheck().value()) {
    m_VersionChecker.checkLatest();
  } else {
    qDebug("update check disabled");
  }

  if (m_AppConfig.startedBefore()) {
    m_CoreProcess.start();
  }
}

void MainWindow::onCoreProcessStarting() {

  if (isActivationEnabled()) {
    const auto &license = m_LicenseHandler.license();
    if (license.isExpired() && showActivationDialog() == QDialog::Rejected) {
      qDebug("license expired, cancelling core start");
      m_CoreProcess.stop();
      return;
    }
  }

#if defined(WINAPI_XWINDOWS) or defined(WINAPI_LIBEI)
  if (deskflow::platform::isWayland()) {
    m_WaylandWarnings.showOnce(this, m_CoreProcess.mode());
  }
#endif

  saveSettings();
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
  m_pMenuFile->addAction(m_pActionActivate);
  m_pMenuFile->addSeparator();
  m_pMenuFile->addAction(m_pActionSave);
  m_pMenuFile->addSeparator();
  m_pMenuFile->addAction(m_pActionQuit);

  m_pMenuEdit->addAction(m_pActionSettings);

  m_pMenuWindow->addAction(m_pActionMinimize);
  m_pMenuWindow->addAction(m_pActionRestore);

  m_pMenuHelp->addAction(m_pActionAbout);
  m_pMenuHelp->addAction(m_pActionHelp);
  m_pMenuFile->addSeparator();
  m_pMenuHelp->addAction(m_pActionClearSettings);

  const auto enableTestMenu =
      strToTrue(qEnvironmentVariable("DESKFLOW_TEST_MENU"));

  if (enableTestMenu || kDebugBuild) {
    auto testMenu = new QMenu("Test", m_pMenuBar);
    m_pMenuBar->addMenu(testMenu);
    testMenu->addAction(m_pActionTestFatalError);
    testMenu->addAction(m_pActionTestCriticalError);
  }

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

void MainWindow::setIcon() {
  QIcon icon;
#ifdef Q_OS_MAC
  switch (getOSXIconsTheme()) {
  case IconsTheme::ICONS_DARK:
    icon.addFile(kDarkIconFile);
    break;
  case IconsTheme::ICONS_LIGHT:
    icon.addFile(kLightIconFile);
    break;
  case IconsTheme::ICONS_TEMPLATE:
  default:
    icon.addFile(kDarkIconFile);
    icon.setIsMask(true);
    break;
  }
#else
  icon.addFile(kIconFile16);
#endif

  m_TrayIcon.setIcon(icon);
}

void MainWindow::handleLogLine(const QString &line) {
  const int kScrollBottomThreshold = 2;

  QScrollBar *verticalScroll = m_pLogOutput->verticalScrollBar();
  int currentScroll = verticalScroll->value();
  int maxScroll = verticalScroll->maximum();
  const auto scrollAtBottom =
      qAbs(currentScroll - maxScroll) <= kScrollBottomThreshold;

  // only trim end instead of the whole line to prevent tab-indented debug
  // filenames from losing their indentation.
  m_pLogOutput->appendPlainText(trimEnd(line));

  if (scrollAtBottom) {
    verticalScroll->setValue(verticalScroll->maximum());
    m_pLogOutput->horizontalScrollBar()->setValue(0);
  }

  updateFromLogLine(line);
}

void MainWindow::updateFromLogLine(const QString &line) {
  checkConnected(line);
  checkFingerprint(line);

  if (isActivationEnabled()) {
    checkLicense(line);
  }
}

void MainWindow::checkConnected(const QString &line) {
  if (m_pRadioGroupServer->isChecked()) {
    m_ServerConnection.handleLogLine(line);
    m_pLabelServerState->updateServerState(line);
  } else {
    m_ClientConnection.handleLogLine(line);
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
            "<p>You are connecting to a server.</p>"
            "<p>Here is it's TLS fingerprint:</p>"
            "<p>%1</p>"
            "<p>Compare this fingerprint to the one on your server's screen. "
            "If the two don't match exactly, then it's probably not the server "
            "you're expecting (it could be a malicious user).</p>"
            "<p>Do you want to trust this fingerprint for future "
            "connections? If you don't, a connection cannot be made.</p>")
            .arg(fingerprint),
        QMessageBox::Yes | QMessageBox::No);

    if (fingerprintReply == QMessageBox::Yes) {
      // start core process again after trusting fingerprint.
      TlsFingerprint::trustedServers().trust(fingerprint);
      m_CoreProcess.start();
    }

    messageBoxAlreadyShown = false;
  }
}

QString MainWindow::getTimeStamp() const {
  QDateTime current = QDateTime::currentDateTime();
  return '[' + current.toString(Qt::ISODate) + ']';
}

void MainWindow::showEvent(QShowEvent *event) {
  QMainWindow::showEvent(event);
  emit shown();
}

void MainWindow::closeEvent(QCloseEvent *event) {
  if (m_Quitting) {
    qDebug("skipping close event handle on quit");
    return;
  }

  if (!m_AppConfig.closeToTray()) {
    qDebug("window will not hide to tray");
    return;
  }

  if (m_AppConfig.showCloseReminder()) {
    messages::showCloseReminder(this);
    m_AppConfig.setShowCloseReminder(false);
  }

  m_ConfigScopes.save();
  qDebug("window should hide to tray");
}

void MainWindow::showFirstConnectedMessage() {
  if (m_AppConfig.startedBefore()) {
    return;
  }

  m_AppConfig.setStartedBefore(true);
  m_ConfigScopes.save();

  const auto isServer = m_CoreProcess.mode() == CoreMode::Server;
  messages::showFirstConnectedMessage(
      this, m_AppConfig.closeToTray(), m_AppConfig.enableService(), isServer);
}

void MainWindow::showDevThanksMessage() {
  if (!m_AppConfig.showDevThanks()) {
    qDebug("skipping dev thanks message");
    return;
  }

  if (isActivationEnabled()) {
    qDebug("activation enabled, skipping dev thanks message");
    return;
  }

  m_AppConfig.setShowDevThanks(false);
  m_ConfigScopes.save();

  messages::showDevThanks(this, kProductName);
}

void MainWindow::onCoreProcessSecureSocket(bool enabled) {
  secureSocket(enabled);
}

void MainWindow::updateStatus() {
  const auto connection = m_CoreProcess.connectionState();
  const auto process = m_CoreProcess.processState();

  switch (process) {
    using enum CoreProcessState;

  case Starting:
    setStatus("Deskflow is starting...");
    break;

  case RetryPending:
    setStatus("Deskflow will retry in a moment...");
    break;

  case Stopping:
    setStatus("Deskflow is stopping...");
    break;

  case Stopped:
    setStatus("Deskflow is not running");
    break;

  case Started: {
    switch (connection) {
      using enum CoreConnectionState;

    case Listening: {
      if (m_CoreProcess.mode() == CoreMode::Server) {
        setStatus("Deskflow is waiting for clients");
      }

      break;
    }

    case Connecting:
      setStatus("Deskflow is connecting...");
      break;

    case Connected: {
      if (m_SecureSocket) {
        setStatus(QString("Deskflow is connected (with %1)")
                      .arg(m_CoreProcess.secureSocketVersion()));
      } else {
        setStatus("Deskflow is connected (without TLS encryption)");
      }
      break;
    }

    case Disconnected:
      setStatus("Deskflow is disconnected");
      break;
    }
  } break;
  }
}

void MainWindow::onCoreProcessStateChanged(CoreProcessState state) {
  updateStatus();

  if (state == CoreProcessState::Started) {
    qDebug("recording that core has started");
    m_AppConfig.setStartedBefore(true);
    m_ConfigScopes.save();
  }

  if (state == CoreProcessState::Started ||
      state == CoreProcessState::Starting ||
      state == CoreProcessState::RetryPending) {
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

  } else {
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
}

void MainWindow::onCoreConnectionStateChanged(CoreConnectionState state) {
  qDebug("core connection state changed: %d", static_cast<int>(state));

  updateStatus();

  // always assume connection is not secure when connection changes
  // to anything except connected. the only way the padlock shows is
  // when the correct TLS version string is detected.
  if (state != CoreConnectionState::Connected) {
    secureSocket(false);
  } else if (isVisible()) {
    showFirstConnectedMessage();
    showDevThanksMessage();
  }
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
  if (isActivationEnabled()) {
    return m_LicenseHandler.productName();
  } else if (!kProductName.isEmpty()) {
    return kProductName;
  } else {
    qFatal("product name not set");
    return "";
  }
}

void MainWindow::updateWindowTitle() { setWindowTitle(productName()); }

void MainWindow::autoAddScreen(const QString name) {

  if (m_ActivationDialogRunning) {
    // add this screen to the pending list if the activation dialog is
    // running.
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
  if ((dialog.exec() == QDialog::Accepted) && m_CoreProcess.isStarted()) {
    m_CoreProcess.restart();
  }
}

int MainWindow::showActivationDialog() {
  if (!isActivationEnabled()) {
    qFatal("cannot show activation dialog when activation is disabled");
  }

  if (m_ActivationDialogRunning) {
    return QDialog::Rejected;
  }

  ActivationDialog activationDialog(this, m_AppConfig, m_LicenseHandler);

  m_ActivationDialogRunning = true;
  int result = activationDialog.exec();
  m_ActivationDialogRunning = false;

  if (result == QDialog::Accepted) {
    m_AppConfig.setActivationHasRun(true);

    // customers who are activating a pro license are usually doing so because
    // they want tls. so, if it's available, turn it on after activating.
    m_AppConfig.setTlsEnabled(m_LicenseHandler.license().isTlsAvailable());

    m_ConfigScopes.save();
  }

  if (!m_PendingClientNames.empty()) {
    foreach (const QString &name, m_PendingClientNames) {
      autoAddScreen(name);
    }

    m_PendingClientNames.clear();
  }

  // restart core process after activation in case switching on tls.
  // this saves customer from having to figure out they need to click apply.
  if (m_CoreProcess.isStarted()) {
    m_CoreProcess.restart();
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
  m_pWidgetServerInput->setVisible(m_AppConfig.invertConnection());

  if (enable) {
    m_pButtonToggleStart->setEnabled(true);
    m_pActionStartCore->setEnabled(true);
    m_CoreProcess.setMode(CoreProcess::Mode::Server);

    // The server can run without any clients configured, and this is actually
    // what you'll want to do the first time since you'll be prompted when an
    // unrecognized client tries to connect.
    if (!m_AppConfig.startedBefore()) {
      qDebug("auto-starting core server for first time");
      m_CoreProcess.start();
      messages::showFirstServerStartMessage(this);
    }
  }
}

void MainWindow::enableClient(bool enable) {
  qDebug(enable ? "client enabled" : "client disabled");
  m_AppConfig.setClientGroupChecked(enable);
  m_pRadioGroupClient->setChecked(enable);
  m_pWidgetClientInput->setEnabled(enable);
  m_pWidgetClientInput->setVisible(!m_AppConfig.invertConnection());

  if (enable) {
    m_pButtonToggleStart->setEnabled(true);
    m_pActionStartCore->setEnabled(true);
    m_CoreProcess.setMode(CoreProcess::Mode::Client);
  }
}

void MainWindow::showAndActivate() {
  if (!isMinimized() && !isHidden()) {
    qDebug("window already visible");
    return;
  }

  showNormal();
  raise();
  activateWindow();
}
