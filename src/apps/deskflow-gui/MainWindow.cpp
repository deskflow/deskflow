/*
 * Deskflow -- mouse and keyboard sharing utility
 * Copyright (C) 2024 Chris Rizzitello <sithlord48@gmail.com>
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

#include "dialogs/AboutDialog.h"
#include "dialogs/ServerConfigDialog.h"
#include "dialogs/SettingsDialog.h"

#include "common/constants.h"
#include "gui/Logger.h"
#include "gui/config/ConfigScopes.h"
#include "gui/constants.h"
#include "gui/core/CoreProcess.h"
#include "gui/diagnostic.h"
#include "gui/messages.h"
#include "gui/string_utils.h"
#include "gui/styles.h"
#include "gui/tls/TlsFingerprint.h"
#include "platform/wayland.h"

#if defined(Q_OS_LINUX)
#include "config.h"
#endif

#include <QApplication>
#include <QDesktopServices>
#include <QFileDialog>
#include <QLocalServer>
#include <QLocalSocket>
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
      m_WindowSaveTimer(this),
      m_guiDupeChecker{new QLocalServer(this)},
      m_actionAbout{new QAction(this)},
      m_actionClearSettings{new QAction(tr("Clear settings"), this)},
      m_actionHelp{new QAction(tr("Report a Bug"), this)},
      m_actionMinimize{new QAction(tr("&Hide"), this)},
      m_actionQuit{new QAction(tr("&Quit"), this)},
      m_actionRestore{new QAction(tr("Show"), this)},
      m_actionSave{new QAction(tr("Save configuration &as..."), this)},
      m_actionSettings{new QAction(tr("Preferences"), this)},
      m_actionStartCore{new QAction(tr("&Start"), this)},
      m_actionStopCore{new QAction(tr("S&top"), this)},
      m_actionTestCriticalError{new QAction(tr("Test Critical Error"), this)},
      m_actionTestFatalError{new QAction(tr("Test Fatal Error"), this)}
{

  ui->setupUi(this);

  // Setup Actions
  m_actionAbout->setText(tr("About %1...").arg(kAppName));
  m_actionAbout->setMenuRole(QAction::AboutRole);

  m_actionQuit->setShortcut(QKeySequence::Quit);
  m_actionQuit->setMenuRole(QAction::QuitRole);

  m_actionSettings->setMenuRole(QAction::PreferencesRole);
  m_actionSave->setShortcut(QKeySequence(tr("Ctrl+Alt+S")));
  m_actionStartCore->setShortcut(QKeySequence(tr("Ctrl+S")));
  m_actionStopCore->setShortcut(QKeySequence(tr("Ctrl+T")));

#ifdef Q_OS_MAC
  ui->btnToggleLog->setFixedHeight(ui->lblLog->height() * 0.6);
#endif

  ui->btnToggleLog->setStyleSheet(QStringLiteral("background:rgba(0,0,0,0);"));
  if (m_AppConfig.logExpanded())
    ui->btnToggleLog->click();

  toggleLogVisible(m_AppConfig.logExpanded());

  // Setup the Instance Checking
  // In case of a previous crash remove first
  m_guiDupeChecker->removeServer(m_guiSocketName);
  m_guiDupeChecker->listen(m_guiSocketName);

  createMenuBar();
  setupControls();
  connectSlots();

  setIcon();

  m_ConfigScopes.signalReady();

  updateScreenName();
  applyConfig();
  restoreWindow();

  qDebug().noquote() << "active settings path:" << m_ConfigScopes.activeFilePath();

  updateSize();
}

MainWindow::~MainWindow()
{
  m_guiDupeChecker->close();
  m_CoreProcess.cleanup();
}

void MainWindow::restoreWindow()
{
  const auto &windowSize = m_AppConfig.mainWindowSize();
  if (windowSize.has_value()) {
    qDebug() << "restoring main window size";
    resize(windowSize.value());
  }

  const auto &windowPosition = m_AppConfig.mainWindowPosition();
  if (windowPosition.has_value()) {
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;
    for (auto screen : QGuiApplication::screens()) {
      auto geo = screen->geometry();
      x = std::min(geo.x(), x);
      y = std::min(geo.y(), y);
      w = std::max(geo.x() + geo.width(), w);
      h = std::max(geo.y() + geo.height(), h);
    }
    const QSize totalScreenSize(w, h);
    const QPoint point = windowPosition.value();
    if (point.x() < totalScreenSize.width() && point.y() < totalScreenSize.height()) {
      qDebug() << "restoring main window position";
      move(point);
    }
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
    qDebug() << "not yet ready to save window size and position, skipping";
    return;
  }

  qDebug() << "saving window size and position";
  m_AppConfig.setMainWindowSize(size());
  m_AppConfig.setMainWindowPosition(pos());
  m_ConfigScopes.save();
}

void MainWindow::setupControls()
{
  setWindowTitle(kAppName);

  secureSocket(false);

  ui->m_pLabelUpdate->setStyleSheet(kStyleNoticeLabel);
  ui->m_pLabelUpdate->hide();

  ui->m_pLabelNotice->setStyleSheet(kStyleNoticeLabel);
  ui->m_pLabelNotice->hide();

  ui->m_pLabelIpAddresses->setText(tr("This computer's IP addresses: %1").arg(getIPAddresses()));

  if (m_AppConfig.lastVersion() != kVersion) {
    m_AppConfig.setLastVersion(kVersion);
  }

#if defined(Q_OS_MAC)

  ui->rbModeServer->setAttribute(Qt::WA_MacShowFocusRect, 0);
  ui->rbModeClient->setAttribute(Qt::WA_MacShowFocusRect, 0);

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

  connect(m_actionAbout, &QAction::triggered, this, &MainWindow::openAboutDialog);
  connect(m_actionClearSettings, &QAction::triggered, this, &MainWindow::clearSettings);
  connect(m_actionHelp, &QAction::triggered, this, &MainWindow::openHelpUrl);
  connect(m_actionMinimize, &QAction::triggered, this, &MainWindow::hide);

  connect(m_actionQuit, &QAction::triggered, this, &MainWindow::close);
  connect(m_actionRestore, &QAction::triggered, this, &MainWindow::showAndActivate);
  connect(m_actionSave, &QAction::triggered, this, &MainWindow::saveConfig);
  connect(m_actionSettings, &QAction::triggered, this, &MainWindow::openSettings);
  connect(m_actionStartCore, &QAction::triggered, this, &MainWindow::startCore);
  connect(m_actionStopCore, &QAction::triggered, this, &MainWindow::stopCore);
  connect(m_actionTestFatalError, &QAction::triggered, this, &MainWindow::testFatalError);
  connect(m_actionTestCriticalError, &QAction::triggered, this, &MainWindow::testCriticalError);

  connect(&m_VersionChecker, &VersionChecker::updateFound, this, &MainWindow::onVersionCheckerUpdateFound);

  connect(&m_WindowSaveTimer, &QTimer::timeout, this, &MainWindow::onWindowSaveTimerTimeout);

// Mac os tray will only show a menu
#ifndef Q_OS_MAC
  connect(&m_TrayIcon, &TrayIcon::activated, this, &MainWindow::onTrayIconActivated);
#endif

  connect(
      &m_ServerConnection, &ServerConnection::configureClient, this, &MainWindow::onServerConnectionConfigureClient
  );

  connect(&m_ServerConnection, &ServerConnection::messageShowing, this, &MainWindow::showAndActivate);
  connect(&m_ClientConnection, &ClientConnection::messageShowing, this, &MainWindow::showAndActivate);

  connect(ui->btnToggleCore, &QPushButton::clicked, m_actionStartCore, &QAction::trigger, Qt::UniqueConnection);
  connect(ui->btnApplySettings, &QPushButton::clicked, this, &MainWindow::resetCore);
  connect(ui->btnConnect, &QPushButton::clicked, this, &MainWindow::resetCore);
  connect(ui->btnConnectToClient, &QPushButton::clicked, this, &MainWindow::resetCore);

  connect(ui->lineHostname, &QLineEdit::returnPressed, ui->btnConnect, &QPushButton::click);
  connect(ui->lineHostname, &QLineEdit::textChanged, &m_CoreProcess, &deskflow::gui::CoreProcess::setAddress);

  connect(ui->lineClientIp, &QLineEdit::returnPressed, ui->btnConnectToClient, &QPushButton::click);
  connect(ui->lineClientIp, &QLineEdit::textChanged, &m_CoreProcess, &deskflow::gui::CoreProcess::setAddress);

  connect(ui->btnConfigureServer, &QPushButton::clicked, this, [this] { showConfigureServer(""); });
  connect(ui->lblComputerName, &QLabel::linkActivated, this, &MainWindow::openSettings);
  connect(ui->lblMyFingerprint, &QLabel::linkActivated, this, &MainWindow::showMyFingerprint);

  connect(ui->rbModeServer, &QRadioButton::clicked, this, &MainWindow::setModeServer);
  connect(ui->rbModeClient, &QRadioButton::clicked, this, &MainWindow::setModeClient);

  connect(ui->btnToggleLog, &QAbstractButton::toggled, this, &MainWindow::toggleLogVisible);

  connect(m_guiDupeChecker, &QLocalServer::newConnection, this, &MainWindow::showAndActivate);
}

void MainWindow::toggleLogVisible(bool visible)
{
  if (visible) {
    ui->btnToggleLog->setArrowType(Qt::DownArrow);
    ui->textLog->setVisible(true);
    m_AppConfig.setLogExpanded(true);
  } else {
    ui->btnToggleLog->setArrowType(Qt::RightArrow);
    m_expandedSize = size();
    ui->textLog->setVisible(false);
    m_AppConfig.setLogExpanded(false);
  }
  // 10 ms is long enough to process events and quick enough to not see the visual change.
  QTimer::singleShot(10, this, &MainWindow::updateSize);
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
  if (m_TlsUtility.isEnabled()) {
    m_TlsUtility.generateCertificate();
  }
}

void MainWindow::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
  if (reason != QSystemTrayIcon::Trigger)
    return;
  isVisible() ? hide() : showAndActivate();
}

void MainWindow::onVersionCheckerUpdateFound(const QString &version)
{
  const auto link = QString(kLinkDownload).arg(kUrlDownload, kColorWhite);
  const auto text = tr("A new version is available (v%1). %2").arg(version, link);

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
        this, tr("Address missing"), tr("Please enter the hostname or IP address of the other computer.")
    );
  } else if (error == CoreProcess::Error::StartFailed) {
    show();
    QMessageBox::warning(
        this, tr("Core cannot be started"),
        "The Core executable could not be successfully started, "
        "although it does exist. "
        "Please check if you have sufficient permissions to run this program."
    );
  }
}

void MainWindow::startCore()
{
  m_ClientConnection.setShowMessage();
  m_CoreProcess.start();
}

void MainWindow::stopCore()
{
  qDebug() << "stopping core process";
  m_CoreProcess.stop();
}

void MainWindow::testFatalError() const
{
  qFatal() << "test fatal error";
}

void MainWindow::testCriticalError() const
{
  qCritical() << "test critical error";
}

void MainWindow::clearSettings()
{
  if (!messages::showClearSettings(this)) {
    qDebug() << "clear settings cancelled";
    return;
  }

  m_CoreProcess.stop();

  m_SaveOnExit = false;
  diagnostic::clearSettings(m_ConfigScopes, true);
}

bool MainWindow::saveConfig()
{
  QString fileName = QFileDialog::getSaveFileName(this, tr("Save configuration as..."));

  if (!fileName.isEmpty() && !m_ServerConfig.save(fileName)) {
    QMessageBox::warning(this, tr("Save failed"), tr("Could not save configuration to file."));
    return true;
  }

  return false;
}

void MainWindow::openAboutDialog()
{
  AboutDialog about(this);
  about.exec();
}

void MainWindow::openHelpUrl() const
{
  QDesktopServices::openUrl(QUrl(kUrlHelp));
}

void MainWindow::openSettings()
{
  auto dialog = SettingsDialog(this, m_AppConfig, m_ServerConfig, m_CoreProcess);

  if (dialog.exec() == QDialog::Accepted) {
    m_ConfigScopes.save();

    applyConfig();

    if (m_CoreProcess.isStarted()) {
      m_CoreProcess.restart();
    }
  }
}

void MainWindow::resetCore()
{
  m_ClientConnection.setShowMessage();
  m_CoreProcess.restart();
}

void MainWindow::updateSize()
{
#ifdef Q_OS_MAC
  // On mac os the titlebar is part of the height so we need to adjust our Y coord to avoid moving the window up
  const auto kTitleBarOffset = 28;
#else
  const auto kTitleBarOffset = 0;
#endif
  if (ui->textLog->isVisible()) {
    setMaximumHeight(16777215);
    setMaximumWidth(16777215);
    setGeometry(x(), y() + kTitleBarOffset, m_expandedSize.width(), m_expandedSize.height());
  } else {
    adjustSize();
    // Prevent Resize with log collapsed
    setMaximumHeight(height());
    setMaximumWidth(width());
  }
}

void MainWindow::showMyFingerprint()
{
  QMessageBox::information(this, "TLS fingerprint", TlsFingerprint::local().readFirst());
}

void MainWindow::setModeServer()
{
  enableServer(true);
  enableClient(false);
  m_ConfigScopes.save();
}

void MainWindow::setModeClient()
{
  enableClient(true);
  enableServer(false);
  m_ConfigScopes.save();
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

  QList<QAction *> trayActions{m_actionStartCore, m_actionStopCore, nullptr, m_actionQuit};

#ifdef Q_OS_MAC
  // Duplicate quit needed for mac os tray menu
  QAction *actionTrayQuit = new QAction(tr("Quit Deskflow"), this);
  actionTrayQuit->setShortcut(QKeySequence::Quit);
  connect(actionTrayQuit, &QAction::triggered, this, &MainWindow::close);

  m_actionRestore->setText(tr("Open Deskflow"));
  trayActions.insert(3, m_actionRestore);
  trayActions.append(nullptr);
  trayActions.append(actionTrayQuit);
#endif

  m_TrayIcon.create(trayActions);

  if (!m_AppConfig.enableUpdateCheck().has_value()) {
    m_AppConfig.setEnableUpdateCheck(messages::showUpdateCheckOption(this));
    m_ConfigScopes.save();
  }

  if (m_AppConfig.enableUpdateCheck().value()) {
    m_VersionChecker.checkLatest();
  } else {
    qDebug() << "update check disabled";
  }

  if (m_AppConfig.startedBefore()) {
    m_CoreProcess.start();
  }

  if (m_AppConfig.autoHide()) {
    hide();
  } else {
    showAndActivate();
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
  auto menuFile = new QMenu(tr("File"));
  menuFile->addAction(m_actionStartCore);
  menuFile->addAction(m_actionStopCore);
  menuFile->addSeparator();
  menuFile->addAction(m_actionSave);
  menuFile->addSeparator();
  menuFile->addAction(m_actionQuit);

  auto menuEdit = new QMenu(tr("Edit"));
  menuEdit->addAction(m_actionSettings);

  auto menuWindow = new QMenu(tr("Window"));
  menuWindow->addAction(m_actionMinimize);

  auto menuHelp = new QMenu(tr("Help"));
  menuHelp->addAction(m_actionAbout);
  menuHelp->addAction(m_actionHelp);
  menuHelp->addSeparator();
  menuHelp->addAction(m_actionClearSettings);

  auto menuBar = new QMenuBar(this);
  menuBar->addMenu(menuFile);
  menuBar->addMenu(menuEdit);
#if !defined(Q_OS_MAC)
  menuBar->addMenu(menuWindow);
#endif
  menuBar->addMenu(menuHelp);

  const auto enableTestMenu = strToTrue(qEnvironmentVariable("DESKFLOW_TEST_MENU"));
  if (enableTestMenu || kDebugBuild) {
    auto testMenu = new QMenu(tr("Test"));
    menuBar->addMenu(testMenu);
    testMenu->addAction(m_actionTestFatalError);
    testMenu->addAction(m_actionTestCriticalError);
  }

  setMenuBar(menuBar);
}

void MainWindow::applyConfig()
{
  enableServer(m_AppConfig.serverGroupChecked());
  enableClient(m_AppConfig.clientGroupChecked());

  ui->lineHostname->setText(m_AppConfig.serverHostname());
  ui->lineClientIp->setText(m_ServerConfig.getClientAddress());
  updateLocalFingerprint();
}

void MainWindow::saveSettings()
{
  m_AppConfig.setServerGroupChecked(ui->rbModeServer->isChecked());
  m_AppConfig.setClientGroupChecked(ui->rbModeClient->isChecked());
  m_AppConfig.setServerHostname(ui->lineHostname->text());
  m_ServerConfig.setClientAddress(ui->lineClientIp->text());

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

  QScrollBar *verticalScroll = ui->textLog->verticalScrollBar();
  int currentScroll = verticalScroll->value();
  int maxScroll = verticalScroll->maximum();
  const auto scrollAtBottom = qAbs(currentScroll - maxScroll) <= kScrollBottomThreshold;

  // only trim end instead of the whole line to prevent tab-indented debug
  // filenames from losing their indentation.
  ui->textLog->appendPlainText(trimEnd(line));

  if (scrollAtBottom) {
    verticalScroll->setValue(verticalScroll->maximum());
    ui->textLog->horizontalScrollBar()->setValue(0);
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
  if (ui->rbModeServer->isChecked()) {
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
        this, tr("Security question"),
        tr("<p>You are connecting to a server.</p>"
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
  return QStringLiteral("[%1]").arg(QDateTime::currentDateTime().toString(Qt::ISODate));
}

void MainWindow::showEvent(QShowEvent *event)
{
  QMainWindow::showEvent(event);
  Q_EMIT shown();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
  if (m_AppConfig.closeToTray() && event->spontaneous()) {
    if (m_AppConfig.showCloseReminder()) {
      messages::showCloseReminder(this);
      m_AppConfig.setShowCloseReminder(false);
    }
    qDebug() << "hiding to tray";
    hide();
    event->ignore();
    return;
  }

  if (m_SaveOnExit)
    m_ConfigScopes.save();

  qDebug() << "quitting application";
  event->accept();
  QApplication::quit();
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
    setStatus(tr("%1 is starting...").arg(kAppName));
    break;

  case RetryPending:
    setStatus(tr("%1 will retry in a moment...").arg(kAppName));
    break;

  case Stopping:
    setStatus(tr("%1 is stopping...").arg(kAppName));
    break;

  case Stopped:
    setStatus(tr("%1 is not running").arg(kAppName));
    break;

  case Started: {
    switch (connection) {
      using enum CoreConnectionState;

    case Listening: {
      if (m_CoreProcess.mode() == CoreMode::Server) {
        setStatus(tr("%1 is waiting for clients").arg(kAppName));
      }

      break;
    }

    case Connecting:
      setStatus(tr("%1 is connecting...").arg(kAppName));
      break;

    case Connected: {
      if (m_SecureSocket) {
        setStatus(tr("%1 is connected (with %2)").arg(kAppName, m_CoreProcess.secureSocketVersion()));
      } else {
        setStatus(tr("%1 is connected (without TLS encryption)").arg(kAppName));
      }
      break;
    }

    case Disconnected:
      setStatus(tr("%1 is disconnected").arg(kAppName));
      break;
    }
  } break;
  }
}

void MainWindow::onCoreProcessStateChanged(CoreProcessState state)
{
  updateStatus();

  if (state == CoreProcessState::Started) {
    qDebug() << "recording that core has started";
    m_AppConfig.setStartedBefore(true);
    m_ConfigScopes.save();
  }

  if (state == CoreProcessState::Started || state == CoreProcessState::Starting ||
      state == CoreProcessState::RetryPending) {
    disconnect(ui->btnToggleCore, &QPushButton::clicked, m_actionStartCore, &QAction::trigger);
    connect(ui->btnToggleCore, &QPushButton::clicked, m_actionStopCore, &QAction::trigger, Qt::UniqueConnection);

    ui->btnToggleCore->setText(tr("&Stop"));
    ui->btnApplySettings->setEnabled(true);

    m_actionStartCore->setEnabled(false);
    m_actionStopCore->setEnabled(true);

  } else {
    disconnect(ui->btnToggleCore, &QPushButton::clicked, m_actionStopCore, &QAction::trigger);
    connect(ui->btnToggleCore, &QPushButton::clicked, m_actionStartCore, &QAction::trigger, Qt::UniqueConnection);

    ui->btnToggleCore->setText(tr("&Start"));
    ui->btnApplySettings->setEnabled(false);

    m_actionStartCore->setEnabled(true);
    m_actionStopCore->setEnabled(false);
  }
}

void MainWindow::onCoreConnectionStateChanged(CoreConnectionState state)
{
  qDebug() << "core connection state changed: " << static_cast<int>(state);

  updateStatus();

  // always assume connection is not secure when connection changes
  // to anything except connected. the only way the padlock shows is
  // when the correct TLS version string is detected.
  if (state != CoreConnectionState::Connected) {
    secureSocket(false);
  } else if (isVisible()) {
    showFirstConnectedMessage();
  }
}

void MainWindow::setVisible(bool visible)
{
  QMainWindow::setVisible(visible);
  m_actionMinimize->setEnabled(visible);
#ifndef Q_OS_MAC
  m_actionRestore->setEnabled(!visible);
#else
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
    qFatal() << "failed to check if fingerprint exists";
  }

  if (m_AppConfig.tlsEnabled() && fingerprintExists) {
    ui->lblMyFingerprint->setVisible(true);
  } else {
    ui->lblMyFingerprint->setVisible(false);
  }
}

void MainWindow::autoAddScreen(const QString name)
{

  int r = m_ServerConfig.autoAddScreen(name);
  if (r != kAutoAddScreenOk) {
    switch (r) {
    case kAutoAddScreenManualServer:
      showConfigureServer(tr("Please add the server (%1) to the grid.").arg(m_AppConfig.screenName()));
      break;

    case kAutoAddScreenManualClient:
      showConfigureServer(tr("Please drag the new client screen (%1) "
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
  ui->lblComputerName->setText(tr("This computer's name: %1 "
                                  R"((<a href="#" style="color: %2">change</a>))")
                                   .arg(m_AppConfig.screenName(), kColorSecondary));
  m_ServerConfig.updateServerName();
}

void MainWindow::enableServer(bool enable)
{
  QString serverStr = enable ? QStringLiteral("server enabled") : QStringLiteral("server disabled");
  qDebug() << serverStr;
  m_AppConfig.setServerGroupChecked(enable);
  ui->rbModeServer->setChecked(enable);
  ui->m_pWidgetServer->setEnabled(enable);
  ui->m_pWidgetServerInput->setVisible(m_AppConfig.invertConnection());

  if (enable) {
    ui->btnToggleCore->setEnabled(true);
    m_actionStartCore->setEnabled(true);

    if (m_CoreProcess.isStarted() && m_CoreProcess.mode() != CoreProcess::Mode::Server)
      m_CoreProcess.stop();

    m_CoreProcess.setMode(CoreProcess::Mode::Server);

    // The server can run without any clients configured, and this is actually
    // what you'll want to do the first time since you'll be prompted when an
    // unrecognized client tries to connect.
    if (!m_AppConfig.startedBefore() && !m_CoreProcess.isStarted()) {
      qDebug() << "auto-starting core server for first time";
      m_CoreProcess.start();
      messages::showFirstServerStartMessage(this);
    }
  }
}

void MainWindow::enableClient(bool enable)
{
  QString clientStr = enable ? QStringLiteral("client enabled") : QStringLiteral("client disabled");
  qDebug() << clientStr;
  m_AppConfig.setClientGroupChecked(enable);
  ui->rbModeClient->setChecked(enable);
  ui->m_pWidgetClientInput->setEnabled(enable);
  ui->m_pWidgetClientInput->setVisible(!m_AppConfig.invertConnection());

  if (enable) {
    ui->btnToggleCore->setEnabled(true);
    m_actionStartCore->setEnabled(true);
    if (m_CoreProcess.isStarted() && m_CoreProcess.mode() != CoreProcess::Mode::Client)
      m_CoreProcess.stop();
    m_CoreProcess.setMode(CoreProcess::Mode::Client);
  }
}

void MainWindow::showAndActivate()
{
#ifdef Q_OS_MAC
  forceAppActive();
#endif
  showNormal();
  raise();
  activateWindow();
}
