/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Chris Rizzitello <sithord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2012 - 2024 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2008 Volker Lanz <vl@fidra.de>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
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
#include "gui/style_utils.h"
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

MainWindow::MainWindow(ConfigScopes &configScopes, AppConfig &appConfig)
    : ui{std::make_unique<Ui::MainWindow>()},
      m_configScopes(configScopes),
      m_appConfig(appConfig),
      m_serverConfig(appConfig, *this),
      m_coreProcess(appConfig, m_serverConfig),
      m_serverConnection(this, appConfig, m_serverConfig, m_serverConfigDialogState),
      m_clientConnection(this, appConfig),
      m_tlsUtility(appConfig),
      m_trayIcon{new QSystemTrayIcon(this)},
      m_guiDupeChecker{new QLocalServer(this)},
      m_actionAbout{new QAction(this)},
      m_actionClearSettings{new QAction(tr("Clear settings"), this)},
      m_actionReportBug{new QAction(tr("Report a Bug"), this)},
      m_actionMinimize{new QAction(tr("&Minimize to tray"), this)},
      m_actionQuit{new QAction(tr("&Quit"), this)},
      m_actionTrayQuit{new QAction(tr("&Quit"), this)},
      m_actionRestore{new QAction(tr("&Open Deskflow"), this)},
      m_actionSave{new QAction(tr("Save configuration &as..."), this)},
      m_actionSettings{new QAction(tr("Preferences"), this)},
      m_actionStartCore{new QAction(tr("&Start"), this)},
      m_actionStopCore{new QAction(tr("S&top"), this)},
      m_actionTestCriticalError{new QAction(tr("Test Critical Error"), this)},
      m_actionTestFatalError{new QAction(tr("Test Fatal Error"), this)}
{
  const auto themeName = QStringLiteral("deskflow-%1").arg(iconMode());
  if (QIcon::themeName().isEmpty())
    QIcon::setThemeName(themeName);
  else
    QIcon::setFallbackThemeName(themeName);

  ui->setupUi(this);

  // Setup Actions
  m_actionAbout->setText(tr("About %1...").arg(kAppName));
  m_actionAbout->setMenuRole(QAction::AboutRole);
  m_actionAbout->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::HelpAbout));

#ifndef Q_OS_WIN
  m_actionQuit->setShortcut(QKeySequence::Quit);
  m_actionTrayQuit->setShortcut(QKeySequence::Quit);
#else
  m_actionQuit->setShortcut(QKeySequence(QStringLiteral("Ctrl+Q")));
  m_actionTrayQuit->setShortcut(QKeySequence(QStringLiteral("Ctrl+Q")));
#endif
  m_actionQuit->setMenuRole(QAction::QuitRole);

  m_actionQuit->setIcon(QIcon(QIcon::fromTheme("application-exit")));
  m_actionTrayQuit->setIcon(QIcon(QIcon::fromTheme("application-exit")));

  m_actionClearSettings->setIcon(QIcon::fromTheme(QStringLiteral("edit-clear-all")));

  m_actionSettings->setMenuRole(QAction::PreferencesRole);
  m_actionSettings->setIcon(QIcon::fromTheme(QStringLiteral("configure")));

  m_actionSave->setShortcut(QKeySequence(tr("Ctrl+Alt+S")));
  m_actionSave->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::DocumentSaveAs));

  m_actionStartCore->setShortcut(QKeySequence(tr("Ctrl+S")));
  m_actionStartCore->setIcon(QIcon::fromTheme(QStringLiteral("system-run")));

  m_actionStopCore->setShortcut(QKeySequence(tr("Ctrl+T")));
  m_actionStopCore->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::ProcessStop));

  m_actionReportBug->setIcon(QIcon(QIcon::fromTheme(QStringLiteral("tools-report-bug"))));

#ifdef Q_OS_MAC
  ui->btnToggleLog->setFixedHeight(ui->lblLog->height() * 0.6);
#endif

  ui->btnToggleLog->setStyleSheet(QStringLiteral("background:rgba(0,0,0,0);"));
  if (m_appConfig.logExpanded())
    ui->btnToggleLog->click();

  toggleLogVisible(m_appConfig.logExpanded());

  // Setup the Instance Checking
  // In case of a previous crash remove first
  m_guiDupeChecker->removeServer(m_guiSocketName);
  m_guiDupeChecker->listen(m_guiSocketName);

  createMenuBar();
  setupControls();
  connectSlots();

  setupTrayIcon();

  m_configScopes.signalReady();

  updateScreenName();
  applyConfig();
  restoreWindow();

  qDebug().noquote() << "active settings path:" << m_configScopes.activeFilePath();

  updateSize();
}

MainWindow::~MainWindow()
{
  m_guiDupeChecker->close();
  m_coreProcess.cleanup();
}

void MainWindow::restoreWindow()
{
  const auto &windowSize = m_appConfig.mainWindowSize();
  if (windowSize.has_value()) {
    qDebug() << "restoring main window size";
    m_expandedSize = windowSize.value();
  }

  const auto &windowPosition = m_appConfig.mainWindowPosition();
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
}

void MainWindow::setupControls()
{
  setWindowTitle(kAppName);

  secureSocket(false);

  ui->lblUpdate->setStyleSheet(kStyleNoticeLabel);
  ui->lblUpdate->hide();

  ui->lblNotice->setStyleSheet(kStyleNoticeLabel);
  ui->lblNotice->hide();

  ui->lblIpAddresses->setText(tr("This computer's IP addresses: %1").arg(getIPAddresses()));

  if (m_appConfig.lastVersion() != kVersion) {
    m_appConfig.setLastVersion(kVersion);
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

  connect(&Logger::instance(), &Logger::newLine, this, &MainWindow::handleLogLine);

  connect(this, &MainWindow::shown, this, &MainWindow::firstShown, Qt::QueuedConnection);

  connect(&m_configScopes, &ConfigScopes::saving, this, &MainWindow::configScopesSaving, Qt::DirectConnection);

  connect(&m_appConfig, &AppConfig::tlsChanged, this, &MainWindow::appConfigTlsChanged);

  connect(&m_appConfig, &AppConfig::screenNameChanged, this, &MainWindow::appConfigScreenNameChanged);

  connect(&m_appConfig, &AppConfig::invertConnectionChanged, this, &MainWindow::appConfigInvertConnection);

  connect(&m_coreProcess, &CoreProcess::starting, this, &MainWindow::coreProcessStarting, Qt::DirectConnection);

  connect(&m_coreProcess, &CoreProcess::error, this, &MainWindow::coreProcessError);

  connect(&m_coreProcess, &CoreProcess::logLine, this, &MainWindow::handleLogLine);

  connect(&m_coreProcess, &CoreProcess::processStateChanged, this, &MainWindow::coreProcessStateChanged);

  connect(&m_coreProcess, &CoreProcess::connectionStateChanged, this, &MainWindow::coreConnectionStateChanged);

  connect(&m_coreProcess, &CoreProcess::secureSocket, this, &MainWindow::coreProcessSecureSocket);

  connect(m_actionAbout, &QAction::triggered, this, &MainWindow::openAboutDialog);
  connect(m_actionClearSettings, &QAction::triggered, this, &MainWindow::clearSettings);
  connect(m_actionReportBug, &QAction::triggered, this, &MainWindow::openHelpUrl);
  connect(m_actionMinimize, &QAction::triggered, this, &MainWindow::hide);

  connect(m_actionQuit, &QAction::triggered, this, &MainWindow::close);
  connect(m_actionTrayQuit, &QAction::triggered, this, &MainWindow::close);
  connect(m_actionRestore, &QAction::triggered, this, &MainWindow::showAndActivate);
  connect(m_actionSave, &QAction::triggered, this, &MainWindow::saveConfig);
  connect(m_actionSettings, &QAction::triggered, this, &MainWindow::openSettings);
  connect(m_actionStartCore, &QAction::triggered, this, &MainWindow::startCore);
  connect(m_actionStopCore, &QAction::triggered, this, &MainWindow::stopCore);
  connect(m_actionTestFatalError, &QAction::triggered, this, &MainWindow::testFatalError);
  connect(m_actionTestCriticalError, &QAction::triggered, this, &MainWindow::testCriticalError);

  connect(&m_versionChecker, &VersionChecker::updateFound, this, &MainWindow::versionCheckerUpdateFound);

// Mac os tray will only show a menu
#ifndef Q_OS_MAC
  connect(m_trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::trayIconActivated);
#endif

  connect(&m_serverConnection, &ServerConnection::configureClient, this, &MainWindow::serverConnectionConfigureClient);

  connect(&m_serverConnection, &ServerConnection::messageShowing, this, &MainWindow::showAndActivate);
  connect(&m_clientConnection, &ClientConnection::messageShowing, this, &MainWindow::showAndActivate);

  connect(ui->btnToggleCore, &QPushButton::clicked, m_actionStartCore, &QAction::trigger, Qt::UniqueConnection);
  connect(ui->btnApplySettings, &QPushButton::clicked, this, &MainWindow::resetCore);
  connect(ui->btnConnect, &QPushButton::clicked, this, &MainWindow::resetCore);
  connect(ui->btnConnectToClient, &QPushButton::clicked, this, &MainWindow::resetCore);

  connect(ui->lineHostname, &QLineEdit::returnPressed, ui->btnConnect, &QPushButton::click);
  connect(ui->lineHostname, &QLineEdit::textChanged, &m_coreProcess, &deskflow::gui::CoreProcess::setAddress);

  connect(ui->lineClientIp, &QLineEdit::returnPressed, ui->btnConnectToClient, &QPushButton::click);
  connect(ui->lineClientIp, &QLineEdit::textChanged, &m_coreProcess, &deskflow::gui::CoreProcess::setAddress);

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
    m_appConfig.setLogExpanded(true);
  } else {
    ui->btnToggleLog->setArrowType(Qt::RightArrow);
    m_expandedSize = size();
    ui->textLog->setVisible(false);
    m_appConfig.setLogExpanded(false);
  }
  // 1 ms delay is to make sure we have left the function before calling updateSize
  QTimer::singleShot(1, this, &MainWindow::updateSize);
}

void MainWindow::firstShown()
{
  // if a critical error was shown just before the main window (i.e. on app
  // load), it will be hidden behind the main window. therefore we need to raise
  // it up in front of the main window.
  // HACK: because the `onShown` event happens just as the window is shown, the
  // message box has a chance of being raised under the main window. to solve
  // this we delay the error dialog raise by a split second. this seems a bit
  // hacky and fragile, so maybe there's a better approach.
  const auto kCriticalDialogDelay = 100;
  QTimer::singleShot(kCriticalDialogDelay, this, &messages::raiseCriticalDialog);
}

void MainWindow::configScopesSaving()
{
  m_serverConfig.commit();
}

void MainWindow::appConfigTlsChanged()
{
  if (m_tlsUtility.isEnabled()) {
    m_tlsUtility.generateCertificate();
  }
}

void MainWindow::trayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
  if (reason != QSystemTrayIcon::Trigger)
    return;
  isVisible() ? hide() : showAndActivate();
}

void MainWindow::versionCheckerUpdateFound(const QString &version)
{
  const auto link = QString(kLinkDownload).arg(kUrlDownload, kColorWhite);
  const auto text = tr("A new version is available (v%1). %2").arg(version, link);

  ui->lblUpdate->show();
  ui->lblUpdate->setText(text);
}

void MainWindow::appConfigScreenNameChanged()
{
  updateScreenName();
}

void MainWindow::appConfigInvertConnection()
{
  applyConfig();
}

void MainWindow::coreProcessError(CoreProcess::Error error)
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
  m_clientConnection.setShowMessage();
  m_coreProcess.start();
}

void MainWindow::stopCore()
{
  qDebug() << "stopping core process";
  m_coreProcess.stop();
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

  m_coreProcess.stop();

  m_saveOnExit = false;
  diagnostic::clearSettings(m_configScopes, true);
}

bool MainWindow::saveConfig()
{
  QString fileName = QFileDialog::getSaveFileName(this, tr("Save configuration as..."));

  if (!fileName.isEmpty() && !m_serverConfig.save(fileName)) {
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
  auto dialog = SettingsDialog(this, m_appConfig, m_serverConfig, m_coreProcess);

  if (dialog.exec() == QDialog::Accepted) {
    m_configScopes.save();

    applyConfig();

    if (m_coreProcess.isStarted()) {
      m_coreProcess.restart();
    }
  }
}

void MainWindow::resetCore()
{
  m_clientConnection.setShowMessage();
  m_coreProcess.restart();
}

void MainWindow::updateSize()
{
  if (ui->textLog->isVisible()) {
    setMaximumHeight(16777215);
    setMaximumWidth(16777215);
    resize(m_expandedSize);
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
  m_configScopes.save();
}

void MainWindow::setModeClient()
{
  enableClient(true);
  enableServer(false);
  m_configScopes.save();
}

void MainWindow::serverConnectionConfigureClient(const QString &clientName)
{
  m_serverConfigDialogState.setVisible(true);
  ServerConfigDialog dialog(this, m_serverConfig, m_appConfig);
  if (dialog.addClient(clientName) && dialog.exec() == QDialog::Accepted) {
    m_coreProcess.restart();
  }
  m_serverConfigDialogState.setVisible(false);
}

//////////////////////////////////////////////////////////////////////////////
// End slots
//////////////////////////////////////////////////////////////////////////////

void MainWindow::open()
{
  if (!m_appConfig.enableUpdateCheck().has_value()) {
    m_appConfig.setEnableUpdateCheck(messages::showUpdateCheckOption(this));
    m_configScopes.save();
  }

  if (m_appConfig.enableUpdateCheck().value()) {
    m_versionChecker.checkLatest();
  } else {
    qDebug() << "update check disabled";
  }

  if (m_appConfig.startedBefore()) {
    m_coreProcess.start();
  }

  if (m_appConfig.autoHide()) {
    hide();
  } else {
    showAndActivate();
  }
}

void MainWindow::coreProcessStarting()
{
  if (deskflow::platform::isWayland()) {
    m_waylandWarnings.showOnce(this, m_coreProcess.mode());
  }
  saveSettings();
}

void MainWindow::setStatus(const QString &status)
{
  ui->lblStatus->setText(status);
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

  auto menuHelp = new QMenu(tr("Help"));
  menuHelp->addAction(m_actionAbout);
  menuHelp->addAction(m_actionReportBug);
  menuHelp->addSeparator();
  menuHelp->addAction(m_actionClearSettings);

  auto menuBar = new QMenuBar(this);
  menuBar->addMenu(menuFile);
  menuBar->addMenu(menuEdit);
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

void MainWindow::setupTrayIcon()
{
  auto trayMenu = new QMenu(this);
  trayMenu->addActions({m_actionStartCore, m_actionStopCore, m_actionMinimize, m_actionRestore, m_actionTrayQuit});
  trayMenu->insertSeparator(m_actionMinimize);
  trayMenu->insertSeparator(m_actionTrayQuit);
  m_trayIcon->setContextMenu(trayMenu);

  setIcon();
  m_trayIcon->show();
}

void MainWindow::applyConfig()
{
  enableServer(m_appConfig.serverGroupChecked());
  enableClient(m_appConfig.clientGroupChecked());

  ui->lineHostname->setText(m_appConfig.serverHostname());
  ui->lineClientIp->setText(m_serverConfig.getClientAddress());
  updateLocalFingerprint();
  setIcon();
}

void MainWindow::saveSettings()
{
  m_appConfig.setServerGroupChecked(ui->rbModeServer->isChecked());
  m_appConfig.setClientGroupChecked(ui->rbModeClient->isChecked());
  m_appConfig.setServerHostname(ui->lineHostname->text());
  m_serverConfig.setClientAddress(ui->lineClientIp->text());

  m_configScopes.save();
}

void MainWindow::setIcon()
{
  // Using a theme icon that is packed in exe renders an invisible icon
  // Instead use the resource path of the packed icon
  // TODO Report to Qt ref the bug here
#ifndef Q_OS_MAC
  QString iconString = QStringLiteral(":/icons/deskflow-%1/apps/64/deskflow").arg(iconMode());
  if (!appConfig().colorfulTrayIcon()) {
    iconString.append(QStringLiteral("-symbolic"));
  }
  m_trayIcon->setIcon(QIcon(iconString));
#else
  if (m_appConfig.colorfulTrayIcon()) {
    m_trayIcon->setIcon(QIcon::fromTheme(QStringLiteral("deskflow")));
  } else {
    m_trayIcon->setIcon(QIcon::fromTheme(QStringLiteral("deskflow-symbolic")));
    m_trayIcon->icon().setIsMask(true);
  }
#endif
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
    m_serverConnection.handleLogLine(line);
    ui->labelServerState->updateServerState(line);
  } else {
    m_clientConnection.handleLogLine(line);
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
    m_coreProcess.stop();

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
      m_coreProcess.start();
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
  if (m_appConfig.closeToTray() && event->spontaneous()) {
    if (m_appConfig.showCloseReminder()) {
      messages::showCloseReminder(this);
      m_appConfig.setShowCloseReminder(false);
    }
    qDebug() << "hiding to tray";
    hide();
    event->ignore();
    return;
  }

  if (m_saveOnExit) {
    m_appConfig.setMainWindowPosition(pos());
    m_appConfig.setMainWindowSize(size());
    m_configScopes.save();
  }
  qDebug() << "quitting application";
  event->accept();
  QApplication::quit();
}

void MainWindow::showFirstConnectedMessage()
{
  if (m_appConfig.startedBefore()) {
    return;
  }

  m_appConfig.setStartedBefore(true);
  m_configScopes.save();

  const auto isServer = m_coreProcess.mode() == CoreMode::Server;
  messages::showFirstConnectedMessage(this, m_appConfig.closeToTray(), m_appConfig.enableService(), isServer);
}

void MainWindow::coreProcessSecureSocket(bool enabled)
{
  secureSocket(enabled);
}

void MainWindow::updateStatus()
{
  const auto connection = m_coreProcess.connectionState();
  const auto process = m_coreProcess.processState();

  ui->lblConnectionSecurityStatus->setVisible(false);
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
      if (m_coreProcess.mode() == CoreMode::Server) {
        setStatus(tr("%1 is waiting for clients").arg(kAppName));
      }

      break;
    }

    case Connecting:
      setStatus(tr("%1 is connecting...").arg(kAppName));
      break;

    case Connected: {
      ui->lblConnectionSecurityStatus->setVisible(true);
      if (m_secureSocket) {
        setStatus(tr("%1 is connected (with %2)").arg(kAppName, m_coreProcess.secureSocketVersion()));
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

void MainWindow::coreProcessStateChanged(CoreProcessState state)
{
  updateStatus();

  if (state == CoreProcessState::Started) {
    qDebug() << "recording that core has started";
    m_appConfig.setStartedBefore(true);
    m_configScopes.save();
  }

  if (state == CoreProcessState::Started || state == CoreProcessState::Starting ||
      state == CoreProcessState::RetryPending) {
    disconnect(ui->btnToggleCore, &QPushButton::clicked, m_actionStartCore, &QAction::trigger);
    connect(ui->btnToggleCore, &QPushButton::clicked, m_actionStopCore, &QAction::trigger, Qt::UniqueConnection);

    ui->btnToggleCore->setText(tr("&Stop"));
    ui->btnToggleCore->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::ProcessStop));

    ui->btnApplySettings->setEnabled(true);

    m_actionStartCore->setEnabled(false);
    m_actionStopCore->setEnabled(true);

  } else {
    disconnect(ui->btnToggleCore, &QPushButton::clicked, m_actionStopCore, &QAction::trigger);
    connect(ui->btnToggleCore, &QPushButton::clicked, m_actionStartCore, &QAction::trigger, Qt::UniqueConnection);

    ui->btnToggleCore->setText(tr("&Start"));
    ui->btnToggleCore->setIcon(QIcon::fromTheme(QStringLiteral("system-run")));

    ui->btnApplySettings->setEnabled(false);

    m_actionStartCore->setEnabled(true);
    m_actionStopCore->setEnabled(false);
  }
}

void MainWindow::coreConnectionStateChanged(CoreConnectionState state)
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

  if (m_appConfig.tlsEnabled() && fingerprintExists) {
    ui->lblMyFingerprint->setVisible(true);
  } else {
    ui->lblMyFingerprint->setVisible(false);
  }
}

void MainWindow::autoAddScreen(const QString name)
{

  int r = m_serverConfig.autoAddScreen(name);
  if (r != kAutoAddScreenOk) {
    switch (r) {
    case kAutoAddScreenManualServer:
      showConfigureServer(tr("Please add the server (%1) to the grid.").arg(m_appConfig.screenName()));
      break;

    case kAutoAddScreenManualClient:
      showConfigureServer(tr("Please drag the new client screen (%1) "
                             "to the desired position on the grid.")
                              .arg(name));
      break;
    }
  }
}

void MainWindow::hide()
{
#ifdef Q_OS_MAC
  macOSNativeHide();
#else
  QMainWindow::hide();
#endif
  m_actionRestore->setVisible(true);
  m_actionMinimize->setVisible(false);
}

void MainWindow::showConfigureServer(const QString &message)
{
  ServerConfigDialog dialog(this, serverConfig(), m_appConfig);
  dialog.message(message);
  if ((dialog.exec() == QDialog::Accepted) && m_coreProcess.isStarted()) {
    m_coreProcess.restart();
  }
}

void MainWindow::secureSocket(bool secureSocket)
{
  m_secureSocket = secureSocket;

  const auto txt = secureSocket ? tr("Secure Connection") : tr("Insecure Connection");
  ui->lblConnectionSecurityStatus->setToolTip(txt);

  const auto icon = QIcon::fromTheme(secureSocket ? QIcon::ThemeIcon::SecurityHigh : QIcon::ThemeIcon::SecurityLow);
  ui->lblConnectionSecurityStatus->setPixmap(icon.pixmap(QSize(32, 32)));
}

void MainWindow::updateScreenName()
{
  ui->lblComputerName->setText(tr("This computer's name: %1 "
                                  R"((<a href="#" style="color: %2">change</a>))")
                                   .arg(m_appConfig.screenName(), kColorSecondary));
  m_serverConfig.updateServerName();
}

void MainWindow::enableServer(bool enable)
{
  QString serverStr = enable ? QStringLiteral("server enabled") : QStringLiteral("server disabled");
  qDebug() << serverStr;
  m_appConfig.setServerGroupChecked(enable);
  ui->rbModeServer->setChecked(enable);
  ui->widgetServer->setEnabled(enable);
  ui->widgetServerInput->setVisible(m_appConfig.invertConnection());

  if (enable) {
    ui->btnToggleCore->setEnabled(true);
    m_actionStartCore->setEnabled(true);

    if (m_coreProcess.isStarted() && m_coreProcess.mode() != CoreProcess::Mode::Server)
      m_coreProcess.stop();

    m_coreProcess.setMode(CoreProcess::Mode::Server);

    // The server can run without any clients configured, and this is actually
    // what you'll want to do the first time since you'll be prompted when an
    // unrecognized client tries to connect.
    if (!m_appConfig.startedBefore() && !m_coreProcess.isStarted()) {
      qDebug() << "auto-starting core server for first time";
      m_coreProcess.start();
      messages::showFirstServerStartMessage(this);
    }
  }
}

void MainWindow::enableClient(bool enable)
{
  QString clientStr = enable ? QStringLiteral("client enabled") : QStringLiteral("client disabled");
  qDebug() << clientStr;
  m_appConfig.setClientGroupChecked(enable);
  ui->rbModeClient->setChecked(enable);
  ui->widgetClientInput->setEnabled(enable);
  ui->widgetClientInput->setVisible(!m_appConfig.invertConnection());

  if (enable) {
    ui->btnToggleCore->setEnabled(true);
    m_actionStartCore->setEnabled(true);
    if (m_coreProcess.isStarted() && m_coreProcess.mode() != CoreProcess::Mode::Client)
      m_coreProcess.stop();
    m_coreProcess.setMode(CoreProcess::Mode::Client);
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
  m_actionRestore->setVisible(false);
  m_actionMinimize->setVisible(true);
}
