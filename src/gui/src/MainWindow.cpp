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
#include "ui_MainWindow.h"

#include "ServerConfigDialog.h"
#include "common/constants.h"
#include "dialogs/AboutDialog.h"
#include "gui/Logger.h"
#include "gui/config/ConfigScopes.h"
#include "gui/constants.h"
#include "gui/core/CoreProcess.h"
#include "gui/diagnostic.h"
#include "gui/dialogs/SettingsDialog.h"
#include "gui/messages.h"
#include "gui/string_utils.h"
#include "gui/styles.h"
#include "gui/tls/TlsFingerprint.h"
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
#include <QNetworkInterface>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollBar>

#include <memory>

#if defined(Q_OS_MAC)
#include <ApplicationServices/ApplicationServices.h>
#endif

using namespace deskflow::gui;

using CoreMode = CoreProcess::Mode;
using CoreConnectionState = CoreProcess::ConnectionState;
using CoreProcessState = CoreProcess::ProcessState;

const auto kIconFile = ":/icons/128x128/tray.png";

#ifdef Q_OS_MAC
const auto kLightIconFile = ":/icons/128x128/tray-light.png";
const auto kDarkIconFile = ":/icons/128x128/tray-dark.png";
#endif // Q_OS_MAC

MainWindow::MainWindow(ConfigScopes &configScopes, AppConfig &appConfig)
    : ui{std::make_unique<Ui::MainWindow>()},
      m_ConfigScopes(configScopes),
      m_AppConfig(appConfig),
      m_ServerConfig(appConfig, *this),
      m_CoreProcess(appConfig, m_ServerConfig),
      m_ServerConnection(this, appConfig, m_ServerConfig, m_ServerConfigDialogState),
      m_ClientConnection(this, appConfig),
      m_TlsUtility(appConfig),
      m_WindowSaveTimer(this)
{

  ui->setupUi(this);
  createMenuBar();
  setupControls();
  connectSlots();

  // handled by `onCreated`
  Q_EMIT created();
}

MainWindow::~MainWindow()
{
  m_CoreProcess.cleanup();
}

void MainWindow::restoreWindow()
{

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

void MainWindow::saveWindow()
{
  if (!m_SaveWindow) {
    qDebug("not yet ready to save window size and position, skipping");
    return;
  }

  qDebug("saving window size and position");
  m_AppConfig.setMainWindowSize(size());
  m_AppConfig.setMainWindowPosition(pos());
  m_ConfigScopes.save();
}

void MainWindow::setupControls()
{
  setWindowTitle(kAppName);

  ui->m_pActionHelp->setText(DESKFLOW_HELP_TEXT);

  secureSocket(false);
  updateLocalFingerprint();

  ui->m_pLabelUpdate->setStyleSheet(kStyleNoticeLabel);
  ui->m_pLabelUpdate->hide();

  ui->m_pLabelNotice->setStyleSheet(kStyleNoticeLabel);
  ui->m_pLabelNotice->hide();

  ui->m_pLabelIpAddresses->setText(QString("This computer's IP addresses: %1").arg(getIPAddresses()));

  if (m_AppConfig.lastVersion() != DESKFLOW_VERSION) {
    m_AppConfig.setLastVersion(DESKFLOW_VERSION);
  }

#if defined(Q_OS_MAC)

  ui->m_pRadioGroupServer->setAttribute(Qt::WA_MacShowFocusRect, 0);
  ui->m_pRadioGroupClient->setAttribute(Qt::WA_MacShowFocusRect, 0);

#endif
}

//////////////////////////////////////////////////////////////////////////////
// Begin slots
//////////////////////////////////////////////////////////////////////////////

// remember: using queued connection allows the render loop to run before
// executing the slot. the default is to instantly call the slot when the
// signal is emitted from the thread that owns the receiver's object.
void MainWindow::connectSlots()
{

  connect(
      &Logger::instance(), &Logger::newLine, this, //
      [this](const QString &line) { handleLogLine(line); }
  );

  connect(this, &MainWindow::created, this, &MainWindow::onCreated);

  connect(this, &MainWindow::shown, this, &MainWindow::onShown, Qt::QueuedConnection);

  connect(&m_ConfigScopes, &ConfigScopes::saving, this, &MainWindow::onConfigScopesSaving, Qt::DirectConnection);

  connect(&m_AppConfig, &AppConfig::tlsChanged, this, &MainWindow::onAppConfigTlsChanged);

  connect(&m_AppConfig, &AppConfig::screenNameChanged, this, &MainWindow::onAppConfigScreenNameChanged);

  connect(&m_AppConfig, &AppConfig::invertConnectionChanged, this, &MainWindow::onAppConfigInvertConnection);

  connect(&m_CoreProcess, &CoreProcess::starting, this, &MainWindow::onCoreProcessStarting, Qt::DirectConnection);

  connect(&m_CoreProcess, &CoreProcess::error, this, &MainWindow::onCoreProcessError);

  connect(
      &m_CoreProcess, &CoreProcess::logLine, this, //
      [this](const QString &line) { handleLogLine(line); }
  );

  connect(&m_CoreProcess, &CoreProcess::processStateChanged, this, &MainWindow::onCoreProcessStateChanged);

  connect(&m_CoreProcess, &CoreProcess::connectionStateChanged, this, &MainWindow::onCoreConnectionStateChanged);

  connect(&m_CoreProcess, &CoreProcess::secureSocket, this, &MainWindow::onCoreProcessSecureSocket);

  connect(ui->m_pActionMinimize, &QAction::triggered, this, &MainWindow::hide);

  connect(
      ui->m_pActionRestore, &QAction::triggered, this, //
      [this]() { showAndActivate(); }
  );

  connect(ui->m_pActionQuit, &QAction::triggered, qApp, [this] {
    qDebug("quitting application");
    m_Quitting = true;
    QApplication::quit();
  });

  connect(&m_VersionChecker, &VersionChecker::updateFound, this, &MainWindow::onVersionCheckerUpdateFound);

  connect(&m_WindowSaveTimer, &QTimer::timeout, this, &MainWindow::onWindowSaveTimerTimeout);

  connect(&m_TrayIcon, &TrayIcon::activated, this, &MainWindow::onTrayIconActivated);

  connect(
      &m_ServerConnection, &ServerConnection::configureClient, this, &MainWindow::onServerConnectionConfigureClient
  );

  connect(&m_ServerConnection, &ServerConnection::messageShowing, this, [this]() { showAndActivate(); });

  connect(&m_ClientConnection, &ClientConnection::messageShowing, this, [this]() { showAndActivate(); });
}

void MainWindow::onAppAboutToQuit()
{
  if (m_SaveOnExit) {
    m_ConfigScopes.save();
  }
}

void MainWindow::onCreated()
{

  setIcon();

  m_ConfigScopes.signalReady();

  applyCloseToTray();

  updateScreenName();
  applyConfig();
  restoreWindow();

  qDebug().noquote() << "active settings path:" << m_ConfigScopes.activeFilePath();
}

void MainWindow::onShown()
{
  // if a critical error was shown just before the main window (i.e. on app
  // load), it will be hidden behind the main window. therefore we need to raise
  // it up in front of the main window.
  // HACK: because the `onShown` event happens just as the window is shown, the
  // message box has a chance of being raised under the main window. to solve
  // this we delay the error dialog raise by a split second. this seems a bit
  // hacky and fragile, so maybe there's a better approach.
  const auto kCriticalDialogDelay = 100;
  QTimer::singleShot(kCriticalDialogDelay, this, [] { messages::raiseCriticalDialog(); });
}

void MainWindow::onConfigScopesSaving()
{
  m_ServerConfig.commit();
}

void MainWindow::onAppConfigTlsChanged()
{
  updateLocalFingerprint();
  if (m_TlsUtility.isEnabled()) {
    m_TlsUtility.generateCertificate();
  }
}

void MainWindow::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason)
{

  if (reason == QSystemTrayIcon::DoubleClick) {
    if (isVisible()) {
      hide();
    } else {
      showAndActivate();
    }
  }
}

void MainWindow::onVersionCheckerUpdateFound(const QString &version)
{
  const auto link = QString(kLinkDownload).arg(kUrlDownload, kColorWhite);
  const auto text = QString("A new version is available (v%1). %2").arg(version, link);

  ui->m_pLabelUpdate->show();
  ui->m_pLabelUpdate->setText(text);
}

void MainWindow::onAppConfigScreenNameChanged()
{
  updateScreenName();
}

void MainWindow::onAppConfigInvertConnection()
{
  applyConfig();
}

void MainWindow::onCoreProcessError(CoreProcess::Error error)
{
  if (error == CoreProcess::Error::AddressMissing) {
    QMessageBox::warning(
        this, QString("Address missing"), QString("Please enter the hostname or IP address of the other computer.")
    );
  } else if (error == CoreProcess::Error::StartFailed) {
    show();
    QMessageBox::warning(
        this, QString("Core cannot be started"),
        "The Core executable could not be successfully started, "
        "although it does exist. "
        "Please check if you have sufficient permissions to run this program."
    );
  }
}

void MainWindow::on_m_pActionStartCore_triggered()
{
  m_ClientConnection.setShowMessage();
  m_CoreProcess.start();
}

void MainWindow::on_m_pActionStopCore_triggered()
{
  qDebug("stopping core process");
  m_CoreProcess.stop();
}

void MainWindow::on_m_pActionTestFatalError_triggered() const
{
  qFatal("test fatal error");
}

void MainWindow::on_m_pActionTestCriticalError_triggered() const
{
  qCritical("test critical error");
}

void MainWindow::on_m_pActionClearSettings_triggered()
{
  if (!messages::showClearSettings(this)) {
    qDebug("clear settings cancelled");
    return;
  }

  m_CoreProcess.stop();

  m_Quitting = true;
  m_SaveOnExit = false;
  diagnostic::clearSettings(m_ConfigScopes, true);
}

bool MainWindow::on_m_pActionSave_triggered()
{
  QString fileName = QFileDialog::getSaveFileName(this, QString("Save configuration as..."));

  if (!fileName.isEmpty() && !m_ServerConfig.save(fileName)) {
    QMessageBox::warning(this, QString("Save failed"), QString("Could not save configuration to file."));
    return true;
  }

  return false;
}

void MainWindow::on_m_pActionAbout_triggered()
{
  AboutDialog about(this);
  about.exec();
}

void MainWindow::on_m_pActionHelp_triggered() const
{
  QDesktopServices::openUrl(QUrl(kUrlHelp));
}

void MainWindow::on_m_pActionSettings_triggered()
{
  auto dialog = SettingsDialog(this, m_AppConfig, m_ServerConfig, m_CoreProcess);

  if (dialog.exec() == QDialog::Accepted) {
    m_ConfigScopes.save();

    applyConfig();
    applyCloseToTray();

    if (m_CoreProcess.isStarted()) {
      m_CoreProcess.restart();
    }
  }
}

void MainWindow::on_m_pButtonConfigureServer_clicked()
{
  showConfigureServer();
}

void MainWindow::on_m_pLineEditHostname_returnPressed()
{
  ui->m_pButtonConnect->click();
}

void MainWindow::on_m_pLineEditClientIp_returnPressed()
{
  ui->m_pButtonConnectToClient->click();
}

void MainWindow::on_m_pLineEditHostname_textChanged(const QString &text)
{
  m_CoreProcess.setAddress(text);
}

void MainWindow::on_m_pLineEditClientIp_textChanged(const QString &text)
{
  m_CoreProcess.setAddress(text);
}

void MainWindow::on_m_pButtonApply_clicked()
{
  m_ClientConnection.setShowMessage();
  m_CoreProcess.restart();
}

void MainWindow::on_m_pLabelComputerName_linkActivated(const QString &)
{
  ui->m_pActionSettings->trigger();
}

void MainWindow::on_m_pLabelFingerprint_linkActivated(const QString &)
{
  QMessageBox::information(this, "TLS fingerprint", TlsFingerprint::local().readFirst());
}

void MainWindow::on_m_pRadioGroupServer_clicked(bool)
{
  enableServer(true);
  enableClient(false);
  m_ConfigScopes.save();
}

void MainWindow::on_m_pRadioGroupClient_clicked(bool)
{
  enableClient(true);
  enableServer(false);
  m_ConfigScopes.save();
}

void MainWindow::on_m_pButtonConnect_clicked()
{
  on_m_pButtonApply_clicked();
}

void MainWindow::on_m_pButtonConnectToClient_clicked()
{
  on_m_pButtonApply_clicked();
}

void MainWindow::onWindowSaveTimerTimeout()
{
  saveWindow();
}

void MainWindow::onServerConnectionConfigureClient(const QString &clientName)
{
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

void MainWindow::resizeEvent(QResizeEvent *event)
{
  QMainWindow::resizeEvent(event);

  // postpone save so that settings are not written every delta change.
  m_WindowSaveTimer.setSingleShot(true);
  m_WindowSaveTimer.start(1000);
}

void MainWindow::moveEvent(QMoveEvent *event)
{
  QMainWindow::moveEvent(event);

  // postpone save so that settings are not written every delta change.
  m_WindowSaveTimer.setSingleShot(true);
  m_WindowSaveTimer.start(1000);
}

void MainWindow::open()
{

  std::vector<QAction *> trayMenu = {ui->m_pActionStartCore, ui->m_pActionStopCore, nullptr,
                                     ui->m_pActionMinimize,  ui->m_pActionRestore,  nullptr,
                                     ui->m_pActionQuit};

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

void MainWindow::onCoreProcessStarting()
{
  if (deskflow::platform::isWayland()) {
    m_WaylandWarnings.showOnce(this, m_CoreProcess.mode());
  }
  saveSettings();
}

void MainWindow::setStatus(const QString &status)
{
  ui->m_pStatusLabel->setText(status);
}

void MainWindow::createMenuBar()
{
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

  m_pMenuFile->addAction(ui->m_pActionStartCore);
  m_pMenuFile->addAction(ui->m_pActionStopCore);
  m_pMenuFile->addSeparator();
  m_pMenuFile->addAction(ui->m_pActionSave);
  m_pMenuFile->addSeparator();
  m_pMenuFile->addAction(ui->m_pActionQuit);

  m_pMenuEdit->addAction(ui->m_pActionSettings);

  m_pMenuWindow->addAction(ui->m_pActionMinimize);
  m_pMenuWindow->addAction(ui->m_pActionRestore);

  m_pMenuHelp->addAction(ui->m_pActionAbout);
  m_pMenuHelp->addAction(ui->m_pActionHelp);
  m_pMenuFile->addSeparator();
  m_pMenuHelp->addAction(ui->m_pActionClearSettings);

  ui->m_pActionAbout->setText(QString("About %1...").arg(kAppName));

  const auto enableTestMenu = strToTrue(qEnvironmentVariable("DESKFLOW_TEST_MENU"));

  if (enableTestMenu || kDebugBuild) {
    auto testMenu = new QMenu("Test", m_pMenuBar);
    m_pMenuBar->addMenu(testMenu);
    testMenu->addAction(ui->m_pActionTestFatalError);
    testMenu->addAction(ui->m_pActionTestCriticalError);
  }

  setMenuBar(m_pMenuBar);
}

void MainWindow::applyConfig()
{
  enableServer(m_AppConfig.serverGroupChecked());
  enableClient(m_AppConfig.clientGroupChecked());

  ui->m_pLineEditHostname->setText(m_AppConfig.serverHostname());
  ui->m_pLineEditClientIp->setText(m_ServerConfig.getClientAddress());
}

void MainWindow::applyCloseToTray() const
{
  QApplication::setQuitOnLastWindowClosed(!m_AppConfig.closeToTray());
}

void MainWindow::saveSettings()
{
  m_AppConfig.setServerGroupChecked(ui->m_pRadioGroupServer->isChecked());
  m_AppConfig.setClientGroupChecked(ui->m_pRadioGroupClient->isChecked());
  m_AppConfig.setServerHostname(ui->m_pLineEditHostname->text());
  m_ServerConfig.setClientAddress(ui->m_pLineEditClientIp->text());

  m_ConfigScopes.save();
}

void MainWindow::setIcon()
{
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
  icon.addFile(kIconFile);
#endif

  m_TrayIcon.setIcon(icon);
}

void MainWindow::handleLogLine(const QString &line)
{
  const int kScrollBottomThreshold = 2;

  QScrollBar *verticalScroll = ui->m_pLogOutput->verticalScrollBar();
  int currentScroll = verticalScroll->value();
  int maxScroll = verticalScroll->maximum();
  const auto scrollAtBottom = qAbs(currentScroll - maxScroll) <= kScrollBottomThreshold;

  // only trim end instead of the whole line to prevent tab-indented debug
  // filenames from losing their indentation.
  ui->m_pLogOutput->appendPlainText(trimEnd(line));

  if (scrollAtBottom) {
    verticalScroll->setValue(verticalScroll->maximum());
    ui->m_pLogOutput->horizontalScrollBar()->setValue(0);
  }

  updateFromLogLine(line);
}

void MainWindow::updateFromLogLine(const QString &line)
{
  checkConnected(line);
  checkFingerprint(line);
}

void MainWindow::checkConnected(const QString &line)
{
  if (ui->m_pRadioGroupServer->isChecked()) {
    m_ServerConnection.handleLogLine(line);
    ui->m_pLabelServerState->updateServerState(line);
  } else {
    m_ClientConnection.handleLogLine(line);
    ui->m_pLabelClientState->updateClientState(line);
  }
}

void MainWindow::checkFingerprint(const QString &line)
{
  static const QRegularExpression re(".*server fingerprint: ([A-F0-9:]+)");
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
        QString("<p>You are connecting to a server.</p>"
                "<p>Here is it's TLS fingerprint:</p>"
                "<p>%1</p>"
                "<p>Compare this fingerprint to the one on your server's screen. "
                "If the two don't match exactly, then it's probably not the server "
                "you're expecting (it could be a malicious user).</p>"
                "<p>Do you want to trust this fingerprint for future "
                "connections? If you don't, a connection cannot be made.</p>")
            .arg(fingerprint),
        QMessageBox::Yes | QMessageBox::No
    );

    if (fingerprintReply == QMessageBox::Yes) {
      // start core process again after trusting fingerprint.
      TlsFingerprint::trustedServers().trust(fingerprint);
      m_CoreProcess.start();
    }

    messageBoxAlreadyShown = false;
  }
}

QString MainWindow::getTimeStamp() const
{
  QDateTime current = QDateTime::currentDateTime();
  return '[' + current.toString(Qt::ISODate) + ']';
}

void MainWindow::showEvent(QShowEvent *event)
{
  QMainWindow::showEvent(event);
  Q_EMIT shown();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
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

void MainWindow::showFirstConnectedMessage()
{
  if (m_AppConfig.startedBefore()) {
    return;
  }

  m_AppConfig.setStartedBefore(true);
  m_ConfigScopes.save();

  const auto isServer = m_CoreProcess.mode() == CoreMode::Server;
  messages::showFirstConnectedMessage(this, m_AppConfig.closeToTray(), m_AppConfig.enableService(), isServer);
}

void MainWindow::showDevThanksMessage()
{
  if (!m_AppConfig.showDevThanks()) {
    qDebug("skipping dev thanks message");
    return;
  }

  m_AppConfig.setShowDevThanks(false);
  m_ConfigScopes.save();

  messages::showDevThanks(this, kAppName);
}

void MainWindow::onCoreProcessSecureSocket(bool enabled)
{
  secureSocket(enabled);
}

void MainWindow::updateStatus()
{
  const auto connection = m_CoreProcess.connectionState();
  const auto process = m_CoreProcess.processState();

  switch (process) {
    using enum CoreProcessState;

  case Starting:
    setStatus(QString("%1 is starting...").arg(kAppName));
    break;

  case RetryPending:
    setStatus(QString("%1 will retry in a moment...").arg(kAppName));
    break;

  case Stopping:
    setStatus(QString("%1 is stopping...").arg(kAppName));
    break;

  case Stopped:
    setStatus(QString("%1 is not running").arg(kAppName));
    break;

  case Started: {
    switch (connection) {
      using enum CoreConnectionState;

    case Listening: {
      if (m_CoreProcess.mode() == CoreMode::Server) {
        setStatus(QString("%1 is waiting for clients").arg(kAppName));
      }

      break;
    }

    case Connecting:
      setStatus(QString("%1 is connecting...").arg(kAppName));
      break;

    case Connected: {
      if (m_SecureSocket) {
        setStatus(QString("%1 is connected (with %2)").arg(kAppName, m_CoreProcess.secureSocketVersion()));
      } else {
        setStatus(QString("%1 is connected (without TLS encryption)").arg(kAppName));
      }
      break;
    }

    case Disconnected:
      setStatus(QString("%1 is disconnected").arg(kAppName));
      break;
    }
  } break;
  }
}

void MainWindow::onCoreProcessStateChanged(CoreProcessState state)
{
  updateStatus();

  if (state == CoreProcessState::Started) {
    qDebug("recording that core has started");
    m_AppConfig.setStartedBefore(true);
    m_ConfigScopes.save();
  }

  if (state == CoreProcessState::Started || state == CoreProcessState::Starting ||
      state == CoreProcessState::RetryPending) {
    disconnect(ui->m_pButtonToggleStart, &QPushButton::clicked, ui->m_pActionStartCore, &QAction::trigger);
    connect(ui->m_pButtonToggleStart, &QPushButton::clicked, ui->m_pActionStopCore, &QAction::trigger);

    ui->m_pButtonToggleStart->setText(QString("&Stop"));
    ui->m_pButtonApply->setEnabled(true);

    ui->m_pActionStartCore->setEnabled(false);
    ui->m_pActionStopCore->setEnabled(true);

  } else {
    disconnect(ui->m_pButtonToggleStart, &QPushButton::clicked, ui->m_pActionStopCore, &QAction::trigger);
    connect(ui->m_pButtonToggleStart, &QPushButton::clicked, ui->m_pActionStartCore, &QAction::trigger);

    ui->m_pButtonToggleStart->setText(QString("&Start"));
    ui->m_pButtonApply->setEnabled(false);

    ui->m_pActionStartCore->setEnabled(true);
    ui->m_pActionStopCore->setEnabled(false);
  }
}

void MainWindow::onCoreConnectionStateChanged(CoreConnectionState state)
{
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

void MainWindow::setVisible(bool visible)
{
  QMainWindow::setVisible(visible);
  ui->m_pActionMinimize->setEnabled(visible);
  ui->m_pActionRestore->setEnabled(!visible);

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

QString MainWindow::getIPAddresses() const
{
  QStringList result;
  bool hinted = false;
  const auto localnet = QHostAddress::parseSubnet("192.168.0.0/16");
  const QList<QHostAddress> addresses = QNetworkInterface::allAddresses();

  for (const auto &address : addresses) {
    if (address.protocol() == QAbstractSocket::IPv4Protocol && address != QHostAddress(QHostAddress::LocalHost) &&
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

void MainWindow::updateLocalFingerprint()
{
  bool fingerprintExists = false;
  try {
    fingerprintExists = TlsFingerprint::local().fileExists();
  } catch (const std::exception &e) {
    qDebug() << e.what();
    qFatal("failed to check if fingerprint exists");
  }

  if (m_AppConfig.tlsEnabled() && fingerprintExists && ui->m_pRadioGroupServer->isChecked()) {
    ui->m_pLabelFingerprint->setVisible(true);
  } else {
    ui->m_pLabelFingerprint->setVisible(false);
  }
}

void MainWindow::autoAddScreen(const QString name)
{

  int r = m_ServerConfig.autoAddScreen(name);
  if (r != kAutoAddScreenOk) {
    switch (r) {
    case kAutoAddScreenManualServer:
      showConfigureServer(QString("Please add the server (%1) to the grid.").arg(m_AppConfig.screenName()));
      break;

    case kAutoAddScreenManualClient:
      showConfigureServer(QString("Please drag the new client screen (%1) "
                                  "to the desired position on the grid.")
                              .arg(name));
      break;
    }
  }
}

void MainWindow::showConfigureServer(const QString &message)
{
  ServerConfigDialog dialog(this, serverConfig(), m_AppConfig);
  dialog.message(message);
  if ((dialog.exec() == QDialog::Accepted) && m_CoreProcess.isStarted()) {
    m_CoreProcess.restart();
  }
}

void MainWindow::secureSocket(bool secureSocket)
{
  m_SecureSocket = secureSocket;
  if (secureSocket) {
    ui->m_pLabelPadlock->show();
  } else {
    ui->m_pLabelPadlock->hide();
  }
}

void MainWindow::updateScreenName()
{
  ui->m_pLabelComputerName->setText(QString("This computer's name: %1 "
                                            R"((<a href="#" style="color: %2">change</a>))")
                                        .arg(m_AppConfig.screenName(), kColorSecondary));
  m_ServerConfig.updateServerName();
}

void MainWindow::enableServer(bool enable)
{
  qDebug(enable ? "server enabled" : "server disabled");
  m_AppConfig.setServerGroupChecked(enable);
  ui->m_pRadioGroupServer->setChecked(enable);
  ui->m_pWidgetServer->setEnabled(enable);
  ui->m_pWidgetServerInput->setVisible(m_AppConfig.invertConnection());

  if (enable) {
    ui->m_pButtonToggleStart->setEnabled(true);
    ui->m_pActionStartCore->setEnabled(true);
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

void MainWindow::enableClient(bool enable)
{
  qDebug(enable ? "client enabled" : "client disabled");
  m_AppConfig.setClientGroupChecked(enable);
  ui->m_pRadioGroupClient->setChecked(enable);
  ui->m_pWidgetClientInput->setEnabled(enable);
  ui->m_pWidgetClientInput->setVisible(!m_AppConfig.invertConnection());

  if (enable) {
    ui->m_pButtonToggleStart->setEnabled(true);
    ui->m_pActionStartCore->setEnabled(true);
    m_CoreProcess.setMode(CoreProcess::Mode::Client);
  }
}

void MainWindow::showAndActivate()
{
  if (!isMinimized() && !isHidden()) {
    qDebug("window already visible");
    return;
  }

  showNormal();
  raise();
  activateWindow();
}
