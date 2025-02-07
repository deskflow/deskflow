/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 - 2025 Chris Rizzitello <sithord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2012 - 2024 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2008 Volker Lanz <vl@fidra.de>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "MainWindow.h"
#include "ui_MainWindow.h"

#include "dialogs/AboutDialog.h"
#include "dialogs/FingerprintDialog.h"
#include "dialogs/ServerConfigDialog.h"
#include "dialogs/SettingsDialog.h"

#include "base/String.h"
#include "common/constants.h"
#include "gui/Logger.h"
#include "gui/config/ConfigScopes.h"
#include "gui/constants.h"
#include "gui/core/CoreProcess.h"
#include "gui/diagnostic.h"
#include "gui/ipc/DaemonIpcClient.h"
#include "gui/messages.h"
#include "gui/string_utils.h"
#include "gui/style_utils.h"
#include "gui/styles.h"
#include "net/FingerprintDatabase.h"
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
#include <QRegularExpressionValidator>
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
      m_daemonIpcClient{new ipc::DaemonIpcClient(this)},
      m_lblSecurityStatus{new QLabel(this)},
      m_lblStatus{new QLabel(this)},
      m_btnFingerprint{new QToolButton(this)},
      m_btnUpdate{new QPushButton(this)},
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

  // Force generation of SHA256 for the localhost
  if (m_appConfig.tlsEnabled()) {
    if (!QFile::exists(localFingerprintDb())) {
      regenerateLocalFingerprints();
      return;
    }

    deskflow::FingerprintDatabase db;
    db.read(localFingerprintDb().toStdString());
    if (db.fingerprints().size() != kTlsDbSize) {
      regenerateLocalFingerprints();
    }
  }
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

  ui->lblIpAddresses->setText(tr("This computer's IP addresses: %1").arg(getIPAddresses()));

  if (m_appConfig.lastVersion() != kVersion) {
    m_appConfig.setLastVersion(kVersion);
  }

  // Setup the log toggle, set its initial state to closed
  ui->btnToggleLog->setStyleSheet(kStyleFlatButton);
  if (m_appConfig.logExpanded()) {
    ui->btnToggleLog->setArrowType(Qt::DownArrow);
    ui->textLog->setVisible(true);
    ui->btnToggleLog->click();
  } else {
    ui->textLog->setVisible(false);
  }

  ui->serverOptions->setVisible(false);
  ui->clientOptions->setVisible(false);
  ui->rbModeClient->setChecked(m_appConfig.clientGroupChecked());
  ui->rbModeServer->setChecked(m_appConfig.serverGroupChecked());

  if (m_appConfig.clientGroupChecked() || m_appConfig.serverGroupChecked())
    updateModeControls(m_appConfig.serverGroupChecked());

  ui->lineEditName->setValidator(new QRegularExpressionValidator(m_nameRegEx, this));
  ui->lineEditName->setVisible(false);

#if defined(Q_OS_MAC)

  ui->rbModeServer->setAttribute(Qt::WA_MacShowFocusRect, 0);
  ui->rbModeClient->setAttribute(Qt::WA_MacShowFocusRect, 0);

#endif

  const auto trayItemSize = QSize(24, 24);
  m_btnFingerprint->setStyleSheet(kStyleFlatButtonHoverable);
  m_btnFingerprint->setIcon(QIcon::fromTheme(QStringLiteral("fingerprint")));
  m_btnFingerprint->setFixedSize(trayItemSize);
  m_btnFingerprint->setIconSize(trayItemSize);
  m_btnFingerprint->setAutoRaise(true);
  m_btnFingerprint->setToolTip(tr("View local fingerprint"));
  ui->statusBar->insertPermanentWidget(0, m_btnFingerprint);

  m_lblSecurityStatus->setVisible(false);
  m_lblSecurityStatus->setFixedSize(trayItemSize);
  m_lblSecurityStatus->setScaledContents(true);
  ui->statusBar->insertPermanentWidget(1, m_lblSecurityStatus);

  ui->statusBar->insertPermanentWidget(2, m_lblStatus, 1);

  m_btnUpdate->setVisible(false);
  m_btnUpdate->setStyleSheet(kStyleFlatButtonHoverable);
  m_btnUpdate->setFlat(true);
  m_btnUpdate->setText(tr("Update available"));
  m_btnUpdate->setLayoutDirection(Qt::RightToLeft);
  m_btnUpdate->setIcon(QIcon::fromTheme(QStringLiteral("software-updates-release")));
  m_btnUpdate->setFixedHeight(24);
  m_btnUpdate->setIconSize(trayItemSize);
  ui->statusBar->insertPermanentWidget(3, m_btnUpdate);
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
  connect(&m_appConfig, &AppConfig::screenNameChanged, this, &MainWindow::updateScreenName);
  connect(&m_appConfig, &AppConfig::logLevelChanged, &m_coreProcess, &CoreProcess::applyLogLevel);

  connect(&m_coreProcess, &CoreProcess::starting, this, &MainWindow::coreProcessStarting, Qt::DirectConnection);
  connect(&m_coreProcess, &CoreProcess::error, this, &MainWindow::coreProcessError);
  connect(&m_coreProcess, &CoreProcess::logLine, this, &MainWindow::handleLogLine);
  connect(&m_coreProcess, &CoreProcess::processStateChanged, this, &MainWindow::coreProcessStateChanged);
  connect(&m_coreProcess, &CoreProcess::connectionStateChanged, this, &MainWindow::coreConnectionStateChanged);
  connect(&m_coreProcess, &CoreProcess::secureSocket, this, &MainWindow::secureSocket);

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

  connect(ui->lineHostname, &QLineEdit::returnPressed, ui->btnConnect, &QPushButton::click);
  connect(ui->lineHostname, &QLineEdit::textChanged, &m_coreProcess, &deskflow::gui::CoreProcess::setAddress);

  connect(ui->btnConfigureServer, &QPushButton::clicked, this, [this] { showConfigureServer(""); });
  connect(ui->lblComputerName, &QLabel::linkActivated, this, &MainWindow::openSettings);
  connect(m_btnFingerprint, &QToolButton::clicked, this, &MainWindow::showMyFingerprint);

  connect(ui->rbModeServer, &QRadioButton::toggled, this, &MainWindow::coreModeToggled);
  connect(ui->rbModeClient, &QRadioButton::toggled, this, &MainWindow::coreModeToggled);

  connect(ui->btnToggleLog, &QAbstractButton::toggled, this, &MainWindow::toggleLogVisible);

  connect(m_btnUpdate, &QPushButton::clicked, this, &MainWindow::openGetNewVersionUrl);

  connect(m_guiDupeChecker, &QLocalServer::newConnection, this, &MainWindow::showAndActivate);

  connect(ui->btnEditName, &QPushButton::clicked, this, &MainWindow::showHostNameEditor);

  connect(ui->lineEditName, &QLineEdit::editingFinished, this, &MainWindow::setHostName);
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
  if (m_tlsUtility.isEnabled() && !QFile::exists(m_appConfig.tlsCertPath())) {
    m_tlsUtility.generateCertificate();
  }
  updateSecurityIcon(m_lblSecurityStatus->isVisible());
}

void MainWindow::trayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
  if (reason != QSystemTrayIcon::Trigger)
    return;
  isVisible() ? hide() : showAndActivate();
}

void MainWindow::versionCheckerUpdateFound(const QString &version)
{
  m_btnUpdate->setVisible(true);
  m_btnUpdate->setToolTip(tr("A new version v%1 is avilable").arg(version));
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

void MainWindow::openGetNewVersionUrl() const
{
  QDesktopServices::openUrl(kUrlDownload);
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
  if (m_appConfig.logExpanded()) {
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
  if (!QFile::exists(localFingerprintDb())) {
    if (regenerateLocalFingerprints())
      showMyFingerprint();
    return;
  }

  deskflow::FingerprintDatabase db;
  db.read(localFingerprintDb().toStdString());
  if (db.fingerprints().size() != kTlsDbSize) {
    if (regenerateLocalFingerprints())
      showMyFingerprint();
    return;
  }

  QList<deskflow::FingerprintData> fingerprints;
  fingerprints.reserve(db.fingerprints().size());
  std::copy(db.fingerprints().begin(), db.fingerprints().end(), std::back_inserter(fingerprints));

  FingerprintDialog fingerprintDialog(this, fingerprints);
  fingerprintDialog.exec();
}

void MainWindow::coreModeToggled()
{
  auto serverMode = ui->rbModeServer->isChecked();

  const auto mode = serverMode ? QStringLiteral("server enabled") : QStringLiteral("client enabled");
  qDebug() << mode;

  m_appConfig.setServerGroupChecked(serverMode);
  m_appConfig.setClientGroupChecked(!serverMode);
  m_configScopes.save();

  updateModeControls(serverMode);
}

void MainWindow::updateModeControls(bool serverMode)
{
  ui->serverOptions->setVisible(serverMode);
  ui->clientOptions->setVisible(!serverMode);
  ui->lblNoMode->setVisible(false);
  ui->btnToggleCore->setEnabled(true);
  m_actionStartCore->setEnabled(true);
  auto expectedCoreMode = serverMode ? CoreProcess::Mode::Server : CoreProcess::Mode::Client;
  if (m_coreProcess.isStarted() && m_coreProcess.mode() != expectedCoreMode)
    m_coreProcess.stop();
  m_coreProcess.setMode(expectedCoreMode);
  if (serverMode) {
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

void MainWindow::updateSecurityIcon(bool visible)
{
  m_lblSecurityStatus->setVisible(visible);
  if (!visible)
    return;

  bool secureSocket = m_appConfig.tlsEnabled();

  const auto txt =
      secureSocket ? tr("%1 Encryption Enabled").arg(m_coreProcess.secureSocketVersion()) : tr("Encryption Disabled");
  m_lblSecurityStatus->setToolTip(txt);

  const auto icon = QIcon::fromTheme(secureSocket ? QIcon::ThemeIcon::SecurityHigh : QIcon::ThemeIcon::SecurityLow);
  m_lblSecurityStatus->setPixmap(icon.pixmap(QSize(32, 32)));
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
    showAndActivate();
    m_appConfig.setEnableUpdateCheck(messages::showUpdateCheckOption(this));
    m_configScopes.save();
  }

  if (m_appConfig.enableUpdateCheck().value()) {
    m_versionChecker.checkLatest();
  } else {
    qDebug() << "update check disabled";
  }

  m_coreProcess.applyLogLevel();

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
  m_lblStatus->setText(status);
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
  ui->lineHostname->setText(m_appConfig.serverHostname());
  updateLocalFingerprint();
  setIcon();

  if (!m_appConfig.serverGroupChecked() && !m_appConfig.clientGroupChecked())
    return;
  updateModeControls(m_appConfig.serverGroupChecked());
}

void MainWindow::saveSettings()
{
  m_appConfig.setServerGroupChecked(ui->rbModeServer->isChecked());
  m_appConfig.setClientGroupChecked(ui->rbModeClient->isChecked());
  m_appConfig.setServerHostname(ui->lineHostname->text());

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
  } else {
    m_clientConnection.handleLogLine(line);
  }
}

void MainWindow::checkFingerprint(const QString &line)
{
  static const QRegularExpression re(R"(.*peer fingerprint: \(SHA1\) ([A-F0-9:]+) \(SHA256\) ([A-F0-9:]+))");
  auto match = re.match(line);
  if (!match.hasMatch()) {
    return;
  }

  const deskflow::FingerprintData sha1 = {
      deskflow::fingerprintTypeToString(deskflow::FingerprintType::SHA1),
      deskflow::string::fromHex(match.captured(1).toStdString())
  };

  const auto sha256Text = match.captured(2);

  const deskflow::FingerprintData sha256 = {
      deskflow::fingerprintTypeToString(deskflow::FingerprintType::SHA256),
      deskflow::string::fromHex(sha256Text.toStdString())
  };

  const bool isClient = m_coreProcess.mode() == CoreMode::Client;

  if ((isClient && m_checkedServers.contains(sha256Text)) || (!isClient && m_checkedClients.contains(sha256Text))) {
    qDebug("ignoring fingerprint, already handled");
    return;
  }

  if (isClient) {
    m_checkedServers.append(sha256Text);
  } else {
    m_checkedClients.append(sha256Text);
  }

  deskflow::FingerprintDatabase db;
  db.read(trustedFingerprintDb().toStdString());

  if (db.isTrusted(sha256)) {
    qDebug("fingerprint is trusted");
    return;
  }

  auto dialogMode = isClient ? FingerprintDialogMode::Client : FingerprintDialogMode::Server;

  FingerprintDialog fingerprintDialog(this, {sha1, sha256}, dialogMode);
  connect(&fingerprintDialog, &FingerprintDialog::requestLocalPrintsDialog, this, &MainWindow::showMyFingerprint);

  if (fingerprintDialog.exec() == QDialog::Accepted) {
    db.addTrusted(sha256);
    db.write(trustedFingerprintDb().toStdString());
    if (isClient) {
      m_checkedServers.removeAll(sha256Text);
    } else {
      m_checkedClients.removeAll(sha256Text);
    }
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

void MainWindow::updateStatus()
{
  const auto connection = m_coreProcess.connectionState();
  const auto process = m_coreProcess.processState();

  updateSecurityIcon(false);
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
        updateSecurityIcon(true);
        setStatus(tr("%1 is waiting for clients").arg(kAppName));
      }

      break;
    }

    case Connecting:
      setStatus(tr("%1 is connecting...").arg(kAppName));
      break;

    case Connected: {
      updateSecurityIcon(true);
      setStatus(tr("%1 is connected").arg(kAppName));
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
  m_btnFingerprint->setVisible(m_appConfig.tlsEnabled() && QFile::exists(localFingerprintDb()));
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
  updateSecurityIcon(m_lblSecurityStatus->isVisible());
}

void MainWindow::updateScreenName()
{
  ui->lblComputerName->setText(m_appConfig.screenName());
  ui->lineEditName->setText(m_appConfig.screenName());
  m_serverConfig.updateServerName();
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

void MainWindow::showHostNameEditor()
{
  ui->lineEditName->show();
  ui->lblComputerName->hide();
  ui->btnEditName->hide();
  ui->lineEditName->setFocus();
}

void MainWindow::setHostName()
{
  ui->lineEditName->hide();
  ui->lblComputerName->show();
  ui->btnEditName->show();

  QString text = ui->lineEditName->text();
  bool existingScreen = serverConfig().screenExists(text) && text != m_appConfig.screenName();

  if (!ui->lineEditName->hasAcceptableInput() || text.isEmpty() || existingScreen) {
    blockSignals(true);
    ui->lineEditName->setText(m_appConfig.screenName());
    blockSignals(false);

    const auto title = tr("Invalid Screen Name");
    QString body;
    if (existingScreen) {
      body = tr("Screen name already exists");
    } else {
      body = tr("The name you have chosen is invalid.\n\n"
                "Valid names:\n"
                "• Use letters and numbers\n"
                "• May also use _ or -\n"
                "• Are between 1 and 255 characters");
    }
    QMessageBox::information(this, title, body);
    return;
  }

  ui->lblComputerName->setText(ui->lineEditName->text());
  m_appConfig.setScreenName(ui->lineEditName->text());
  applyConfig();
}

QString MainWindow::getTlsPath()
{
  CoreTool coreTool;
  return QStringLiteral("%1/%2").arg(coreTool.getProfileDir(), kSslDir);
}

QString MainWindow::localFingerprintDb()
{
  return QStringLiteral("%1/%2").arg(getTlsPath(), kFingerprintLocalFilename);
}

QString MainWindow::trustedFingerprintDb()
{
  const bool isClient = m_coreProcess.mode() == CoreMode::Client;
  const auto trustFile = isClient ? kFingerprintTrustedServersFilename : kFingerprintTrustedClientsFilename;
  return QStringLiteral("%1/%2").arg(getTlsPath(), trustFile);
}

bool MainWindow::regenerateLocalFingerprints()
{
  if (!QFile::exists(m_appConfig.tlsCertPath()) && !m_tlsUtility.generateCertificate()) {
    return false;
  }

  TlsCertificate tls;
  if (!tls.generateFingerprint(m_appConfig.tlsCertPath())) {
    return false;
  }

  updateLocalFingerprint();
  return true;
}
