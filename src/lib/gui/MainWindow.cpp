#include "gui/widgets/OptionControl.h"
/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2024 - 2026 Chris Rizzitello <sithord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2012 - 2024 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2008 Volker Lanz <vl@fidra.de>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "MainWindow.h"
#include "gui/widgets/ActionButton.h"
#include "ui_MainWindow.h"
#include "widgets/ConnectionHome.h"
#include <QComboBox>
#include <QScrollArea>
#include <QScrollBar>
#include <QStackedWidget>
#include <QTabWidget>

#include "Diagnostic.h"
#include "StyleUtils.h"

#include "ExperienceStyle.h"
#include "common/CandidatePolicy.h"
#include "common/LabExecutionPolicy.h"
#include "core/DeviceRecovery.h"
#include "core/DiscoveryScope.h"
#include "core/NearbyDiscovery.h"
#include "core/NearbyPublisher.h"
#include "core/RecoveryProbe.h"
#include "dialogs/AboutDialog.h"
#include "dialogs/ClientConfigDialog.h"
#include "dialogs/DesktopLayoutEditor.h"
#include "dialogs/DiagnosticExportDialog.h"
#include "dialogs/DisplayTopologyDialog.h"
#include "dialogs/FingerprintDialog.h"
#include "dialogs/HelpDialog.h"
#include "dialogs/NearbyDevicesDialog.h"
#include "dialogs/PairingReviewDialog.h"
#include "dialogs/ServerConfigDialog.h"
#include "dialogs/SettingsDialog.h"
#include "dialogs/TrustedDevicesDialog.h"
#include "gui/IdentityBindings.h"
#include <QComboBox>
#include <QNetworkInterface>
#include <QSignalBlocker>

#include "common/DisplayTopology.h"
#include "common/PlatformInfo.h"
#include "common/Settings.h"
#include "common/UrlConstants.h"
#include "common/VersionInfo.h"
#include "core/AutomaticRecovery.h"
#include "core/DiscoveryTransport.h"
#include "core/PowerMonitor.h"
#include "dialogs/AutomaticRecoveryDialog.h"
#include "gui/Messages.h"
#include "gui/TlsUtility.h"
#include "gui/core/CoreProcess.h"
#include "gui/ipc/DaemonIpcClient.h"
#include "gui/widgets/LogDock.h"
#include "net/FingerprintDatabase.h"
#include "widgets/ConnectionDiagnostics.h"
#include "widgets/RecentServersButton.h"
#include "widgets/StatusBar.h"

#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
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
#include <QScreen>
#include <QScrollBar>
#include <QSignalBlocker>
#include <QToolButton>

#include <memory>

#if defined(Q_OS_MACOS)
#include <ApplicationServices/ApplicationServices.h>
#endif

using namespace deskflow::gui;

MainWindow::MainWindow(StartupMode startupMode)
    : m_startupMode(startupMode),
      ui{std::make_unique<Ui::MainWindow>()},
      m_coreProcess(m_serverConfig),
      m_trayIcon{new QSystemTrayIcon(this)},
      m_guiDupeChecker{new QLocalServer(this)},
      m_daemonIpcClient{new ipc::DaemonIpcClient(this)},
      m_logDock{new LogDock(this)},
      m_statusBar{new StatusBar(this)},
      m_recentServers{new RecentServersButton(this)},
      m_connectionDiagnostics{new ConnectionDiagnostics(this)},
      m_menuFile{new QMenu(this)},
      m_menuEdit{new QMenu(this)},
      m_menuView{new QMenu(this)},
      m_menuHelp{new QMenu(this)},
      m_actionAbout{new QAction(this)},
      m_actionMinimize{new QAction(this)},
      m_actionQuit{new QAction(this)},
      m_actionTrayQuit{new QAction(this)},
      m_actionRestore{new QAction(this)},
      m_actionSettings{new QAction(this)},
      m_actionStartCore{new QAction(this)},
      m_actionRestartCore{new QAction(this)},
      m_actionStopCore{new QAction(this)},
      m_actionShowHelp{new QAction(this)},
      m_networkMonitor{new NetworkMonitor(this)}
{
  ui->setupUi(this);
  deskflow::gui::applyExperienceStyle(this);
  setMinimumSize(620, 480);
  // The original fixed-height central widget clipped dynamic status/details.
  // Keep every control in one real, keyboard-scrollable content area.
  auto *content = takeCentralWidget();
  auto *scroll = new QScrollArea(this);
  scroll->setObjectName("mainContentScroll");
  scroll->setFrameShape(QFrame::NoFrame);
  scroll->setWidgetResizable(true);
  scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  content->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
  content->layout()->setAlignment(Qt::AlignTop);
  content->layout()->setContentsMargins(16, 14, 16, 14);
  content->layout()->setSpacing(12);
  content->layout()->setSizeConstraint(QLayout::SetMinimumSize);
  scroll->setWidget(content);
  // Device selection remains visible even while advanced settings scroll.
  // These are the existing actions, not a second connection implementation.
  auto *central = new QWidget(this);
  auto *centralLayout = new QVBoxLayout(central);
  centralLayout->setContentsMargins(0, 0, 0, 0);
  centralLayout->setSpacing(0);
  m_deviceActions = new QWidget(central);
  m_deviceActions->setObjectName("deviceActions");
  m_deviceActions->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
  auto *deviceRow = new QHBoxLayout(m_deviceActions);
  deviceRow->setContentsMargins(16, 12, 16, 12);
  deviceRow->setSpacing(10);
  centralLayout->addWidget(m_deviceActions);
  centralLayout->addWidget(scroll, 1);
  setCentralWidget(central);
  ui->widget->layout()->setContentsMargins(0, 0, 0, 0);
  ui->widgetModeSelection->layout()->setContentsMargins(0, 0, 0, 0);
  ui->widgetModeOptions->layout()->setContentsMargins(0, 0, 0, 0);
  ui->widgetModeOptions->layout()->setSpacing(12);
  ui->groupBox->layout()->setContentsMargins(14, 12, 14, 12);
  ui->groupBox->layout()->setSpacing(12);
  // Two compact rows keep both Chinese and English usable at 640 logical pixels.
  auto *oldClientLayout = ui->clientOptions->layout();
  while (auto *item = oldClientLayout->takeAt(0))
    delete item;
  delete oldClientLayout;
  auto *clientLayout = new QVBoxLayout(ui->clientOptions);
  clientLayout->setContentsMargins(0, 0, 0, 0);
  clientLayout->setSpacing(10);
  auto *addressRow = new QHBoxLayout;
  addressRow->addWidget(ui->m_pLabelServerName);
  addressRow->addWidget(ui->cbHostname, 1);
  clientLayout->addLayout(addressRow);
  auto *clientActions = new QHBoxLayout;
  clientActions->setObjectName("clientActions");
  clientActions->addWidget(ui->btnConfigureClient);
  clientActions->addStretch();
  clientLayout->addLayout(clientActions);
  auto *oldServerLayout = ui->serverOptions->layout();
  auto *serverButtons = new QHBoxLayout;
  while (auto *item = oldServerLayout->takeAt(0))
    serverButtons->addItem(item);
  delete oldServerLayout;
  auto *serverLayout = new QVBoxLayout(ui->serverOptions);
  serverLayout->setContentsMargins(0, 0, 0, 0);
  serverLayout->setSpacing(10);
  serverLayout->addLayout(serverButtons);

  setWindowIcon(QIcon::fromTheme("org.deskflow.deskflow"));

  addDockWidget(Qt::BottomDockWidgetArea, m_logDock);

  // Setup Actions
  m_actionAbout->setMenuRole(QAction::AboutRole);
  m_actionAbout->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::HelpAbout));

  m_actionMinimize->setIcon(QIcon::fromTheme(QStringLiteral("window-minimize-pip")));
  m_actionRestore->setIcon(QIcon::fromTheme(QStringLiteral("window-restore-pip")));

  if (!deskflow::platform::isWindows()) {
    m_actionQuit->setShortcut(QKeySequence::Quit);
    m_actionTrayQuit->setShortcut(QKeySequence::Quit);
  }

  m_actionQuit->setIcon(QIcon::fromTheme("application-exit"));
  m_actionQuit->setMenuRole(QAction::QuitRole);

  m_actionTrayQuit->setIcon(QIcon::fromTheme("application-exit"));
  m_actionTrayQuit->setMenuRole(QAction::NoRole);

  m_actionSettings->setIcon(QIcon::fromTheme(QStringLiteral("configure")));
  m_actionSettings->setMenuRole(QAction::PreferencesRole);
  m_actionSettings->setShortcut(QKeySequence::Preferences);

  m_actionStartCore->setIcon(QIcon::fromTheme(QStringLiteral("system-run")));
  m_actionStartCore->setMenuRole(QAction::NoRole);

  m_actionRestartCore->setVisible(false);
  m_actionRestartCore->setIcon(QIcon::fromTheme(QStringLiteral("view-refresh")));
  m_actionRestartCore->setMenuRole(QAction::NoRole);

  m_actionStopCore->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::ProcessStop));
  m_actionStopCore->setMenuRole(QAction::NoRole);

  m_actionShowHelp->setIcon(QIcon::fromTheme(QStringLiteral("question")));
  m_actionShowHelp->setMenuRole(QAction::NoRole);
  m_actionShowHelp->setShortcut(QKeySequence::HelpContents);
#ifdef DESKFLOW_UI_REVIEW_BUILD
  // Links, exports and system actions are outside this offline window review.
  m_actionAbout->setEnabled(false);
  m_actionShowHelp->setEnabled(false);
  ui->btnSaveServerConfig->setEnabled(false);
#endif

  // Setup the Instance Checking
  // In case of a previous crash remove first
#ifndef DESKFLOW_LAB_BUILD
  QLocalServer::removeServer(m_guiSocketName);
  m_guiDupeChecker->listen(m_guiSocketName);
#endif

  createMenuBar();
  setupControls();
  updateText();
  connectSlots();
#ifndef DESKFLOW_LAB_BUILD
  setupTrayIcon();
#endif
  updateComputerName();
  setHelpFilePath();

  qDebug().noquote() << "active settings path:" << Settings::settingsPath();

  // Force generation of a certificate on the host
#ifndef DESKFLOW_LAB_BUILD
  if (TlsUtility::isEnabled()) {
#ifdef DESKFLOW_CANDIDATE_BUILD
    const bool safe = deskflow::candidate::securityProfile(
        Settings::settingsFile(), Settings::value(Settings::Security::Certificate).toString(), true,
        Settings::value(Settings::Security::CheckPeers).toBool()
    );
    if (safe && TlsUtility::isCertValid())
      m_fingerprint = {QCryptographicHash::Sha256, TlsUtility::certFingerprint()};
    // Candidate setup is user-driven from the home view. Merely opening the
    // app must not create credentials or present a security setup dialog.
#else
    if (Settings::value(Settings::Security::KeySize).toInt() < 2048) {
      QMessageBox::information(
          this, kAppName,
          tr("Your current TLS key is smaller than the minimum allowed size, A new key 2048-bit key will be generated.")
      );
      Settings::setValue(Settings::Security::KeySize, 2048);
    }
    if (!TlsUtility::isCertValid()) {
      generateCertificate();
    } else {
      m_fingerprint = {QCryptographicHash::Sha256, TlsUtility::certFingerprint()};
    }
#endif
  }
#endif

  applyConfig();
  m_connectionOptions->setChecked(false);
  m_statusBar->setSecurityIcon(TlsUtility::isEnabled());
  setupConnectionHome();
  restoreWindow();

#ifdef Q_OS_MACOS
  // Route native quits (Cmd+Q / Apple menu Quit / Dock "Quit") to the usual close-to-tray decision instead.
#ifndef DESKFLOW_LAB_BUILD
  installQuitHandler([this] { return !maybeHideToTray(); });
#endif
#endif
}
MainWindow::~MainWindow()
{
  if (m_automaticRecovery)
    m_automaticRecovery->cancel();
  // Stop network monitoring
  if (m_networkMonitor) {
    m_networkMonitor->stopMonitoring();
  }

  m_guiDupeChecker->close();
  m_coreProcess.cleanup();
}

void MainWindow::resizeToContent()
{
  if (m_homePages) {
    setMaximumHeight(QWIDGETSIZE_MAX);
    return; // Resizing a utility window is the user's choice, not a status side effect.
  }
  auto *scroll = findChild<QScrollArea *>("mainContentScroll");
  if (!scroll)
    return;
  layout()->activate();
  ui->topLevelWidget->layout()->activate();
  const int width = scroll->viewport()->width();
  int height = ui->topLevelWidget->layout()->totalHeightForWidth(width);
  if (height < 0)
    height = ui->topLevelWidget->sizeHint().height();
  height = qMax(height, ui->topLevelWidget->minimumHeight());
  // Include the real menu/dock/status/scroll-frame geometry, not just hints.
  const int chrome = qMax(
                         menuBar()->sizeHint().height() + m_statusBar->sizeHint().height(),
                         this->height() - scroll->viewport()->height()
                     ) +
                     2;
  int available = screen()->availableGeometry().height() - 48;
#ifdef DESKFLOW_LAB_BUILD
  // Offscreen screenshots use a documented desktop viewport, even at 2x DPR.
  available = 960;
#endif
  if (!Settings::value(Settings::Gui::LogExpanded).toBool()) {
    // A utility window follows its content height. Expanding the width must
    // not leave an empty lower half; smaller windows retain vertical scrolling.
    setMaximumHeight(qBound(minimumHeight(), height + chrome, qMax(minimumHeight(), available)));
    resize(this->width(), maximumHeight());
    // Style polish and height-for-width layout requests can settle one turn later.
    // One bounded correction removes needless scrolling in the natural window;
    // deliberate smaller windows still scroll normally after the resize finishes.
    QTimer::singleShot(0, this, [this, scroll, available] {
      const int missing = scroll->verticalScrollBar()->maximum();
      if (missing > 0 && this->height() < available) {
        setMaximumHeight(qMin(available, this->height() + missing + 2));
        resize(this->width(), maximumHeight());
      }
    });
  } else
    setMaximumHeight(QWIDGETSIZE_MAX);
}

void MainWindow::restoreWindow()
{
  const auto saved = Settings::value(Settings::Gui::WindowGeometry).toRect();
  if (saved.isValid())
    setGeometry(saved);
  else
    resize(760, 560);
  m_expandedSize = size();
  QTimer::singleShot(0, this, &MainWindow::resizeToContent);
  if (!QGuiApplication::primaryScreen()->availableGeometry().intersects(geometry()))
    move(QGuiApplication::primaryScreen()->availableGeometry().center() - rect().center());
}

deskflow::gui::NearbyDevicesDialog *MainWindow::createRecoveryDialog(
    const QString &fp, deskflow::gui::DiscoveryScopeSource source, deskflow::gui::ScopedAddressResolver *resolver,
    deskflow::gui::TlsProbeExecutor *executor
)
{
  auto *nearby = new NearbyDevicesDialog(this, source, &m_identityBindings, fp);
  nearby->setAttribute(Qt::WA_DeleteOnClose);
  if (auto *recovery = nearby->recovery()) {
    auto *discovery = nearby->findChild<NearbyDiscovery *>();
    if (!resolver)
      resolver = deskflow::gui::createScopedAddressResolver(nearby, source, [discovery](const QString &h, int p) {
        return discovery->serviceNames(h, p);
      });
    else
      resolver->setParent(nearby);
    if (!executor)
      executor = new deskflow::gui::ProcessTlsProbe(nearby, source);
    else
      executor->setParent(nearby);
#ifdef DESKFLOW_CANDIDATE_BUILD
    executor->selectCandidateProfile(
        Settings::settingsFile(), Settings::value(Settings::Security::Certificate).toString(), fp
    );
#endif
    new deskflow::gui::RecoveryProbeBroker(*recovery, *resolver, *executor, source, nearby);
    connect(nearby, &NearbyDevicesDialog::preparedSessionRequested, this, [this, nearby, executor] {
      if (!deskflow::lab::kProbeNetworkExecutionEnabled)
        return;
      const auto root = QFileDialog::getExistingDirectory(nearby, tr("Choose an explicitly prepared test session"));
      if (!root.isEmpty() && !executor->selectPreparedSession(root))
        nearby->findChild<QLabel *>("nearbyStatus")
            ->setText(tr("Test credentials were rejected. The saved address and trust are unchanged."));
    });
    if (!deskflow::lab::kProbeNetworkExecutionEnabled)
      connect(recovery, &deskflow::gui::DeviceRecovery::stateChanged, nearby, [this, nearby](auto state) {
        if (state == deskflow::gui::DeviceRecovery::State::NeedsAction)
          nearby->findChild<QLabel *>("nearbyStatus")
              ->setText(
                  tr("Real network and credential use are disabled in this build. The saved address and trust are "
                     "unchanged.")
              );
      });
  }
  connect(
      nearby, &NearbyDevicesDialog::verifiedEndpoint, this,
      [this](const QString &identity, const QString &host, int port) {
        Settings::setValue("gui/identityBindings", m_identityBindings.encode());
        if (m_coreProcess.setExpectedPeerFingerprint(identity))
          quickReconnect(host, port);
#ifdef DESKFLOW_LAB_BUILD
        if (auto *banner = findChild<QLabel *>("labPreviewBanner"))
          banner->setText(
              tr("Simulated identity matched · saved address updated to %1 · input control remains disabled").arg(host)
          );
        QTimer::singleShot(0, this, &MainWindow::resizeToContent);
#endif
      }
  );
  return nearby;
}

void MainWindow::setupAutomaticRecovery(
    DiscoveryScopeSource source, ScopedAddressResolver *resolver, TlsProbeExecutor *executor,
    NearbyDiscovery *discovery, std::function<qint64()> clock
)
{
  if (m_automaticRecovery) {
    m_automaticRecovery->cancel();
    delete m_automaticRecovery;
    m_automaticRecovery = nullptr;
  }
  if (!discovery)
    discovery = new NearbyDiscovery;
  if (!resolver)
    resolver = createScopedAddressResolver(nullptr, source, [discovery](const QString &h, int p) {
      return discovery->serviceNames(h, p);
    });
  if (!executor)
    executor = new ProcessTlsProbe(
        nullptr, source,
        Settings::value("gui/runtimeProfile").toByteArray().isEmpty()
            ? deskflow::lab::RuntimeProfile{}
            : deskflow::lab::RuntimeProfile::decode(Settings::value("gui/runtimeProfile").toByteArray())
    );
#ifdef DESKFLOW_CANDIDATE_BUILD
  const auto selected = AutomaticRecoveryPolicy::decode(Settings::value("gui/automaticRecovery").toByteArray());
  if (selected.enabled &&
      !executor->selectCandidateProfile(
          Settings::settingsFile(), Settings::value(Settings::Security::Certificate).toString(), selected.fingerprint
      ))
    m_recoveryPreparationError =
        tr("Address recovery could not prepare this computer’s identity. The saved preference has not been enabled for "
           "this session.");
#endif
  m_automaticProbe = executor;
  m_automaticRecovery = new AutomaticRecovery(
      m_coreProcess, m_identityBindings, *discovery, *resolver, *executor, source, this, std::move(clock)
  );
  discovery->setParent(m_automaticRecovery);
  resolver->setParent(m_automaticRecovery);
  executor->setParent(m_automaticRecovery);
  if (!m_automaticRecovery->configure(
          AutomaticRecoveryPolicy::decode(Settings::value("gui/automaticRecovery").toByteArray())
      ))
    m_recoveryPreparationError =
        tr("The saved recovery computer is no longer available. Choose a saved computer again.");
  connect(
      m_networkMonitor, &NetworkMonitor::networkConfigurationChanged, m_automaticRecovery,
      &AutomaticRecovery::networkChanged
  );
  connect(
      &m_coreProcess, &CoreProcess::recoveredConnectionRequested, m_automaticRecovery,
      [this](const QString &, const QString &host, int port) {
        m_connectionDiagnostics->beginAttempt(false, host, port, true, true);
      }
  );
  if (!m_powerMonitor)
    m_powerMonitor = new PowerMonitor(this);
  connect(m_powerMonitor, &PowerMonitor::sleepingChanged, m_automaticRecovery, &AutomaticRecovery::powerChanged);
  connect(
      m_automaticRecovery, &AutomaticRecovery::verifiedEndpoint, this,
      [this](const QString &, const QString &host, int) {
        Settings::setValue("gui/identityBindings", m_identityBindings.encode());
        ui->cbHostname->lineEdit()->setText(host);
      }
  );
  connect(m_automaticRecovery, &AutomaticRecovery::stateChanged, this, [this](auto s) {
    m_connectionDiagnostics->setDiscoveryActive(s == AutomaticRecovery::State::Searching);
    if (s == AutomaticRecovery::State::NeedsAction &&
        m_connectionDiagnostics->phase() != ConnectionDiagnostics::Phase::NeedsAction)
      m_connectionDiagnostics->requireAttention(ConnectionDiagnostics::Attention::Recovery);
  });
}

void MainWindow::setupConnectionHome()
{
  m_connectionOptionsPage = takeCentralWidget();
  m_connectionOptionsPage->layout()->setContentsMargins(16, 12, 16, 16);
  m_connectionOptionsPage->layout()->setSpacing(12);
  m_homePages = new QStackedWidget(this);
  m_home = new ConnectionHome(m_homePages);
  m_homePages->addWidget(m_home);
  m_homePages->addWidget(m_connectionOptionsPage);
  setCentralWidget(m_homePages);
  auto *back = new ActionButton(tr("My desktop"), m_connectionOptionsPage);
  back->setObjectName("backToComputers");
  back->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
  qobject_cast<QVBoxLayout *>(m_connectionOptionsPage->layout())->insertWidget(0, back, 0, Qt::AlignLeft);
  auto *pageTitle = new QLabel(tr("Settings"), m_connectionOptionsPage);
  pageTitle->setObjectName("secondaryPageTitle");
  auto heading = pageTitle->font();
  heading.setPointSize(18);
  heading.setBold(true);
  pageTitle->setFont(heading);
  qobject_cast<QVBoxLayout *>(m_connectionOptionsPage->layout())->insertWidget(1, pageTitle);
  auto *preferences = new ActionButton(tr("Application preferences…"), m_connectionOptionsPage);
  preferences->setObjectName("applicationPreferences");
  qobject_cast<QVBoxLayout *>(m_connectionOptionsPage->layout())->insertWidget(2, preferences, 0, Qt::AlignLeft);
  connect(preferences, &QPushButton::clicked, this, &MainWindow::openSettings);
  auto *advanced = new ActionButton(tr("Advanced sharing settings…"), m_connectionOptionsPage);
  advanced->setObjectName("advancedSharingSettings");
  qobject_cast<QVBoxLayout *>(m_connectionOptionsPage->layout())->insertWidget(3, advanced, 0, Qt::AlignLeft);
  connect(advanced, &QPushButton::clicked, this, &MainWindow::showAdvancedServerSettings);
  connect(back, &QPushButton::clicked, this, [this] {
    const bool returnToSettings = m_secondaryReturnToSettings;
    m_secondaryReturnToSettings = false;
    m_homePages->setCurrentWidget(m_home);
    resizeToContent();
    if (returnToSettings)
      openSettingsPage(3);
    else
      m_home->findChild<QPushButton *>("homeSettings")->setFocus();
  });
  auto *preparation = new QLabel(m_connectionOptionsPage);
  preparation->setObjectName("connectionPreparation");
  preparation->setWordWrap(true);
  preparation->setTextFormat(Qt::PlainText);
  preparation->setContentsMargins(16, 4, 16, 8);
  qobject_cast<QVBoxLayout *>(m_connectionOptionsPage->layout())->insertWidget(2, preparation);
  preparation->hide();
  connect(m_home, &ConnectionHome::roleChosen, this, [this](ConnectionHome::Role role) {
    if (m_coreProcess.processState() != ProcessState::Stopped)
      return;
    if (role == ConnectionHome::Role::Share)
      ui->rbModeServer->setChecked(true);
    else if (role == ConnectionHome::Role::Receive)
      ui->rbModeClient->setChecked(true);
    refreshConnectionHome();
  });
  connect(m_home, &ConnectionHome::addressChosen, this, [this](const QString &address) {
    if (m_coreProcess.mode() != CoreMode::Client || m_coreProcess.processState() != ProcessState::Stopped)
      return;
    ui->cbHostname->lineEdit()->setText(address);
    refreshConnectionHome();
  });
  connect(m_home, &ConnectionHome::findDeviceRequested, this, [this] {
    if (m_coreProcess.mode() == CoreMode::Client)
      findChild<QPushButton *>("nearbyComputersButton")->click();
    refreshConnectionHome();
  });
  connect(m_home, &ConnectionHome::savedDevicesRequested, this, [this] {
    findChild<QPushButton *>("savedDevicesButton")->click();
  });
  connect(m_home, &ConnectionHome::startStopRequested, this, [this] {
    if (m_coreProcess.processState() == ProcessState::Stopped)
      startCore();
    else
      stopCore();
    refreshConnectionHome();
  });
  connect(m_home, &ConnectionHome::addComputerRequested, this, &MainWindow::showAddComputer);
  connect(m_home, &ConnectionHome::retryRequested, this, [this] {
    if (m_coreProcess.processState() == ProcessState::Stopped)
      startCore();
  });
  connect(m_home, &ConnectionHome::settingsRequested, this, &MainWindow::openSettings);
  connect(m_home, &ConnectionHome::identitySetupRequested, this, [this] { openSettingsPage(3); });
  connect(m_home, &ConnectionHome::layoutRequested, this, &MainWindow::showDesktopArrangement);
  connect(m_home, &ConnectionHome::connectionOptionsRequested, this, [this] { showConnectionOptions(); });
  connect(m_home, &ConnectionHome::diagnosticsRequested, this, [this] { showConnectionOptions(true); });
  connect(m_connectionDiagnostics, &ConnectionDiagnostics::summaryChanged, this, [this] { refreshConnectionHome(); });
  connect(Settings::instance(), &Settings::settingsChanged, this, [this] { refreshConnectionHome(); });
  m_statusBar->hide();
  setupDesktopWorkspace();
  refreshConnectionHome();
}

void MainWindow::showConnectionOptions(bool diagnostics)
{
  m_secondaryDiagnostics = diagnostics;
  m_secondaryAddComputer = false;
  m_secondaryReturnToSettings = false;
  m_homePages->setCurrentWidget(m_connectionOptionsPage);
  m_connectionOptions->setChecked(!diagnostics);
  if (diagnostics)
    m_connectionDiagnostics->findChild<QPushButton *>("diagnosticDetailsButton")->setChecked(true);
  updateSecondaryPage();
  setMaximumHeight(QWIDGETSIZE_MAX);
}

void MainWindow::showAddComputer()
{
  if (m_coreProcess.mode() != CoreMode::Server)
    return;
  showConnectionOptions();
  m_secondaryAddComputer = true;
  updateSecondaryPage();
}

void MainWindow::updateSecondaryPage()
{
  if (!m_homePages)
    return;
  const bool server = m_coreProcess.mode() == CoreMode::Server;
  findChild<QLabel *>("secondaryPageTitle")
      ->setText(
          m_secondaryDiagnostics   ? tr("Diagnostics")
          : m_secondaryAddComputer ? tr("Add a computer")
                                   : tr("Computers and pairing")
      );
  findChild<QPushButton *>("backToComputers")
      ->setText(m_secondaryReturnToSettings ? tr("Back to settings") : tr("My desktop"));
  findChild<QPushButton *>("applicationPreferences")->hide();
  findChild<QPushButton *>("advancedSharingSettings")
      ->setVisible(!m_secondaryDiagnostics && !m_secondaryAddComputer && server);
  m_deviceActions->setVisible(!m_secondaryDiagnostics && !m_secondaryAddComputer);
  m_serverDiscovery->setVisible(!m_secondaryDiagnostics && server);
  ui->widget->setVisible(!m_secondaryDiagnostics);
  ui->groupBox->setVisible(!m_secondaryDiagnostics && !m_secondaryAddComputer);
  m_connectionDiagnostics->setVisible(m_secondaryDiagnostics);
  auto *preparation = findChild<QLabel *>("connectionPreparation");
  preparation->setVisible(m_secondaryAddComputer);
  preparation->setText(
      tr("On the other computer, choose Receive control. Find %1 nearby, or enter this computer's address. Verify "
         "identities on the first connection; its screens will appear here automatically.")
          .arg(Settings::value(Settings::Core::ComputerName).toString())
  );
}

void MainWindow::refreshConnectionHome()
{
  if (!m_home)
    return;
  ConnectionHome::State state;
  const auto mode = m_coreProcess.mode();
  state.role = mode == CoreMode::Client   ? ConnectionHome::Role::Receive
               : mode == CoreMode::Server ? ConnectionHome::Role::Share
                                          : ConnectionHome::Role::None;
  using P = ConnectionDiagnostics::Phase;
  using S = ConnectionHome::Stage;
  switch (m_connectionDiagnostics->phase()) {
  case P::Starting:
    state.stage = S::Starting;
    break;
  case P::Discovering:
    state.stage = S::Discovering;
    break;
  case P::Connecting:
    state.stage = S::Connecting;
    break;
  case P::Verifying:
    state.stage = S::Verifying;
    break;
  case P::Recovering:
    state.stage = S::Recovering;
    break;
  case P::Connected:
    state.stage = S::Connected;
    break;
  case P::Listening:
    state.stage = S::Listening;
    break;
  // RetryPending and the following reconnect attempt are one recovery cycle.
  // Keep one presentation so repeated core notifications cannot flicker.
  case P::Retrying:
    state.stage = S::Recovering;
    break;
  case P::Stopping:
    state.stage = S::Stopping;
    break;
  case P::NeedsAction:
    state.stage = S::NeedsAction;
    break;
  case P::Paused:
    state.stage = S::Paused;
    break;
  default:
    state.stage = S::Idle;
    break;
  }
  state.localName = Settings::value(Settings::Core::ComputerName).toString();
  state.hasAddress = mode == CoreMode::Client && !ui->cbHostname->lineEdit()->text().trimmed().isEmpty();
  if (state.hasAddress)
    state.peerName = ui->cbHostname->lineEdit()->text().trimmed();
  state.hasSavedDevices = !m_identityBindings.devices().isEmpty();
  if (state.hasAddress) {
    const IdentityBindings::Endpoint endpoint{
        ui->cbHostname->lineEdit()->text().trimmed(), Settings::value(Settings::Core::Port).toInt()
    };
    auto device = m_identityBindings.selectedDevice(endpoint, m_coreProcess.expectedPeerFingerprint());
    if (!device)
      device = m_identityBindings.selectedDevice(endpoint);
    if (device) {
      state.peerName = device->label;
      state.peerKnown = true;
    }
  } else if (mode == CoreMode::Server && state.stage == S::Connected)
    state.peerName = m_connectedClientNames.join(", ");
  state.identityReady =
      TlsUtility::isEnabled() && Settings::value(Settings::Security::CheckPeers).toBool() && TlsUtility::isCertValid();
#ifdef DESKFLOW_CANDIDATE_BUILD
  state.identityReady =
      state.identityReady &&
      deskflow::candidate::securityProfile(
          Settings::settingsFile(), Settings::value(Settings::Security::Certificate).toString(), true, true
      );
#endif
  state.canStart = canRunCore();
  state.explanation = m_connectionDiagnostics->explanation();
  state.retrySeconds = m_connectionDiagnostics->retrySeconds();
  using A = ConnectionDiagnostics::Attention;
  const auto attention = m_connectionDiagnostics->attention();
  if (state.stage == S::NeedsAction &&
      (attention == A::StartFailed || attention == A::Address || attention == A::Daemon)) {
    state.stage = S::Failed;
    state.canRetry = attention != A::Daemon && state.identityReady && state.canStart &&
                     m_coreProcess.processState() == ProcessState::Stopped;
  }
  if (state.stage == S::Idle && !m_coreProcess.connectionRequested() &&
      Settings::value(deskflow::candidate::pausedKey).toBool())
    state.stage = S::Paused;
  if (mode == CoreMode::Client && m_automaticRecovery) {
    const auto policy = m_automaticRecovery->policy();
    if (!m_recoveryPreparationError.isEmpty())
      state.recoverySummary = m_recoveryPreparationError;
    else if (!policy.enabled)
      state.recoverySummary = tr("Address recovery is off. Set it up for a saved computer if its address changes.");
    else if (!m_automaticProbe || !m_automaticProbe->ready())
      state.recoverySummary =
          tr("Address recovery is saved but not ready. Review the saved computer’s recovery settings.");
    else if (policy.fingerprint != m_coreProcess.expectedPeerFingerprint() && m_coreProcess.connectionRequested())
      state.recoverySummary = tr("Address recovery is configured for a different saved computer.");
    else if (m_automaticRecovery->state() == AutomaticRecovery::State::WaitingNetwork)
      state.recoverySummary = tr("Address recovery is waiting for the selected network.");
    else
      state.recoverySummary = tr("Address recovery: %1. Pausing also pauses recovery.").arg(policy.network.name);
  }
  m_home->setState(state);
  refreshDesktopWorkspace();
}

void MainWindow::setupControls()
{
  secureSocket(false);
  ui->btnSaveServerConfig->setText(tr("Export…"));
  ui->btnSaveServerConfig->setAccessibleName(tr("Export server configuration"));
  m_connectionOptions = new QToolButton(this);
  m_connectionOptions->setObjectName("connectionOptionsToggle");
  m_connectionOptions->setText(tr("Connection settings"));
  m_connectionOptions->setCheckable(true);
  m_connectionOptions->setChecked(true);
  m_connectionOptions->setAutoRaise(true);
  m_connectionOptions->setArrowType(Qt::DownArrow);
  m_connectionOptions->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  auto *connectionHeader = new QHBoxLayout;
  connectionHeader->addWidget(m_connectionOptions);
  connectionHeader->addStretch();
  ui->horizontalWidget->layout()->removeWidget(ui->btnToggleCore);
  ui->horizontalWidget->layout()->removeWidget(ui->btnRestartCore);
  connectionHeader->addWidget(ui->btnRestartCore);
  ui->horizontalWidget->hide();
  qobject_cast<QBoxLayout *>(ui->groupBox->layout())->insertLayout(0, connectionHeader);
  connect(m_connectionOptions, &QToolButton::toggled, this, [this](bool expanded) {
    m_connectionOptions->setArrowType(expanded ? Qt::DownArrow : Qt::RightArrow);
    updateModeControls();
    QTimer::singleShot(0, this, &MainWindow::resizeToContent);
  });
  connect(m_connectionDiagnostics, &ConnectionDiagnostics::manualConnectionRequested, this, [this] {
    if (m_home) {
      m_homePages->setCurrentWidget(m_home);
      m_home->showManualEntry();
      return;
    }
    m_connectionOptions->setChecked(true);
    QTimer::singleShot(0, this, [this] {
      ui->cbHostname->lineEdit()->setFocus(Qt::OtherFocusReason);
      ui->cbHostname->lineEdit()->selectAll();
      findChild<QScrollArea *>("mainContentScroll")->ensureWidgetVisible(ui->cbHostname, 12, 12);
    });
  });
  auto *deviceRow = qobject_cast<QHBoxLayout *>(m_deviceActions->layout());
  auto *savedDevices = new QPushButton(tr("Saved devices"), this);
  savedDevices->setObjectName("savedDevicesButton");
  connect(
      m_connectionDiagnostics, &ConnectionDiagnostics::recoverySettingsRequested, savedDevices, &QPushButton::click
  );
  connect(savedDevices, &QPushButton::clicked, this, [this] {
    const bool allowed = canQuickReconnect() && m_coreProcess.processState() == ProcessState::Stopped;
    const auto reason = m_coreProcess.mode() == CoreMode::Server
                            ? tr("This computer provides keyboard and mouse input. Start a connection from the "
                                 "receiving computer. Saved names and recovery preferences remain available.")
                        : m_coreProcess.processState() != ProcessState::Stopped
                            ? tr("A connection is active. Disconnect before choosing another controlling computer. "
                                 "Saved names and recovery preferences remain available.")
                            : tr("Prepare this computer's identity and receiving role before reconnecting. Saved names "
                                 "and recovery preferences remain available.");
    TrustedDevicesDialog dialog(m_identityBindings, allowed, this, reason);
    connect(&dialog, &TrustedDevicesDialog::metadataChanged, this, [this] {
      Settings::setValue("gui/identityBindings", m_identityBindings.encode());
      refreshConnectionHome();
    });
    connect(
        &dialog, &TrustedDevicesDialog::connectRequested, this,
        [this](const QString &fp, const QString &host, int port) {
          if (m_coreProcess.setExpectedPeerFingerprint(fp))
            quickReconnect(host, port);
        }
    );
    connect(&dialog, &TrustedDevicesDialog::automaticRecoveryRequested, &dialog, [this, &dialog](const QString &fp) {
      AutomaticRecoveryDialog settings(fp, m_automaticRecovery->policy(), DiscoveryScope::current, &dialog);
      connect(&settings, &AutomaticRecoveryDialog::preparedSessionRequested, this, [this, &settings] {
        if (!deskflow::lab::kProbeNetworkExecutionEnabled)
          return;
        const auto root =
            QFileDialog::getExistingDirectory(&settings, tr("Choose an explicitly prepared test session"));
        if (!root.isEmpty())
          settings.showCredentialResult(m_automaticProbe->selectPreparedSession(root));
      });
      settings.setReceivingRole(m_coreProcess.mode() == CoreMode::Client);
      settings.setApplyHandler([this, fp](const AutomaticRecoveryPolicy &policy) -> QString {
        if (policy.enabled && (!policy.valid() || !m_identityBindings.devices().contains(fp)))
          return tr("This saved computer or network is no longer available. Nothing was saved.");
#ifdef DESKFLOW_CANDIDATE_BUILD
        const auto previous = m_automaticRecovery->policy();
        if (policy.enabled &&
            !m_automaticProbe->selectCandidateProfile(
                Settings::settingsFile(), Settings::value(Settings::Security::Certificate).toString(), fp
            )) {
          if (previous.enabled)
            m_automaticProbe->selectCandidateProfile(
                Settings::settingsFile(), Settings::value(Settings::Security::Certificate).toString(),
                previous.fingerprint
            );
          return tr(
              "Could not prepare identity verification for recovery. Check this computer’s identity in Settings. "
              "Nothing was saved."
          );
        }
#endif
        if (!m_automaticRecovery->configure(policy))
          return tr("Recovery could not be configured. The previous preference is unchanged.");
        Settings::setValue("gui/automaticRecovery", policy.encode());
        m_recoveryPreparationError.clear();
        // Merely saving a policy must not change the identity of a current or
        // future explicit connection. prepareStart binds it when appropriate.
        updateModeControls();
        refreshConnectionHome();
        return {};
      });
      settings.exec();
    });
    connect(&dialog, &TrustedDevicesDialog::locateRequested, this, [this](const QString &fp) {
      createRecoveryDialog(fp, deskflow::gui::DiscoveryScope::current)->open();
    });
    dialog.exec();
  });
  auto *nearby = new QPushButton(tr("Nearby computers"), this);
  nearby->setObjectName("nearbyComputersButton");
  nearby->setProperty("primaryAction", true);
  deviceRow->addWidget(nearby);
  deviceRow->addWidget(savedDevices);
  deviceRow->addWidget(m_recentServers);
  ui->serverOptions->layout()->removeWidget(ui->btnConfigureServer);
  ui->btnConfigureServer->setText(tr("Computer layout…"));
  ui->btnConfigureServer->setProperty("primaryAction", true);
  deviceRow->addWidget(ui->btnConfigureServer);
  deviceRow->addStretch();
  deviceRow->addWidget(ui->btnToggleCore);
  connect(nearby, &QPushButton::clicked, this, [this] {
#ifdef DESKFLOW_UI_REVIEW_BUILD
    NearbyDevicesDialog dialog(this, [] { return QVector<DiscoveryScope>{}; });
#else
    NearbyDevicesDialog dialog(this);
#endif
    connect(
        &dialog, &NearbyDevicesDialog::searchingChanged, m_connectionDiagnostics,
        &ConnectionDiagnostics::setDiscoveryActive
    );
    connect(&dialog, &NearbyDevicesDialog::manualEntryRequested, this, [this] {
      m_homePages->setCurrentWidget(m_home);
      m_home->showManualEntry();
    });
    if (dialog.exec() == QDialog::Accepted) {
      // User selection edits an endpoint only. Existing Connect + TLS/pin flow
      // still decides whether a session can be established and trusted.
      Settings::setValue(Settings::Core::Port, dialog.selectedPort());
      ui->cbHostname->lineEdit()->setText(dialog.selectedHost());
    }
    m_connectionDiagnostics->setDiscoveryActive(false);
  });
  m_nearbyPublisher = new NearbyPublisher(this);
  // Server discovery is a primary setup step, independent of advanced settings
  // and long diagnostic text. Keep it outside the scrolling content.
  m_serverDiscovery = new QWidget(this);
  m_serverDiscovery->setObjectName("serverDiscoveryControls");
  m_serverDiscovery->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
  auto *discoveryLayout = new QVBoxLayout(m_serverDiscovery);
  discoveryLayout->setContentsMargins(16, 10, 16, 12);
  discoveryLayout->setSpacing(8);
  qobject_cast<QVBoxLayout *>(centralWidget()->layout())->insertWidget(1, m_serverDiscovery);
  auto *advertiseNetwork = new QComboBox(this);
  advertiseNetwork->setObjectName("advertiseNetwork");
  advertiseNetwork->setAccessibleName(tr("Local network for discovery"));
  auto scopes = std::make_shared<QVector<DiscoveryScope>>();
  auto reloadNetworks = [advertiseNetwork, scopes] {
    const QSignalBlocker blocker(advertiseNetwork);
    *scopes = DiscoveryScope::current();
    advertiseNetwork->clear();
    advertiseNetwork->addItem(tr("Choose a private Wi-Fi or Ethernet network"));
    for (const auto &scope : *scopes)
      advertiseNetwork->addItem(tr("%1 · %2").arg(scope.name, scope.address.toString()));
  };
  reloadNetworks();
  discoveryLayout->addWidget(advertiseNetwork);
  auto *duration = new QComboBox(this);
  duration->setObjectName("advertiseDuration");
  duration->setAccessibleName(tr("Discovery duration"));
  duration->addItem(tr("5 minutes"));
  duration->addItem(tr("Until I stop · selected network only"));
  auto *durationLabel = new QLabel(tr("Discovery duration"), this);
  durationLabel->setObjectName("advertiseDurationLabel");
  durationLabel->setBuddy(duration);
  auto *durationRow = new QHBoxLayout;
  durationRow->setSpacing(12);
  durationRow->addWidget(durationLabel);
  durationRow->addWidget(duration, 1);
  discoveryLayout->addLayout(durationRow);
  auto *advertise = new deskflow::gui::OptionCheckBox(tr("Make this server discoverable"), this);
  advertise->setObjectName("advertiseNearbyServer");
  advertise->setChecked(false);
  advertise->setEnabled(false);
  advertise->setToolTip(tr(
      "Shares a temporary random name, selected private address and port. No account name or fingerprint is advertised."
  ));
  discoveryLayout->addWidget(advertise);
  auto *advertiseStatus = new QLabel(this);
  advertiseStatus->setObjectName("advertiseStatus");
  advertiseStatus->setWordWrap(true);
  advertiseStatus->setTextFormat(Qt::PlainText);
  discoveryLayout->addWidget(advertiseStatus);
  auto configurePublisher = [this, advertise, advertiseNetwork, advertiseStatus, scopes, duration] {
    const auto index = advertiseNetwork->currentIndex() - 1;
    const auto scope = index >= 0 && index < scopes->size() ? (*scopes)[index] : DiscoveryScope{};
    const auto binding = Settings::value(Settings::Core::Interface).toString();
    const bool allowed = scope.valid() && (binding.isEmpty() || QHostAddress(binding) == scope.address);
    if (!advertise->isChecked())
      advertiseStatus->setText(
          index < 0 ? tr("Select a local network above to enable discovery.")
                    : tr("Discovery is off. Enable it to let nearby devices find this server.")
      );
    m_nearbyPublisher->configure(
        advertise->isChecked() && allowed, m_coreProcess.mode() == CoreMode::Server,
        Settings::value(Settings::Core::Port).toInt(), Settings::value(Settings::Security::TlsEnabled).toBool(),
        Settings::value(Settings::Security::CheckPeers).toBool(), scope, duration->currentIndex() == 1
    );
    if (advertise->isChecked() && !allowed)
      advertiseStatus->setText(tr("The selected network does not match the server binding. Discovery stays off."));
  };
  connect(advertiseNetwork, &QComboBox::currentIndexChanged, this, [advertise, configurePublisher](int index) {
    advertise->setChecked(false);
    advertise->setEnabled(index > 0);
    configurePublisher();
  });
  connect(advertise, &QCheckBox::toggled, this, [configurePublisher] { configurePublisher(); });
  connect(duration, &QComboBox::currentIndexChanged, this, [advertise, configurePublisher] {
    // Changing exposure duration requires a new explicit opt-in.
    advertise->setChecked(false);
    configurePublisher();
  });
  connect(&m_coreProcess, &CoreProcess::connectionIntentChanged, this, [this](bool requested) {
    if (!requested)
      m_nearbyPublisher->configure(false, false, 0, true, true, {});
  });
  connect(m_nearbyPublisher, &NearbyPublisher::status, advertiseStatus, &QLabel::setText);
  connect(m_nearbyPublisher, &NearbyPublisher::exposureEnded, this, [advertise] {
    const QSignalBlocker blocker(advertise);
    advertise->setChecked(false);
  });
  connect(&m_coreProcess, &CoreProcess::connectionAttemptStarting, this, [configurePublisher] {
    configurePublisher();
  });
  connect(&m_coreProcess, &CoreProcess::processStateChanged, m_nearbyPublisher, &NearbyPublisher::processChanged);
  connect(
      &m_coreProcess, &CoreProcess::connectionStateObserved, m_nearbyPublisher, &NearbyPublisher::connectionChanged
  );
  connect(
      m_networkMonitor, &NetworkMonitor::networkConfigurationChanged, this,
      [this, reloadNetworks, advertise, advertiseNetwork, scopes] {
        // Listener recovery is independent of optional discovery exposure.
        m_coreProcess.observeServerInterfaces();
        m_nearbyPublisher->networkChanged();
        reloadNetworks();
        const QSignalBlocker blocker(advertiseNetwork), checkBlocker(advertise);
        if (m_nearbyPublisher->enabled()) {
          auto i = scopes->indexOf(m_nearbyPublisher->scope());
          if (i < 0) {
            scopes->append(m_nearbyPublisher->scope());
            i = scopes->size() - 1;
            advertiseNetwork->addItem(tr("Selected network unavailable"));
          }
          advertiseNetwork->setCurrentIndex(i + 1);
          advertise->setChecked(true);
          advertise->setEnabled(true);
        } else {
          advertise->setChecked(false);
          advertise->setEnabled(false);
        }
      }
  );
  configurePublisher();
  m_recentServers->loadHistory(Settings::value(Settings::Gui::RecentServers).toString());
  const auto identityRecords = Settings::value("gui/identityBindings").toByteArray();
  if (!identityRecords.isEmpty())
    m_identityBindings.load(identityRecords);
  setupAutomaticRecovery();
  // Connection settings stay reachable ahead of expanded diagnostic details.
  qobject_cast<QBoxLayout *>(ui->topLevelWidget->layout())->addWidget(m_connectionDiagnostics);
  connect(
      m_connectionDiagnostics->findChild<QPushButton *>("diagnosticDetailsButton"), &QPushButton::toggled, this,
      [this] { QTimer::singleShot(0, this, &MainWindow::resizeToContent); }
  );
  // Status text and retry countdowns are transient. They must never resize the
  // window; reserved label heights keep one- and two-line states geometrically
  // identical so a reconnect loop cannot make the whole app breathe vertically.
  m_statusBar->setConnectionSummaryVisible(false); // One authoritative connection summary.

  ui->btnConfigureServer->setIcon(QIcon::fromTheme(QStringLiteral("configure")));
  ui->btnConfigureClient->setIcon(QIcon::fromTheme(QStringLiteral("configure")));

  if (Settings::value(Settings::Core::LastVersion).toString() != kVersion) {
    Settings::setValue(Settings::Core::LastVersion, kVersion);
  }

  m_logDock->hide(); // Technical logs are an explicit secondary view.

  ui->serverOptions->setVisible(false);
  ui->clientOptions->setVisible(false);

  const auto coreMode = Settings::value(Settings::Core::CoreMode).value<Settings::CoreMode>();
  ui->rbModeClient->setChecked(coreMode == Settings::CoreMode::Client);
  ui->rbModeServer->setChecked(coreMode == Settings::CoreMode::Server);

  ui->lineEditName->setValidator(new QRegularExpressionValidator(m_nameRegEx, this));
  ui->lineEditName->setVisible(false);
  ui->lineEditName->installEventFilter(this);

  if (deskflow::platform::isMac()) {
    ui->rbModeServer->setAttribute(Qt::WA_MacShowFocusRect, false);
    ui->rbModeClient->setAttribute(Qt::WA_MacShowFocusRect, false);

  } else {
    ui->btnSaveServerConfig->setIconSize(QSize(22, 22));
  }
  setStatusBar(m_statusBar);
}

//////////////////////////////////////////////////////////////////////////////
// Begin slots
//////////////////////////////////////////////////////////////////////////////

// remember: using queued connection allows the render loop to run before
// executing the slot. the default is to instantly call the slot when the
// signal is emitted from the thread that owns the receiver's object.
void MainWindow::connectSlots()
{
  connect(&m_coreProcess, &CoreProcess::authenticationCancelled, this, [this] { m_identityBindings.cancel(); });
  connect(
      &m_coreProcess, &CoreProcess::authenticationStarting, this,
      [this](const deskflow::auth::Context &context, const QString &host, int port) {
        if (!m_pendingPairFingerprint.isEmpty() && m_pendingPairFingerprint == context.expected) {
          const auto label = m_pendingPairLabel;
          m_pendingPairFingerprint.clear();
          m_pendingPairLabel.clear();
          m_identityBindings.beginReviewedAuthentication(context, {host, port}, label);
        } else
          m_identityBindings.beginAuthentication(context, {host, port});
      }
  );
  connect(&m_coreProcess, &CoreProcess::authenticationResult, this, [this](const deskflow::auth::Report &report) {
    if (m_identityBindings.consumeAuthentication(report)) {
      Settings::setValue("gui/identityBindings", m_identityBindings.encode());
      refreshConnectionHome();
    }
    if (report.outcome == deskflow::auth::Outcome::Started)
      m_connectionDiagnostics->peerFingerprintObserved();
    else if (report.outcome != deskflow::auth::Outcome::Verified) {
      m_connectionDiagnostics->requireAttention(ConnectionDiagnostics::Attention::Identity);
      if (report.outcome == deskflow::auth::Outcome::WrongIdentity &&
          m_identityBindings.attempt().review == deskflow::gui::IdentityBindings::Review::ChangedIdentity) {
        // Keep the rejected observation separate from the live model. Closing
        // this window cannot replace a pin or approve a different certificate.
        auto review = std::make_shared<deskflow::gui::IdentityBindings>(m_identityBindings);
        auto *dialog = new PairingReviewDialog(*review, {}, this);
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        connect(dialog, &QDialog::destroyed, this, [review] {});
        dialog->open();
      }
      if (report.outcome == deskflow::auth::Outcome::Untrusted && report.context.expected.isEmpty())
        handlePeerFingerprint(report.observed);
    }
  });
  // Bind real typed core events before modal fingerprint/refusal handlers.
  connect(
      &m_coreProcess, &CoreProcess::connectionAttemptStarting, m_connectionDiagnostics,
      &ConnectionDiagnostics::beginAttempt
  );
  connect(
      &m_coreProcess, &CoreProcess::processStateChanged, m_connectionDiagnostics,
      &ConnectionDiagnostics::processStateChanged
  );
  connect(
      &m_coreProcess, &CoreProcess::connectionIntentChanged, m_connectionDiagnostics,
      &ConnectionDiagnostics::connectionIntentChanged
  );
  connect(
      &m_coreProcess, &CoreProcess::connectionStateObserved, m_connectionDiagnostics,
      &ConnectionDiagnostics::connectionStateChanged
  );
  connect(&m_coreProcess, &CoreProcess::secureSocket, m_connectionDiagnostics, &ConnectionDiagnostics::secureSocket);
  connect(
      &m_coreProcess, &CoreProcess::peerFingerprint, m_connectionDiagnostics,
      &ConnectionDiagnostics::peerFingerprintObserved
  );
  connect(&m_coreProcess, &CoreProcess::retryIn, m_connectionDiagnostics, &ConnectionDiagnostics::retryIn);
  connect(
      &m_coreProcess, &CoreProcess::connectionRefused, m_connectionDiagnostics,
      &ConnectionDiagnostics::connectionRefused
  );
  connect(
      &m_coreProcess, &CoreProcess::connectedClientsChanged, m_connectionDiagnostics,
      &ConnectionDiagnostics::connectedClientsChanged
  );
  connect(&m_coreProcess, &CoreProcess::error, this, [this](CoreProcess::Error error) {
    m_connectionDiagnostics->requireAttention(
        error == CoreProcess::Error::AddressMissing ? ConnectionDiagnostics::Attention::Address
                                                    : ConnectionDiagnostics::Attention::StartFailed
    );
  });
  connect(&m_coreProcess, &CoreProcess::daemonIpcClientConnectionFailed, this, [this] {
    m_connectionDiagnostics->requireAttention(ConnectionDiagnostics::Attention::Daemon);
  });
  connect(m_connectionDiagnostics, &ConnectionDiagnostics::settingsRequested, this, &MainWindow::openSettings);
  connect(m_connectionDiagnostics, &ConnectionDiagnostics::reconnectRequested, this, [this] {
    if (m_coreProcess.processState() == ProcessState::Stopped)
      startCore();
    else if (m_coreProcess.processState() == ProcessState::Started ||
             m_coreProcess.processState() == ProcessState::RetryPending)
      resetCore();
  });
  connect(Settings::instance(), &Settings::serverSettingsChanged, this, &MainWindow::serverConfigSaving);
  connect(Settings::instance(), &Settings::settingsChanged, this, &MainWindow::settingsChanged);

  connect(&m_coreProcess, &CoreProcess::error, this, &MainWindow::coreProcessError);
  connect(&m_coreProcess, &CoreProcess::logLine, this, &MainWindow::handleLogLine);
  connect(
      &m_coreProcess, &CoreProcess::processStateChanged, this, &MainWindow::coreProcessStateChanged,
      Qt::QueuedConnection
  );
  connect(&m_coreProcess, &CoreProcess::connectionStateChanged, this, &MainWindow::coreConnectionStateChanged);
  connect(&m_coreProcess, &CoreProcess::secureSocket, this, &MainWindow::secureSocket);
  connect(&m_coreProcess, &CoreProcess::clientConnectionStarting, m_recentServers, &RecentServersButton::beginAttempt);
  connect(
      &m_coreProcess, &CoreProcess::connectionStateChanged, m_recentServers,
      &RecentServersButton::connectionStateChanged
  );
  connect(&m_coreProcess, &CoreProcess::secureSocket, m_recentServers, &RecentServersButton::secureSocket);
  // Direct delivery invalidates the previous attempt before any new IPC state can arrive.
  connect(&m_coreProcess, &CoreProcess::processStateChanged, this, [this](ProcessState state) {
    if (state == ProcessState::Starting || state == ProcessState::Stopping || state == ProcessState::Stopped) {
      m_recentServers->cancelAttempt();
      m_connectedClientNames.clear();
    }
    updateQuickReconnectControls();
  });
  connect(m_recentServers, &RecentServersButton::reconnectRequested, this, &MainWindow::quickReconnect);
  connect(m_recentServers, &RecentServersButton::historyChanged, this, [](const QString &history) {
    Settings::setValue(Settings::Gui::RecentServers, history);
  });
  connect(
      &m_coreProcess, &CoreProcess::daemonIpcClientConnectionFailed, this, &MainWindow::daemonIpcClientConnectionFailed
  );
  connect(&m_coreProcess, &CoreProcess::securityLevelChanged, m_statusBar, &StatusBar::setSecurityLevel);

  connect(m_actionAbout, &QAction::triggered, this, &MainWindow::openAboutDialog);
  connect(m_actionMinimize, &QAction::triggered, this, &MainWindow::hide);

  connect(m_actionQuit, &QAction::triggered, this, &MainWindow::close);
  connect(m_actionTrayQuit, &QAction::triggered, this, &MainWindow::close);
  connect(m_actionRestore, &QAction::triggered, this, &MainWindow::showAndActivate);
  connect(m_actionSettings, &QAction::triggered, this, &MainWindow::openSettings);
  connect(m_actionStartCore, &QAction::triggered, this, &MainWindow::startCore);
  connect(m_actionRestartCore, &QAction::triggered, this, &MainWindow::resetCore);
  connect(m_actionStopCore, &QAction::triggered, this, &MainWindow::stopCore);
  connect(m_actionShowHelp, &QAction::triggered, this, &MainWindow::showHelpViewer);

  // Mac os tray will only show a menu
  if (!deskflow::platform::isMac())
    connect(m_trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::trayIconActivated);

  connect(
      &m_coreProcess, &CoreProcess::displayTopologyReceived, this,
      [this](const QString &device, const QByteArray &payload) {
        if (!m_serverConfig.computerExists(device))
          return;
        auto snapshots =
            deskflow::display::decodeTopologies(Settings::value(Settings::Server::MonitorTopologies).toByteArray());
        if (const auto topology = deskflow::display::Topology::decode(payload))
          snapshots[device] = *topology;
        else
          snapshots.remove(device);
        Settings::setValue(Settings::Server::MonitorTopologies, deskflow::display::encodeTopologies(snapshots));
      }
  );
  connect(
      &m_coreProcess, &CoreProcess::verifiedPeerNameReceived, this,
      [this](const QString &fingerprint, const QString &name) {
        if (m_identityBindings.updateDefaultLabel(fingerprint, name)) {
          Settings::setValue("gui/identityBindings", m_identityBindings.encode());
          refreshConnectionHome();
        }
      }
  );
  connect(&m_coreProcess, &CoreProcess::monitorRoutingModeChanged, this, [this](bool perMonitor) {
    if (!perMonitor && !Settings::value(Settings::Server::MonitorLayout).toByteArray().isEmpty())
      m_logDock->appendLine(
          tr("Per-display layout is unavailable or changed. Using the compatibility computer layout until all saved "
             "displays reconnect.")
      );
  });
  connect(&m_coreProcess, &CoreProcess::connectedClientsChanged, this, &MainWindow::serverClientsChanged);
  connect(&m_coreProcess, &CoreProcess::unrecognisedClient, this, &MainWindow::handleUnrecognisedClient);
  connect(&m_coreProcess, &CoreProcess::connectionRefused, this, &MainWindow::handleConnectionRefused);
  connect(&m_coreProcess, &CoreProcess::retryIn, this, &MainWindow::updateTimeoutDelay);
  connect(&m_coreProcess, &CoreProcess::peerFingerprint, this, &MainWindow::handlePeerFingerprint);
  connect(&m_coreProcess, &CoreProcess::missingKeyboardLayouts, this, &MainWindow::handleMissingKeyboardLayouts);

  if (m_startupMode == StartupMode::Normal && deskflow::candidate::mayResume(
                                                  Settings::value(Settings::Gui::AutoStartCore).toBool(),
                                                  Settings::value(deskflow::candidate::pausedKey).toBool()
                                              )) {
    connect(ui->btnToggleCore, &QPushButton::clicked, m_actionStopCore, &QAction::trigger, Qt::UniqueConnection);
  } else {
    connect(ui->btnToggleCore, &QPushButton::clicked, m_actionStartCore, &QAction::trigger, Qt::UniqueConnection);
  }

  connect(ui->btnRestartCore, &QPushButton::clicked, this, &MainWindow::resetCore);

  connect(ui->cbHostname->lineEdit(), &QLineEdit::returnPressed, ui->btnToggleCore, &QPushButton::click);
  connect(ui->cbHostname, &QComboBox::editTextChanged, this, &MainWindow::remoteHostChanged);

  connect(ui->btnSaveServerConfig, &QPushButton::clicked, this, &MainWindow::saveServerConfig);
  connect(ui->btnConfigureServer, &QPushButton::clicked, this, [this] { showConfigureServer(""); });
  connect(ui->btnConfigureClient, &QPushButton::clicked, this, [this] { showConfigureClient(); });
  connect(ui->lblComputerName, &QLabel::linkActivated, this, &MainWindow::openSettings);

  connect(ui->rbModeServer, &QRadioButton::toggled, this, &MainWindow::coreModeToggled);
  connect(ui->rbModeClient, &QRadioButton::toggled, this, &MainWindow::coreModeToggled);

  connect(m_logDock->toggleViewAction(), &QAction::toggled, this, &MainWindow::toggleLogVisible);

  connect(m_statusBar, &StatusBar::requestShowMyFingerprints, this, &MainWindow::showMyFingerprint);
  connect(m_statusBar, &StatusBar::requestUpdateVersion, this, &MainWindow::openGetNewVersionUrl);
  connect(&m_versionChecker, &VersionChecker::updateFound, m_statusBar, &StatusBar::updateFound);

  connect(m_guiDupeChecker, &QLocalServer::newConnection, this, &MainWindow::showAndActivate);

  connect(ui->btnEditName, &QPushButton::clicked, this, &MainWindow::showHostNameEditor);

  connect(ui->lineEditName, &QLineEdit::editingFinished, this, &MainWindow::setHostName);

  connect(m_networkMonitor, &NetworkMonitor::ipAddressesChanged, this, &MainWindow::updateIpLabel);
}

void MainWindow::toggleLogVisible(bool visible)
{
  // When the main window is hidden e.g. close to tray / minimized), this also triggers the log visibility toggle,
  // but we don't want to hide the log in this case since we would need to un-hide it when the window is shown again.
  if (!isVisible() || isMinimized()) {
    qDebug() << "not toggling log, window not visible";
    return;
  }

  Settings::setValue(Settings::Gui::LogExpanded, visible);
  if (visible && !m_logDock->isFloating() && m_homePages && m_homePages->currentWidget() == m_home) {
    // A log is a secondary pane. Opening it must not resize the user's window
    // to a stale form hint or consume the desktop's primary workspace.
    QTimer::singleShot(0, this, [this] {
      if (m_logDock->isVisible() && !m_logDock->isFloating())
        resizeDocks({m_logDock}, {qBound(72, height() / 4, 180)}, Qt::Vertical);
    });
  } else if (visible && !m_logDock->isFloating())
    resize(m_expandedSize.expandedTo(sizeHint()));
  else {
    if (!visible) {
      m_expandedSize = size();
      m_logDock->hide();
    }
    QTimer::singleShot(0, this, &MainWindow::resizeToContent);
  }
  Settings::setValue(Settings::Gui::WindowGeometry, geometry());
}

void MainWindow::settingsChanged(const QString &key)
{
  if (key.isEmpty() || key == Settings::Client::RemoteHost || key == Settings::Core::Port ||
      key == Settings::Core::CoreMode || key == Settings::Core::ProcessMode || key == Settings::Core::Interface ||
      key.startsWith(QStringLiteral("security/"))) {
    // An edited target/policy is not evidence about the running connection.
    m_recentServers->cancelAttempt();
    updateQuickReconnectControls();
  }
  if (key.isEmpty() || key == Settings::Gui::RecentServers)
    m_recentServers->loadHistory(Settings::value(Settings::Gui::RecentServers).toString());

  if (key == Settings::Log::Level) {
    m_coreProcess.applyLogLevel();
    return;
  }

  if (key == Settings::Core::ComputerName)
    updateComputerName();

  if ((key == Settings::Security::Certificate) || (key == Settings::Security::KeySize) ||
      (key == Settings::Security::TlsEnabled) || (key == Settings::Security::CheckPeers)) {
    if (TlsUtility::isEnabled()) {
#ifdef DESKFLOW_CANDIDATE_BUILD
      if (!deskflow::candidate::securityProfile(
              Settings::settingsFile(), Settings::value(Settings::Security::Certificate).toString(), true,
              Settings::value(Settings::Security::CheckPeers).toBool()
          ) ||
          !TlsUtility::isCertValid()) {
        m_connectionDiagnostics->requireAttention(ConnectionDiagnostics::Attention::Identity);
        return;
      }
#else
      if (!TlsUtility::isCertValid()) {
        qWarning() << tr("invalid certificate, generating a new one");
        TlsUtility::generateCertificate();
      }
#endif
      m_fingerprint = {QCryptographicHash::Sha256, TlsUtility::certFingerprint()};
      updateFingerprintButton();
    }
    updateSecurityIcon(m_statusBar->securityIconVisible());
    return;
  }
}

void MainWindow::serverConfigSaving()
{
  m_serverConfig.commit();
}

void MainWindow::trayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
  if (reason != QSystemTrayIcon::Trigger)
    return;
  isVisible() ? hide() : showAndActivate();
}

void MainWindow::coreProcessError(CoreProcess::Error error)
{
  if (error == CoreProcess::Error::AddressMissing) {
    QMessageBox::warning(
        this, tr("Address missing"), tr("Please enter the hostname or IP address of the other computer.")
    );
  } else if (error == CoreProcess::Error::StartFailed) {
    show();
    auto message = tr("The Core executable could not be started.\n"
                      "Please check if you have sufficient permissions to run %1.")
                       .arg(kCoreBinName);

    if (Settings::value(Settings::Core::CoreMode) == Settings::CoreMode::Server) {
      const auto mode =
          Settings::value(Settings::Server::ExternalConfigFile).toBool() ? tr("read") : tr("read and write");
      message.append(tr("\nAdditionally, check you are able to %1 the server config file: %2")
                         .arg(mode, Settings::serverConfigFile()));
    }
    QMessageBox::warning(this, kAppName, message);
  }
}

void MainWindow::startCore()
{
  // No Mode Set
  if (m_coreProcess.mode() == CoreMode::None) {
    qDebug().noquote() << "skipping core start, no mode set";
    return;
  }

  // Invalid client settings
  if (m_coreProcess.mode() == CoreMode::Client && Settings::value(Settings::Client::RemoteHost).toString().isEmpty()) {
    qDebug().noquote() << "skipping core start, undefined remotehost";
    return;
  }

#ifdef DESKFLOW_CANDIDATE_BUILD
  if (!deskflow::candidate::securityProfile(
          Settings::settingsFile(), Settings::value(Settings::Security::Certificate).toString(),
          TlsUtility::isEnabled(), Settings::value(Settings::Security::CheckPeers).toBool()
      ) ||
      !TlsUtility::isCertValid()) {
    m_connectionDiagnostics->requireAttention(ConnectionDiagnostics::Attention::Identity);
    return;
  }
#endif
  if (m_coreProcess.mode() == CoreMode::Client && m_coreProcess.processState() == ProcessState::Stopped) {
    const IdentityBindings::Endpoint endpoint{
        ui->cbHostname->lineEdit()->text().trimmed(), Settings::value(Settings::Core::Port).toInt()
    };
    const auto current = m_coreProcess.expectedPeerFingerprint();
    auto selected = m_identityBindings.selectedDevice(endpoint, current);
    if (!selected)
      selected = m_identityBindings.selectedDevice(endpoint);
    if (!selected && m_identityBindings.atEndpoint(endpoint).size() > 1) {
      m_connectionDiagnostics->requireAttention(ConnectionDiagnostics::Attention::Recovery);
      return;
    }
    // Restore the exact saved peer after an app restart. Explicit new pairing
    // carries its one-launch pin until authentication creates the shortcut.
    const auto expected = selected ? selected->sha256 : current == m_pendingPairFingerprint ? current : QString();
    if (!m_coreProcess.setExpectedPeerFingerprint(expected))
      return;
  }
  Settings::setValue(deskflow::candidate::pausedKey, false);
  if (m_automaticRecovery && m_coreProcess.mode() == CoreMode::Client && m_automaticRecovery->policy().enabled) {
    if (!m_automaticRecovery->prepareStart(
            Settings::value(Settings::Client::RemoteHost).toString(), Settings::value(Settings::Core::Port).toInt()
        ))
      m_recoveryPreparationError =
          tr("Address recovery is not ready for this connection. Connecting at the selected address remains available."
          );
    else
      m_recoveryPreparationError.clear();
  }
  // Save current IP state when server starts
  if (m_coreProcess.mode() == CoreMode::Server && Settings::value(Settings::Core::Interface).toString().isEmpty()) {
    m_serverStartIPs = NetworkMonitor::validAddresses();
    m_serverStartSuggestedIP = m_serverStartIPs.isEmpty() ? "" : m_serverStartIPs.first();
  }

  m_actionStartCore->setVisible(false);
  m_actionRestartCore->setVisible(true);
  // Suppress duplicate prompts only within one attempt. An explicit retry
  // after cancelling must be able to show the comparison again.
  m_checkedServers.clear();
  m_checkedClients.clear();
  m_coreProcess.start();
}

void MainWindow::stopCore()
{
  Settings::setValue(deskflow::candidate::pausedKey, true);
  Settings::setValue(Settings::Gui::AutoStartCore, false);
  if (m_nearbyPublisher)
    m_nearbyPublisher->configure(false, false, 0, true, true, {});
  qDebug() << "stopping core process";
  m_coreProcess.stop();
  m_actionStartCore->setVisible(true);
  m_actionRestartCore->setVisible(false);
}

void MainWindow::clearSettings()
{
  qDebug() << "clearing settings";

  m_networkMonitor->stopMonitoring();

  disconnect(&m_coreProcess, nullptr, this, nullptr);
  disconnect(&m_versionChecker, nullptr, this, nullptr);
  disconnect(m_guiDupeChecker, nullptr, this, nullptr);
  disconnect(m_trayIcon, nullptr, this, nullptr);
  disconnect(m_logDock->toggleViewAction(), nullptr, this, nullptr);

  m_coreProcess.stop();
  m_coreProcess.clearSettings();

  m_saveOnExit = false;
  diagnostic::clearSettings(true);
}

bool MainWindow::saveServerConfig()
{
  QString fileName = QFileDialog::getSaveFileName(this, tr("Save server configuration as..."));

  if (!fileName.isEmpty() && !m_serverConfig.save(fileName)) {
    QMessageBox::warning(this, tr("Save failed"), tr("Could not save server configuration to file."));
    return true;
  }

  return false;
}

void MainWindow::openAboutDialog()
{
  AboutDialog about(this);
  about.exec();
}

void MainWindow::openGetNewVersionUrl() const
{
#ifdef DESKFLOW_UI_REVIEW_BUILD
  return;
#endif
  QDesktopServices::openUrl(QUrl(kUrlDownload));
}

void MainWindow::openSettings()
{
  openSettingsPage(0);
}

void MainWindow::openSettingsPage(int page)
{
#if defined(DESKFLOW_LAB_BUILD) && !defined(DESKFLOW_UI_REVIEW_BUILD)
  return; // Review builds cannot change login/services/permissions or trust.
#endif
  auto dialog = SettingsDialog(this, m_serverConfig);
  bool connectionSetup = false;
  connect(&dialog, &SettingsDialog::requestConnectionSetup, this, [&connectionSetup] { connectionSetup = true; });
  connect(&dialog, &SettingsDialog::requestStartupQuit, this, [this] { close(); });
  if (auto *tabs = dialog.findChild<QTabWidget *>("tabWidget"))
    tabs->setCurrentIndex(page);

  connect(&dialog, &SettingsDialog::requestRemoveAllSettings, this, &MainWindow::clearSettings, Qt::UniqueConnection);
  if (dialog.exec() == QDialog::Accepted) {
#ifndef DESKFLOW_UI_REVIEW_BUILD
    Settings::save();
    disconnect(&dialog, &SettingsDialog::requestRemoveAllSettings, nullptr, nullptr);

    applyConfig();

    if (m_coreProcess.isStarted()) {
      m_coreProcess.restart();
    }
#endif
  }
  refreshConnectionHome();
  if (connectionSetup) {
    showConnectionOptions();
    m_secondaryReturnToSettings = true;
    updateSecondaryPage();
  } else
    m_home->findChild<QPushButton *>("homeSettings")->setFocus();
}

void MainWindow::resetCore()
{
  m_checkedServers.clear();
  m_checkedClients.clear();
  m_coreProcess.restart();
}

void MainWindow::showMyFingerprint()
{
  FingerprintDialog fingerprintDialog(this, m_fingerprint);
  fingerprintDialog.exec();
}

void MainWindow::coreModeToggled(bool checked)
{
  // this method is called when rbClient or rbServer toggles
  // with both being in the same group one must be turned on if the other is turned off
  // only react to toggle on to avoid calling everything twice when the user switches modes
  if (!checked)
    return;

  Settings::CoreMode mode = Settings::CoreMode::None;

  if (ui->rbModeServer->isChecked())
    mode = Settings::CoreMode::Server;
  if (ui->rbModeClient->isChecked())
    mode = Settings::CoreMode::Client;

  qDebug() << QStringLiteral("change mode to: %1").arg(QVariant::fromValue(mode).toString());

  if (m_coreProcess.isStarted() && m_coreProcess.mode() != mode)
    m_coreProcess.stop();
  m_coreProcess.setMode(mode);

  Settings::setValue(Settings::Core::CoreMode, mode);
  Settings::save();

  if (mode == Settings::CoreMode::Server && m_connectionOptions)
    m_connectionOptions->setChecked(false);
  updateModeControls();
}

void MainWindow::updateModeControls()
{
  const auto mode = m_coreProcess.mode();
  const bool isServer = mode == Settings::CoreMode::Server;
  const bool isClient = mode == Settings::CoreMode::Client;

  m_connectionDiagnostics->setIdleServerMode(isServer);

  findChild<QPushButton *>("nearbyComputersButton")->setVisible(!isServer);
  findChild<QPushButton *>("savedDevicesButton")->setVisible(!isServer);
  m_recentServers->setVisible(!isServer);
  ui->btnConfigureServer->setVisible(isServer);
  if (m_serverDiscovery)
    m_serverDiscovery->setVisible(isServer);

  const bool expanded = !m_connectionOptions || m_connectionOptions->isChecked();
  ui->widgetModeSelection->setVisible(expanded);
  ui->serverOptions->setVisible(expanded && isServer);
  ui->clientOptions->setVisible(expanded && isClient);
  ui->lblNoMode->setVisible(expanded && !isServer && !isClient);
  toggleCanRunCore(canRunCore());

  ui->lblIpAddresses->setVisible(
      (isClient && !Settings::value(Settings::Core::Interface).toString().isEmpty()) || isServer
  );

  if (ui->lblIpAddresses->isVisible())
    updateNetworkInfo();

  if (isServer || (isClient && m_automaticRecovery && m_automaticRecovery->policy().enabled)) {
    m_networkMonitor->startMonitoring();
  } else {
    m_networkMonitor->stopMonitoring();
  }

  if (isServer || isClient)
    updateModeControlLabels();
  updateSecondaryPage();
}

void MainWindow::updateModeControlLabels()
{
  const bool isServer = m_coreProcess.mode() == CoreMode::Server;
  const bool isStarted = m_coreProcess.isStarted();

  QString startText;
  QString stopText;
  QIcon startIcon;
  QIcon stopIcon;

  if (isServer) {
    startText = tr("Start");
    stopText = tr("Stop");
    startIcon = QIcon::fromTheme(QStringLiteral("system-run"));
    stopIcon = QIcon::fromTheme(QIcon::ThemeIcon::ProcessStop);
  } else {
    startText = tr("Connect");
    stopText = tr("Disconnect");
    startIcon = QIcon::fromTheme(QStringLiteral("network-connect"));
    stopIcon = QIcon::fromTheme(QStringLiteral("network-disconnect"));
  }

  m_actionStartCore->setText(startText);
  m_actionStartCore->setIcon(startIcon);
  m_actionStopCore->setText(stopText);
  m_actionStopCore->setIcon(stopIcon);

  if (isStarted) {
    ui->btnToggleCore->setText(stopText);
    ui->btnToggleCore->setIcon(stopIcon);
  } else {
    ui->btnToggleCore->setText(startText);
    ui->btnToggleCore->setIcon(startIcon);
  }
}

void MainWindow::updateSecurityIcon(bool visible)
{
  m_statusBar->setSecurityIconVisible(visible);
  if (!visible)
    return;
  m_statusBar->setSecurityIcon(TlsUtility::isEnabled());
}

void MainWindow::updateNetworkInfo()
{
#ifdef DESKFLOW_UI_REVIEW_BUILD
  updateIpLabel({});
  return;
#endif
  const auto mode = m_coreProcess.mode();
  if (mode == CoreMode::None) {
    return;
  }

  if (mode == CoreMode::Server)
    updateIpLabel(NetworkMonitor::validAddresses());
  else
    updateIpLabel({Settings::value(Settings::Core::Interface).toString()});
}

void MainWindow::serverConnectionConfigureClient(const QString &clientName)
{
  // The existing first-connection prompt has already confirmed adding this
  // protocol name. Do not open a second, conflicting arrangement editor.
  const auto previous = m_serverConfig;
  if (m_serverConfig.isFull() || m_serverConfig.computerExists(clientName))
    return;
  m_serverConfig.addClient(clientName);
  if (!m_serverConfig.computerExists(clientName))
    return;
  if (!Settings::saveConfig([this] { m_serverConfig.commit(); })) {
    m_serverConfig = previous;
    QMessageBox::warning(
        this, tr("Could not add computer"),
        tr("The configuration could not be saved. The previous computers and layout are preserved.")
    );
    return;
  }
  m_homePages->setCurrentWidget(m_home);
  refreshDesktopWorkspace();
  if (m_coreProcess.isStarted())
    m_coreProcess.restart();
}

//////////////////////////////////////////////////////////////////////////////
// End slots
//////////////////////////////////////////////////////////////////////////////

void MainWindow::open()
{
#ifdef DESKFLOW_LAB_BUILD
  show();
  return; // No updater, autostart, notification permission or tray integration.
#endif
  if (!Settings::value(Settings::Gui::Autohide).toBool())
    showAndActivate();
  else if (deskflow::platform::isMac())
    // macOS to call hide after this function ends
    QTimer::singleShot(1, this, &MainWindow::hide);
  else
    hide();

  // if a critical error was shown just before the main window (i.e. on app
  // load), it will be hidden behind the main window. therefore we need to raise
  // it up in front of the main window.
  // HACK: because the `onShown` event happens just as the window is shown, the
  // message box has a chance of being raised under the main window. to solve
  // this we delay the error dialog raise by a split second. this seems a bit
  // hacky and fragile, so maybe there's a better approach.
  const auto kCriticalDialogDelay = 100;
  QTimer::singleShot(kCriticalDialogDelay, this, &messages::raiseCriticalDialog);

  if (!Settings::value(Settings::Gui::AutoUpdateCheck).isValid()) {
    Settings::setValue(Settings::Gui::AutoUpdateCheck, messages::showUpdateCheckOption(this));
  }

  if (Settings::value(Settings::Gui::AutoUpdateCheck).toBool()) {
    m_versionChecker.checkLatest();
  } else {
    qDebug() << "skipping check for new version, disabled";
  }

  if (m_startupMode == StartupMode::Normal && deskflow::candidate::mayResume(
                                                  Settings::value(Settings::Gui::AutoStartCore).toBool(),
                                                  Settings::value(deskflow::candidate::pausedKey).toBool()
                                              )) {
    if (ui->rbModeClient->isChecked() && ui->cbHostname->lineEdit()->text().isEmpty())
      return;
    startCore();
  }
}

void MainWindow::createMenuBar()
{
  m_menuFile->addAction(m_actionStartCore);
  m_menuFile->addAction(m_actionRestartCore);
  m_menuFile->addAction(m_actionStopCore);
  m_menuFile->addSeparator();
  m_menuFile->addAction(m_actionQuit);

  m_menuEdit->addAction(m_actionSettings);

  m_menuView->addAction(m_logDock->toggleViewAction());

  m_menuHelp->addAction(m_actionAbout);
  m_menuHelp->addAction(m_actionShowHelp);

  auto menuBar = new QMenuBar(this);
  menuBar->addMenu(m_menuFile);
  menuBar->addMenu(m_menuEdit);
  menuBar->addMenu(m_menuView);
  menuBar->addMenu(m_menuHelp);

  setMenuBar(menuBar);
}

void MainWindow::setupTrayIcon()
{
  auto trayMenu = new QMenu(this);
  trayMenu->addActions(
      {m_actionStartCore, m_actionRestartCore, m_actionStopCore, m_actionMinimize, m_actionRestore, m_actionTrayQuit}
  );
  trayMenu->insertSeparator(m_actionMinimize);
  trayMenu->insertSeparator(m_actionTrayQuit);
  m_trayIcon->setContextMenu(trayMenu);

  setTrayIcon();
  m_trayIcon->show();
}

void MainWindow::applyConfig()
{
  if (Settings::value(Settings::Gui::ShowVersionInTitle).toBool()) {
    setWindowTitle(QStringLiteral("%1 - %2").arg(kAppName, kDisplayVersion));
  } else {
    setWindowTitle(kAppName);
  }

  const auto host = Settings::value(Settings::Client::RemoteHost).toString();
  QSignalBlocker hostnameBlocker(ui->cbHostname);
  ui->cbHostname->clear();
  ui->cbHostname->addItems(Settings::value(Settings::Client::RecentRemoteHosts).toStringList());
  if (!host.isEmpty())
    ui->cbHostname->setCurrentText(host);

  updateFingerprintButton();
  setTrayIcon();

  if (const auto ip = Settings::value(Settings::Core::Interface).toString(); !ip.isEmpty()) {
    m_serverStartIPs = {ip};
    m_serverStartSuggestedIP = ip;
  }

  coreModeToggled(true);
}

void MainWindow::saveSettings() const
{
  if (ui->rbModeClient->isChecked()) {
    Settings::setValue(Settings::Core::CoreMode, Settings::CoreMode::Client);
  } else if (ui->rbModeServer->isChecked()) {
    Settings::setValue(Settings::Core::CoreMode, Settings::CoreMode::Server);
  }
  if (!ui->cbHostname->currentText().isEmpty()) {
    Settings::setValue(Settings::Client::RemoteHost, ui->cbHostname->currentText());
  }
  Settings::save();
}

void MainWindow::setTrayIcon()
{
  static const auto fallbackPath = QStringLiteral(":/icons/%1-%2/apps/64/%3.svg");

  // Runtime namespaces are deliberately separate; bundled artwork retains
  // its resource identity, including in Candidate and inert Lab builds.
  QString themeIcon = QStringLiteral("org.deskflow.deskflow");
  if (!Settings::value(Settings::Gui::SymbolicTrayIcon).toBool()) {
    if (deskflow::platform::isMac())
      m_trayIcon->setIcon(QIcon::fromTheme(themeIcon, QIcon(fallbackPath.arg("deskflow", iconMode(), themeIcon))));
    else
      m_trayIcon->setIcon(QIcon(fallbackPath.arg("deskflow", QStringLiteral("dark"), themeIcon)));
    return;
  }

  themeIcon.append(QStringLiteral("-symbolic"));

  if (deskflow::platform::isWindows()) {
    QSettings settings(
        QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize"),
        QSettings::NativeFormat
    );
    const QString theme = settings.value(QStringLiteral("SystemUsesLightTheme"), 1).toBool() ? QStringLiteral("light")
                                                                                             : QStringLiteral("dark");
    m_trayIcon->setIcon(QIcon(fallbackPath.arg("deskflow", theme, themeIcon)));
    return;
  }

  auto icon = QIcon::fromTheme(themeIcon, QIcon(fallbackPath.arg("deskflow", iconMode(), themeIcon)));
  icon.setIsMask(true);
  m_trayIcon->setIcon(icon);
}

void MainWindow::handleLogLine(const QString &line)
{
  m_logDock->appendLine(line);
}

void MainWindow::handleUnrecognisedClient(const QString &clientName)
{
  if (m_ignoredClients.contains(clientName)) {
    qDebug("ignoring %s:", qPrintable(clientName));
    return;
  }

  if (m_newClientPromptShowing || m_serverConfigDialogVisible)
    return;

  if (Settings::value(Settings::Server::ExternalConfig).toBool())
    return;

  if (m_serverConfig.isFull() || m_serverConfig.computerExists(clientName))
    return;

  m_newClientPromptShowing = true;

  showAndActivate();

  if (deskflow::gui::messages::showNewClientPrompt(this, clientName)) {
    serverConnectionConfigureClient(clientName);
  } else {
    m_ignoredClients.insert(clientName);
  }

  m_newClientPromptShowing = false;
}

void MainWindow::handleConnectionRefused(deskflow::core::ConnectionRefusal reason)
{
  if (reason != deskflow::core::ConnectionRefusal::AlreadyConnected)
    return;

  if (!isVisible() || m_clientErrorVisible)
    return;

  m_clientErrorVisible = true;
  showAndActivate();

  const auto address = Settings::value(Settings::Client::RemoteHost).toString();
  QMessageBox::warning(
      this, tr("%1 Connection Error").arg(kAppName),
      tr("<p>Failed to connect to the server '%1'.</p>"
         "<p>A Client with your name is already connected to the server.</p>"
         "Please ensure that you're using a unique name and that only a "
         "single instance of the client process is running.</p>")
          .arg(address)
  );

  m_clientErrorVisible = false;
}

void MainWindow::handleMissingKeyboardLayouts(const QString &layouts)
{
  if (Settings::value(Settings::Gui::IgnoreMissingKeyboardLayouts).toBool())
    return;

  QMessageBox msgBox(this);
  msgBox.setIcon(QMessageBox::Warning);
  msgBox.setWindowTitle(tr("Missing Keyboard Layouts"));
  msgBox.setText(tr("<p>Keyboard layout support requires matching layouts on all computers. "
                    "The following layouts from the other computer are not installed on this computer:</p>"
                    "<p><b>%1</b></p>"
                    "<p>Please install them to enable support for these layouts.</p>")
                     .arg(layouts));

  auto *checkBox = new deskflow::gui::OptionCheckBox(tr("Don't show this again"), &msgBox);
  msgBox.setCheckBox(checkBox);
  msgBox.exec();

  if (checkBox->isChecked()) {
    Settings::setValue(Settings::Gui::IgnoreMissingKeyboardLayouts, true);
  }
}

void MainWindow::handlePeerFingerprint(const QString &fingerprint)
{
  // A known device's changed certificate must never flow into "trust new".
  if (!m_coreProcess.expectedPeerFingerprint().isEmpty() &&
      deskflow::auth::fingerprint(fingerprint) != m_coreProcess.expectedPeerFingerprint()) {
    m_connectionDiagnostics->requireAttention(ConnectionDiagnostics::Attention::Identity);
    return;
  }
  const auto sha256Text = deskflow::auth::fingerprint(fingerprint);
  if (sha256Text.isEmpty()) {
    m_connectionDiagnostics->requireAttention(ConnectionDiagnostics::Attention::Identity);
    return;
  }
  const Fingerprint sha256 = {QCryptographicHash::Sha256, QByteArray::fromHex(sha256Text.toLatin1())};

  const bool isClient = m_coreProcess.mode() == CoreMode::Client;
  if ((isClient && m_checkedServers.contains(sha256Text)) || (!isClient && m_checkedClients.contains(sha256Text))) {
    qDebug("ignoring fingerprint, already handled");
    return;
  }

  FingerprintDatabase db;
  db.read(trustedFingerprintDatabase());

  if (db.isTrusted(sha256)) {
    qDebug("fingerprint is trusted");
    return;
  }

  m_connectionDiagnostics->requireAttention(ConnectionDiagnostics::Attention::Identity);

  if (isClient) {
    m_checkedServers.append(sha256Text);
    m_coreProcess.stop();
  } else {
    m_checkedClients.append(sha256Text);
  }

  QString reviewedLabel;
  bool accepted = false;
  const deskflow::gui::IdentityBindings::Endpoint endpoint{
      Settings::value(Settings::Client::RemoteHost).toString(), Settings::value(Settings::Core::Port).toInt()
  };
  if (isClient && deskflow::gui::IdentityBindings::validEndpoint(endpoint)) {
    deskflow::gui::IdentityBindings review;
    auto attempt = review.begin(endpoint);
    review.observe(attempt, sha256Text);
    PairingReviewDialog dialog(review, QString::fromLatin1(m_fingerprint.data.toHex()), this);
    accepted = dialog.exec() == QDialog::Accepted;
    if (accepted)
      reviewedLabel = dialog.displayName();
  } else {
    auto mode = isClient ? FingerprintDialogMode::Client : FingerprintDialogMode::Server;
    FingerprintDialog fingerprintDialog(this, m_fingerprint, mode, sha256);
    accepted = fingerprintDialog.exec() == QDialog::Accepted;
  }
  if (accepted) {
#ifdef DESKFLOW_LAB_BUILD
    // Review artifacts cannot persist trust, including through a UI callback.
    m_connectionDiagnostics->requireAttention(ConnectionDiagnostics::Attention::Identity);
    return;
#endif
    db.addTrusted(sha256);
    if (!db.write(trustedFingerprintDatabase())) {
      qCritical().noquote() << "unable to write fingerprint to db:" << trustedFingerprintDatabase();
      m_coreProcess.stop();
      return;
    }
    if (isClient) {
      if (!reviewedLabel.isEmpty()) {
        if (!m_coreProcess.setExpectedPeerFingerprint(sha256Text))
          return;
        m_pendingPairFingerprint = sha256Text;
        m_pendingPairLabel = reviewedLabel;
      }
      m_checkedServers.removeAll(sha256Text);
      m_coreProcess.start();
    } else {
      m_checkedClients.removeAll(sha256Text);
      m_connectionDiagnostics->requireAttention(ConnectionDiagnostics::Attention::None);
    }
  }
}

bool MainWindow::maybeHideToTray()
{
  if (!Settings::value(Settings::Gui::CloseToTray).toBool()) {
    return false;
  }

  if (Settings::value(Settings::Gui::CloseReminder).toBool()) {
    messages::showCloseReminder(this);
    Settings::setValue(Settings::Gui::CloseReminder, false);
  }
  Settings::setValue(Settings::Gui::WindowGeometry, geometry());
  qDebug() << "hiding to tray";
  hide();
  return true;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
  if (event->spontaneous() && maybeHideToTray()) {
    event->ignore();
    return;
  }

  if (m_saveOnExit) {
    Settings::setValue(Settings::Gui::WindowGeometry, geometry());
    Settings::setValue(
        Settings::Gui::AutoStartCore,
        m_coreProcess.connectionRequested() && !Settings::value(deskflow::candidate::pausedKey).toBool()
    );
  }
  qDebug() << "quitting application";

  // any connected dock view acitons will be triggered
  // disconnect them before accepting the event
  disconnect(m_logDock->toggleViewAction(), &QAction::toggled, nullptr, nullptr);

  event->accept();
  QApplication::quit();
}

void MainWindow::showFirstConnectedMessage()
{
#ifdef DESKFLOW_LAB_BUILD
  return; // Synthetic preview state is not a successful real connection.
#endif
  if (Settings::value(Settings::Gui::ShownFirstConnectedMessage).toBool())
    return;
  Settings::setValue(Settings::Gui::ShownFirstConnectedMessage, true);
  messages::showFirstConnectedMessage(this);
}

void MainWindow::updateStatus()
{
  using enum ProcessState;
  const auto connection = m_coreProcess.connectionState();
  const auto process = m_coreProcess.processState();
  const bool isServer = (m_coreProcess.mode() == CoreMode::Server);
  if (process == Stopped || process == Started) {
    updateNetworkInfo();
    ui->btnEditName->setVisible(process == Stopped);
  }
  m_statusBar->setStatus(connection, process, isServer);
}

void MainWindow::coreProcessStateChanged(ProcessState state)
{
  if (state != m_coreProcess.processState())
    return; // Ignore queued obsolete states.
  using enum ProcessState;
  updateStatus();
  if (state == Started) {
    qDebug() << "recording that core has started";
#ifndef DESKFLOW_LAB_BUILD
    Settings::setValue(Settings::Gui::AutoStartCore, true);
    if (m_coreProcess.mode() == CoreMode::Server &&
        !Settings::value(Settings::Gui::ShownServerFirstStartMessage).toBool()) {
      qDebug() << "starting core server for first time";
      messages::showFirstServerStartMessage(this);
      Settings::setValue(Settings::Gui::ShownServerFirstStartMessage, true);
    }
#endif
  }

  if (state == Started || state == Starting || state == RetryPending) {
    disconnect(ui->btnToggleCore, &QPushButton::clicked, m_actionStartCore, &QAction::trigger);
    connect(ui->btnToggleCore, &QPushButton::clicked, m_actionStopCore, &QAction::trigger, Qt::UniqueConnection);

    ui->btnRestartCore->setEnabled(true);
    m_actionStartCore->setVisible(false);
    m_actionRestartCore->setVisible(true);
    m_actionStopCore->setEnabled(true);

    if (state == Starting) {
      saveSettings();
    }

  } else {
    disconnect(ui->btnToggleCore, &QPushButton::clicked, m_actionStopCore, &QAction::trigger);
    connect(ui->btnToggleCore, &QPushButton::clicked, m_actionStartCore, &QAction::trigger, Qt::UniqueConnection);

    ui->btnRestartCore->setEnabled(false);
    m_actionStartCore->setVisible(true);
    m_actionRestartCore->setVisible(false);
    m_actionStopCore->setEnabled(false);
  }
  updateModeControlLabels();
}

void MainWindow::coreConnectionStateChanged(ConnectionState state)
{
  qDebug() << "core connection state changed:" << static_cast<int>(state);

  updateStatus();
  if (state == ConnectionState::Connected && m_connectionOptions)
    m_connectionOptions->setChecked(false);

  // always assume connection is not secure when connection changes
  // to anything except connected. the only way the padlock shows is
  // when the correct TLS version string is detected.
  if (state != ConnectionState::Connected) {
    secureSocket(false);
  } else {
    if (m_coreProcess.mode() == CoreMode::Client) {
      if (const auto host = ui->cbHostname->currentText(); !host.isEmpty()) {
        QStringList recentHosts = Settings::value(Settings::Client::RecentRemoteHosts).toStringList();
        if (recentHosts.isEmpty() || recentHosts.first() != host) {
          recentHosts.removeAll(host);
          recentHosts.prepend(host);
          while (recentHosts.size() > 10) {
            recentHosts.removeLast();
          }
          Settings::setValue(Settings::Client::RecentRemoteHosts, recentHosts);
          QSignalBlocker hostnameBlocker(ui->cbHostname);
          ui->cbHostname->clear();
          ui->cbHostname->addItems(recentHosts);
          ui->cbHostname->setCurrentText(host);
        }
      }
    }
    if (isVisible()) {
      showFirstConnectedMessage();
    }
  }
}

void MainWindow::updateFingerprintButton()
{
  m_statusBar->setBtnFingerprintVisible(TlsUtility::isEnabled() && !m_fingerprint.data.isEmpty());
}

void MainWindow::hide()
{
#ifdef Q_OS_MACOS
  macOSNativeHide();
#else
  QMainWindow::hide();
#endif
  m_actionRestore->setVisible(true);
  m_actionMinimize->setVisible(false);
}

void MainWindow::changeEvent(QEvent *e)
{
  QMainWindow::changeEvent(e);
  if (e->type() == QEvent::PaletteChange) {
    deskflow::gui::applyExperienceStyle(this);
    updateIconTheme();
    setWindowIcon(QIcon::fromTheme(kRevFqdnName));
    setTrayIcon();
  } else if (e->type() == QEvent::LanguageChange) {
    ui->retranslateUi(this);
    updateModeControlLabels();
    updateNetworkInfo();
    updateStatus();
    serverClientsChanged({});
    updateText();
  }
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
  if (obj != ui->lineEditName || event->type() != QEvent::KeyPress)
    return false;
  if (const auto keyEvent = static_cast<QKeyEvent *>(event); keyEvent->key() != Qt::Key_Escape)
    return false;
  ui->lineEditName->hide();
  ui->lblComputerName->show();
  ui->btnEditName->show();
  ui->lineEditName->setText(Settings::value(Settings::Core::ComputerName).toString());
  toggleCanRunCore(canRunCore());
  return true;
}

void MainWindow::updateText()
{
  m_menuFile->setTitle(tr("&File"));
  m_menuEdit->setTitle(tr("&Edit"));
  m_menuView->setTitle(tr("&View"));
  m_menuHelp->setTitle(tr("&Help"));

  m_actionMinimize->setText(tr("&Minimize to tray"));
  m_actionQuit->setText(tr("&Quit"));
  m_actionTrayQuit->setText(tr("&Quit"));
  //: %1 will be the replaced with the appname
  m_actionRestore->setText(tr("&Open %1").arg(kAppName));
  m_actionSettings->setText(tr("&Preferences"));
  m_actionStartCore->setText(tr("&Start"));
  m_actionRestartCore->setText(tr("Rest&art"));
  m_actionStopCore->setText(tr("S&top"));
  //: %1 will be the replaced with the appname
  m_actionAbout->setText(tr("About %1...").arg(kAppName));

  m_actionShowHelp->setText(tr("View &Help"));

  //: start / restart core shortcut
  m_actionStartCore->setShortcut(QKeySequence(tr("Ctrl+S")));
  m_actionRestartCore->setShortcut(QKeySequence(tr("Ctrl+S")));

  //: stop core shortcut
  m_actionStopCore->setShortcut(QKeySequence(tr("Ctrl+T")));

  if (deskflow::platform::isWindows()) {
    //: Quit shortcut
    m_actionQuit->setShortcut(QKeySequence(tr("Ctrl+Q")));
    m_actionTrayQuit->setShortcut(QKeySequence(tr("Ctrl+Q")));
  }
}

void MainWindow::showConfigureServer(const QString &message)
{
  Q_UNUSED(message);
  showDesktopArrangement();
}

void MainWindow::showConfigureClient()
{
  ClientConfigDialog dialog(this);
  if (dialog.exec() == QDialog::Accepted) {
    applyConfig();
    if (m_coreProcess.isStarted()) {
      m_coreProcess.restart();
    }
  }
}

void MainWindow::secureSocket(bool secureSocket)
{
  m_secureSocket = secureSocket;
  updateSecurityIcon(m_statusBar->securityIconVisible());
}

void MainWindow::updateComputerName()
{
  const auto computerName = Settings::value(Settings::Core::ComputerName).toString();
  ui->lblComputerName->setText(computerName);
  ui->lineEditName->setText(computerName);
  m_serverConfig.updateServerName();
}

void MainWindow::showAndActivate()
{
  const auto wasVisible = isVisible();
#ifdef Q_OS_MACOS
  forceAppActive();
#endif
  showNormal();
  raise();
  activateWindow();
  m_actionRestore->setVisible(false);
  m_actionMinimize->setVisible(true);
  if (!wasVisible)
    restoreWindow();
}

void MainWindow::showHostNameEditor()
{
  ui->lineEditName->show();
  ui->lblComputerName->hide();
  ui->btnEditName->hide();
  toggleCanRunCore(false);
  ui->lineEditName->setFocus();
}

void MainWindow::setHostName()
{
  ui->lineEditName->hide();
  ui->lblComputerName->show();
  ui->btnEditName->show();
  toggleCanRunCore(canRunCore());

  QString text = ui->lineEditName->text();
  const auto computerName = Settings::value(Settings::Core::ComputerName).toString();

  if (text == computerName)
    return;

  const bool isServer = ui->rbModeServer->isChecked();
  bool existingComputer = false;
  if (isServer)
    existingComputer = serverConfig().computerExists(text);

  if (!ui->lineEditName->hasAcceptableInput() || text.isEmpty() || existingComputer) {
    blockSignals(true);
    ui->lineEditName->setText(computerName);
    blockSignals(false);

    const auto title = tr("Invalid Computer Name");
    QString body;
    if (existingComputer) {
      body = tr("Computer name already exists");
    } else {
      body =
          tr("The name you have chosen is invalid.\n\n"
             "Valid names:\n"
             "• Use letters and numbers\n"
             "• May also use _ or -\n"
             "• Are between 1 and 255 characters");
    }
    QMessageBox::information(this, title, body);
    return;
  }

  ui->lblComputerName->setText(ui->lineEditName->text());
  Settings::setValue(Settings::Core::ComputerName, ui->lineEditName->text());
  if (isServer)
    serverConfig().updateServerName();
  applyConfig();
}

QString MainWindow::trustedFingerprintDatabase() const
{
  const bool isClient = m_coreProcess.mode() == CoreMode::Client;
  return isClient ? Settings::tlsTrustedServersDb() : Settings::tlsTrustedClientsDb();
}

bool MainWindow::generateCertificate()
{
  const auto certificate = Settings::value(Settings::Security::Certificate).toString();
  if (!QFile::exists(certificate) && !TlsUtility::generateCertificate()) {
    return false;
  }

  m_fingerprint = {QCryptographicHash::Sha256, TlsUtility::certFingerprint()};
  updateFingerprintButton();
  return true;
}

void MainWindow::serverClientsChanged(const QStringList &clients)
{
  if (m_coreProcess.mode() != CoreMode::Server || !m_coreProcess.isStarted())
    return;
  m_statusBar->setServerClients(clients);
  m_connectedClientNames = clients;
  refreshConnectionHome();
}

void MainWindow::daemonIpcClientConnectionFailed()
{
  if (deskflow::gui::messages::showDaemonOffline(this)) {
    m_coreProcess.retryDaemon();
  }
}

void MainWindow::toggleCanRunCore(bool enableButtons)
{
  const bool isStarted = m_coreProcess.isStarted();
  ui->btnToggleCore->setEnabled(enableButtons);
  m_connectionDiagnostics->setReconnectAllowed(enableButtons);
  ui->btnRestartCore->setEnabled(enableButtons && isStarted);
  m_actionStartCore->setEnabled(enableButtons);
  m_actionStopCore->setEnabled(enableButtons && isStarted);
  updateQuickReconnectControls();
}

bool MainWindow::canQuickReconnect() const
{
  const auto state = m_coreProcess.processState();
  return m_coreProcess.mode() == CoreMode::Client && ui->lineEditName->isHidden() &&
         (Settings::value(Settings::Core::ProcessMode).value<Settings::ProcessMode>() ==
              Settings::ProcessMode::Desktop ||
          deskflow::candidate::settingsPath(Settings::settingsFile())) &&
         TlsUtility::isEnabled() && Settings::value(Settings::Security::CheckPeers).toBool() &&
         state != ProcessState::Starting && state != ProcessState::Stopping;
}

void MainWindow::updateQuickReconnectControls()
{
  QString reason;
  if (!TlsUtility::isEnabled() || !Settings::value(Settings::Security::CheckPeers).toBool())
    reason = tr("Quick reconnect requires TLS and peer fingerprint checks to be enabled in Preferences.");
  else if (Settings::value(Settings::Core::ProcessMode).value<Settings::ProcessMode>() !=
               Settings::ProcessMode::Desktop &&
           !deskflow::candidate::settingsPath(Settings::settingsFile()))
    reason = tr("Quick reconnect is currently available in desktop client mode.");
  else if (m_coreProcess.mode() != CoreMode::Client)
    reason = tr("Choose to use the other computer’s keyboard and mouse first.");
  else
    reason = tr("Finish the current change before reconnecting.");
  m_recentServers->setConnectionAllowed(canQuickReconnect(), reason);
}

void MainWindow::quickReconnect(const QString &address, int port)
{
  // Recheck at activation time. History never changes TLS settings or the trust database.
  if (!canQuickReconnect())
    return;
  ui->cbHostname->lineEdit()->setText(address);
  Settings::setValue(Settings::Core::Port, port);
  Settings::save(false);
  if (m_coreProcess.processState() == ProcessState::Stopped)
    startCore();
  else
    resetCore();
}

void MainWindow::remoteHostChanged(const QString &newRemoteHost)
{
  m_coreProcess.setAddress(newRemoteHost);
  toggleCanRunCore(!newRemoteHost.isEmpty() && ui->rbModeClient->isChecked());
  if (newRemoteHost.isEmpty()) {
    Settings::setValue(Settings::Client::RemoteHost);
  } else {
    Settings::setValue(Settings::Client::RemoteHost, newRemoteHost);
  }
}

void MainWindow::updateIpLabel(const QStringList &addresses)
{
#ifdef DESKFLOW_LAB_BUILD
  ui->lblIpAddresses->setText(tr("Preview address: 127.0.0.1"));
  ui->lblIpAddresses->setToolTip(QString());
  return; // Synthetic states have no actual interface or listening socket.
#endif
  const auto mode = m_coreProcess.mode();
  if (mode == CoreMode::None) {
    return;
  }

  static const auto colorText = QStringLiteral(R"(<span style="color:%1;">%2</span>)");
  const bool serverStarted = m_coreProcess.isStarted();
  const bool fixedIP = !Settings::value(Settings::Core::Interface).toString().isEmpty();

  if (!fixedIP && addresses.isEmpty() && !serverStarted || (serverStarted && m_serverStartSuggestedIP.isEmpty())) {
    ui->lblIpAddresses->setText(colorText.arg(palette().linkVisited().color().name(), tr("No IP Detected")));
    ui->lblIpAddresses->setToolTip(tr("Unable to detect an IP address. Check your network connection is active."));
    return;
  }

  QString labelText = fixedIP ? tr("Using IP: ") : tr("Suggested IP: ");
  QString toolTipText = tr("<p>If connecting via the hostname fails, try %1</p>");

  const bool filterIpList = (serverStarted || fixedIP);
  const QRegularExpression ipListFilter(filterIpList ? QStringLiteral("(%1)").arg(m_serverStartIPs.join("|")) : "");
  const QStringList ipList = addresses.filter(ipListFilter);

  bool IPValid = true;
  if (filterIpList && (m_serverStartSuggestedIP != m_currentIpAddress) || !ipList.contains(m_serverStartSuggestedIP)) {
    IPValid = !ipList.isEmpty();
  }

  if (IPValid) {
    m_currentIpAddress = ipList.first();
    labelText.append(m_currentIpAddress);
  } else {
    labelText.append(colorText.arg(palette().linkVisited().color().name(), m_serverStartSuggestedIP));
    toolTipText.append(tr("\nA bound IP is now invalid, you may need to restart the server."));
  }

  if (ipList.count() < 2 || fixedIP) {
    toolTipText = toolTipText.arg(tr("the suggested IP."));
  } else {
    toolTipText = toolTipText.arg(tr("one of the following IPs:<br/>%1").arg(ipList.join("<br/>")));
  }

  ui->lblIpAddresses->setText(labelText);
  ui->lblIpAddresses->setToolTip(mode == CoreMode::Server ? toolTipText : QString());
}

void MainWindow::updateTimeoutDelay(int newDelay)
{
  m_statusBar->setConnectionInterval(newDelay);
}

void MainWindow::setHelpFilePath()
{
  const QString appPath = QCoreApplication::applicationDirPath();
  auto buildPath = QString("%1/../docs/HelpMain.md").arg(appPath);
  auto installPath = QString("%1/../share/doc/%2/HelpMain.md").arg(appPath, kAppId);
  if (deskflow::platform::isMac()) {
    installPath = QString("%1/../Resources/docs/HelpMain.md").arg(appPath);
    buildPath = QString("%1/../../../../docs/HelpMain.md").arg(appPath);
  } else if (deskflow::platform::isWindows()) {
    installPath = QString("%1/docs/HelpMain.md").arg(appPath);
  }

  buildPath = QDir::cleanPath(buildPath);
  installPath = QDir::cleanPath(installPath);

  if (QFile::exists(installPath))
    m_helpPath = QUrl::fromLocalFile(installPath);
  else if (QFile::exists(buildPath))
    m_helpPath = QUrl::fromLocalFile(buildPath);
  else
    m_helpPath = kUrlWiki;
}

void MainWindow::showHelpViewer() const
{
#ifdef DESKFLOW_UI_REVIEW_BUILD
  return;
#endif
  if (m_helpPath.isLocalFile()) {
    HelpDialog dialogHelp(this->centralWidget(), m_helpPath);
    dialogHelp.exec();
  } else {
    QDesktopServices::openUrl(m_helpPath);
  }
}

bool MainWindow::canRunCore() const
{
#ifdef DESKFLOW_LAB_BUILD
  return false;
#endif
  const auto mode = m_coreProcess.mode();
  const bool isServer = mode == Settings::CoreMode::Server;
  const bool isClient = mode == Settings::CoreMode::Client;
  return ((isServer || isClient) && (isClient && !ui->cbHostname->currentText().isEmpty()) || isServer);
}

#ifdef DESKFLOW_LAB_BUILD
namespace {
class LabAutomaticTransport final : public deskflow::gui::DiscoveryTransport
{
public:
  LabAutomaticTransport() : DiscoveryTransport(nullptr)
  {
  }
  void browse(const deskflow::gui::DiscoveryScope &) override
  {
    Q_EMIT activeChanged(true);
    QTimer::singleShot(0, this, [this] {
      Q_EMIT resolved("candidate", "Unverified preview", "new.local", 34800, 120000);
    });
  }
  void advertise(const deskflow::gui::DiscoveryAdvertisement &) override
  {
  }
  void stop() override
  {
    Q_EMIT activeChanged(false);
  }
};
class LabAddressResolver : public deskflow::gui::ScopedAddressResolver
{
public:
  using ScopedAddressResolver::ScopedAddressResolver;
  void start(const deskflow::auth::Context &c, const deskflow::gui::DiscoveryScope &s, const QString &h, int) override
  {
    QTimer::singleShot(0, this, [this, c, s, h] { Q_EMIT addresses(c, s, h, {"192.168.50.30"}, 10000); });
  }
  void cancel() override
  {
  }
};
class LabTlsProbe : public deskflow::gui::TlsProbeExecutor
{
  bool m_accept;

public:
  explicit LabTlsProbe(bool accept) : m_accept(accept)
  {
  }
  bool ready() const override
  {
    return true;
  }
  bool selectPreparedSession(const QString &) override
  {
    return false;
  }
  void cancel() override
  {
  }
  void
  start(const deskflow::auth::Context &c, const deskflow::gui::DiscoveryScope &, const QString &address, int) override
  {
    const auto outcome = m_accept ? deskflow::auth::Outcome::Verified : deskflow::auth::Outcome::WrongIdentity;
    const auto digest = m_accept ? c.expected : QString(64, 'b');
    // Exercise exactly the parser used by the real helper's stdout. Neither
    // fixture creates a process, network query, credential or trust record.
    deskflow::gui::ProbeOutput parser(c);
    for (const auto &report :
         {deskflow::auth::Report{c, 1, deskflow::auth::Outcome::Started, {}},
          deskflow::auth::Report{c, 1, outcome, digest}})
      parser.feed(
          QJsonDocument(
              QJsonObject{{"event", "peerAuthentication"}, {"payload", report.encode()}}
          ).toJson(QJsonDocument::Compact) +
          '\n'
      );
    const auto result = parser.finish(m_accept ? 0 : 4, true);
    if (result)
      QTimer::singleShot(0, this, [this, c, address, result] { Q_EMIT completed(c, address, *result); });
  }
};
} // namespace
bool MainWindow::labScenario(const QString &name)
{
  if (!QStringList{
          "empty",
          "discovering",
          "connecting",
          "connected",
          "identity",
          "error",
          "retry",
          "long-error",
          "layout",
          "layout-invalid",
          "verifying",
          "listening",
          "stopping",
          "layout-long-name",
          "layout-special-name",
          "layout-invalid-name",
          "pairing",
          "identity-changed",
          "diagnostics",
          "displays",
          "trusted-devices",
          "authenticated-device",
          "recover-device",
          "recover-rejected",
          "recover-unavailable",
          "automatic-config",
          "automatic-recovered",
          "automatic-paused"
      }
           .contains(name))
    return false;
  if (auto *old = findChild<QLabel *>("labPreviewBanner"))
    delete old;
  auto *banner = new QLabel(tr("Isolated preview · simulated connection states · input control disabled"), this);
  banner->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
  banner->setObjectName("labPreviewBanner");
  banner->setWordWrap(true);
  banner->setTextFormat(Qt::PlainText);
  qobject_cast<QBoxLayout *>(ui->topLevelWidget->layout())->insertWidget(0, banner);
  if (name == "empty")
    return true;
  if (name == "identity-changed") {
    const QString first(64, 'a'), second(64, 'b');
    auto generation = m_identityBindings.begin({"192.168.50.20", 34800});
    m_identityBindings.observe(generation, first);
    m_identityBindings.confirm(generation, tr("Preview computer"), true);
    m_identityBindings.authenticated(generation, first, true, true);
    Settings::setValue("gui/identityBindings", m_identityBindings.encode());
    Settings::setValue(Settings::Client::RemoteHost, "192.168.50.30");
    Settings::setValue(Settings::Core::Port, 34800);
    ui->cbHostname->lineEdit()->setText("192.168.50.30");
    Q_EMIT m_coreProcess.connectionAttemptStarting(false, "192.168.50.30", 34800, true, true);
    const auto context = m_coreProcess.labBeginAuthentication(first);
    m_coreProcess.labDeliverAuthentication(
        deskflow::auth::Report{context, 1, deskflow::auth::Outcome::Started, {}}.encode()
    );
    // Exercise the real rejection/UI callback with a synthetic TLS result.
    m_coreProcess.labDeliverAuthentication(
        deskflow::auth::Report{context, 1, deskflow::auth::Outcome::WrongIdentity, second}.encode()
    );
    return true;
  }
  if (name.startsWith("automatic-")) {
    if (!m_identityBindings.load(deskflow::gui::IdentityBindings{}.encode()))
      return false;
    const QString fp(64, 'a');
    const DiscoveryScope scope{7, "Lab-LAN", QHostAddress("192.168.50.10"), 24};
    const auto g = m_identityBindings.begin({"old.local", 34800});
    m_identityBindings.observe(g, fp);
    if (!m_identityBindings.confirm(g, tr("Preview computer"), true) ||
        !m_identityBindings.authenticated(g, fp, true, true))
      return false;
    Settings::setValue("gui/identityBindings", m_identityBindings.encode());
    Settings::setValue(Settings::Client::RemoteHost, "old.local");
    Settings::setValue(Settings::Core::Port, 34800);
    ui->cbHostname->lineEdit()->setText("old.local");
    auto source = [scope] { return QVector<DiscoveryScope>{scope}; };
    if (name == "automatic-config") {
      auto *dialog = new AutomaticRecoveryDialog(fp, {true, fp, scope}, source, this);
      dialog->setAttribute(Qt::WA_DeleteOnClose);
      dialog->show();
      return true;
    }
    auto *transport = new LabAutomaticTransport;
    auto *discovery = new NearbyDiscovery(nullptr, transport);
    transport->setParent(discovery);
    auto clock = std::make_shared<qint64>(0);
    setupAutomaticRecovery(source, new LabAddressResolver, new LabTlsProbe(true), discovery, [clock] {
      return *clock;
    });
    connect(m_automaticRecovery, &AutomaticRecovery::stateChanged, this, [this](auto state) {
      Settings::setValue("lab/automaticState", int(state));
    });
    Q_EMIT m_coreProcess.connectionAttemptStarting(false, "old.local", 34800, true, true);
    m_coreProcess.labBeginAuthentication(fp);
    if (!m_automaticRecovery->configure({true, fp, scope}))
      return false;
    if (name == "automatic-paused")
      m_coreProcess.stop(Settings::ProcessMode::Desktop);
    QTimer::singleShot(20, this, [this, clock] {
      *clock = 5000;
      m_automaticRecovery->tick();
    });
    return true;
  }
  if (name.startsWith("recover-")) {
    const QString fp(64, 'a');
    if (!m_identityBindings.load(deskflow::gui::IdentityBindings{}.encode()))
      return false;
    const auto g = m_identityBindings.begin({"old.local", 34800});
    m_identityBindings.observe(g, fp);
    if (!m_identityBindings.confirm(g, tr("Preview computer"), true) ||
        !m_identityBindings.authenticated(g, fp, true, true))
      return false;
    Settings::setValue("gui/identityBindings", m_identityBindings.encode());
    Settings::setValue(Settings::Client::RemoteHost, "old.local");
    Settings::setValue(Settings::Core::Port, 34800);
    ui->cbHostname->lineEdit()->setText("old.local");
    auto *resolver = name == "recover-unavailable" ? nullptr : new LabAddressResolver;
    auto *executor = name == "recover-unavailable" ? nullptr : new LabTlsProbe(name == "recover-device");
    auto *dialog = createRecoveryDialog(
        fp, [] { return QVector<DiscoveryScope>{{7, "Lab-LAN", QHostAddress("192.168.50.10"), 24}}; }, resolver,
        executor
    );
    auto *recovery = dialog->recovery();
    dialog->findChild<QComboBox *>("nearbyNetwork")->setCurrentIndex(1);
    dialog->show();
    QTimer::singleShot(20, dialog, [dialog, recovery, fp] {
      if (!recovery->begin(fp, {7, "Lab-LAN", QHostAddress("192.168.50.10"), 24}))
        return;
      Q_EMIT dialog->findChild<NearbyDiscovery *>()->found("candidate", tr("Unverified preview"), "new.local", 34800);
    });
    return true;
  }
  if (name == "trusted-devices" || name == "authenticated-device") {
    const QString identity(64, 'a');
    auto g = m_identityBindings.begin({"192.168.50.20", 34800});
    m_identityBindings.observe(g, identity);
    m_identityBindings.confirm(g, tr("Preview computer"), true);
    m_identityBindings.authenticated(g, identity, true, true);
    if (name == "authenticated-device") {
      Settings::setValue(Settings::Client::RemoteHost, "192.168.50.30");
      Settings::setValue(Settings::Core::Port, 34800);
      const auto context = m_coreProcess.labBeginAuthentication(identity);
      m_coreProcess.labDeliverAuthentication(
          deskflow::auth::Report{context, 1, deskflow::auth::Outcome::Started, {}}.encode()
      );
      // Synthetic proof, routed through the same parser, generation gate and
      // MainWindow persistence callback as a real core event. No trust writes.
      m_coreProcess.labDeliverAuthentication(
          deskflow::auth::Report{context, 1, deskflow::auth::Outcome::Verified, identity}.encode()
      );
    }
    auto *dialog = new TrustedDevicesDialog(m_identityBindings, false, this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
    return true;
  }
  if (name == "pairing") {
    auto bindings = std::make_shared<deskflow::gui::IdentityBindings>();
    const QString second(64, 'b'), local(64, 'c');
    const auto generation = bindings->begin({"192.168.50.30", 34800});
    bindings->observe(generation, second);
    auto *dialog = new PairingReviewDialog(*bindings, local, this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    connect(dialog, &QDialog::accepted, this, [bindings, generation, second] {
      // Only this explicitly synthetic LAB scenario supplies a mock TLS result.
      if (bindings->authenticated(generation, second, true, true))
        Settings::setValue("lab/simulatedIdentityBindings", QString::fromUtf8(bindings->encode()));
    });
    connect(dialog, &QDialog::destroyed, this, [bindings] {});
    dialog->show();
    return true;
  }
  if (name == "displays") {
    const auto topology =
        deskflow::gui::MonitorTopology::inspect({{{-1920, 0, 1920, 1080}, 1}, {{0, 0, 2560, 1440}, 2}});
    auto *dialog = new DisplayTopologyDialog(topology, this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
    return true;
  }
  if (name == "diagnostics") {
    m_connectionDiagnostics->beginAttempt(false, "synthetic-private-host.local", 34800, true, true);
    m_connectionDiagnostics->processStateChanged(ProcessState::Started);
    m_connectionDiagnostics->requireAttention(ConnectionDiagnostics::Attention::Protocol);
    auto *dialog = new DiagnosticExportDialog(m_connectionDiagnostics->diagnosticSnapshot(), this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
    return true;
  }
  if (name.startsWith("layout")) {
    ui->rbModeServer->setChecked(true);
    m_networkMonitor->stopMonitoring();
    ui->lblIpAddresses->setText(tr("Preview address: 127.0.0.1"));
    m_serverConfig.addClient("Work-PC");
    if (!m_serverConfig.setDesktopLayout(
            {{"Lab-Mac", 0, 0, 1920, 1080, "这台 Mac"}, {"Work-PC", 1920, 200, 2560, 1440, "工作电脑"}}
        )) {
      banner->setText(tr("Preview layout could not be loaded. Check the protocol names and geometry."));
      return false;
    }
    auto *dialog = new ServerConfigDialog(this, m_serverConfig);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
    QTimer::singleShot(10, dialog, [dialog, name] {
      if (name.startsWith("layout-"))
        QTimer::singleShot(30, dialog, [dialog, name] {
          auto *editor = dialog->findChild<DesktopLayoutEditor *>();
          if (editor) {
            editor->findChild<QComboBox *>("layoutComputer")->setCurrentIndex(1);
            if (name == "layout-invalid")
              editor->moveDesktop("Work-PC", 960, 0, false);
            else {
              auto *display = editor->findChild<QLineEdit *>("layoutDisplayName");
              display->setText(
                  name == "layout-long-name"      ? QString(80, 'W')
                  : name == "layout-special-name" ? QString("研发 <QA> & \"设计\" 🖥️")
                                                  : QString(81, 'W')
              );
              Q_EMIT display->textEdited(display->text());
            }
          }
        });
      if (auto *edit = dialog->findChild<QPushButton *>("editDesktopLayout"))
        edit->click();
    });
    return true;
  }
  m_coreProcess.labObserve(ProcessState::Stopped, ConnectionState::Disconnected);
  if (name == "discovering") {
    m_connectionDiagnostics->setDiscoveryActive(true);
    auto *dialog = new NearbyDevicesDialog(this, [] {
      return QVector<DiscoveryScope>{{7, "Lab-LAN", QHostAddress("192.168.50.10"), 24}};
    });
    dialog->findChild<QComboBox *>("nearbyNetwork")->setCurrentIndex(1);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
    QTimer::singleShot(20, dialog, [dialog] {
      auto *provider = dialog->findChild<NearbyDiscovery *>();
      Q_EMIT provider->status(tr("Preview devices · identities have not been verified"));
      Q_EMIT provider->found("one", "工作电脑 · 办公室", "workstation.local", 34800);
      Q_EMIT provider->found("two", "家庭工作站 · 长名称用于可读性检查", "home-office.local", 34800);
    });
    QTimer::singleShot(0, this, &MainWindow::resizeToContent);
    return true;
  }
  if (name == "listening") {
    ui->rbModeServer->setChecked(true);
    m_networkMonitor->stopMonitoring();
  }
  const auto address = name == "long-error"
                           ? QString(60, 'w') + "." + QString(60, 'w') + "." + QString(60, 'w') + ".local"
                           : QString("workstation.local");
  ui->cbHostname->lineEdit()->setText(address);
  Q_EMIT m_coreProcess.connectionAttemptStarting(
      name == "listening", name == "listening" ? QString() : address, 34800, true, true
  );
  m_coreProcess.labObserve(ProcessState::Started, ConnectionState::Connecting);
  if (name == "connected") {
    Q_EMIT m_coreProcess.secureSocket(true);
    m_coreProcess.labObserve(ProcessState::Started, ConnectionState::Connected);
  } else if (name == "verifying") {
    m_coreProcess.labObserve(ProcessState::Started, ConnectionState::Connected);
  } else if (name == "listening") {
    m_coreProcess.labObserve(ProcessState::Started, ConnectionState::Listening);
  } else if (name == "stopping") {
    m_coreProcess.labObserve(ProcessState::Stopping, ConnectionState::Disconnected);
  } else if (name == "identity") {
    m_connectionDiagnostics->requireAttention(ConnectionDiagnostics::Attention::Identity);
  } else if (name == "error" || name == "long-error") {
    Q_EMIT m_coreProcess.connectionRefused(deskflow::core::ConnectionRefusal::ProtocolError);
    if (name == "long-error")
      m_connectionDiagnostics->findChild<QPushButton *>("diagnosticDetailsButton")->click();
  } else if (name == "retry") {
    m_coreProcess.labObserve(ProcessState::Started, ConnectionState::Disconnected);
    Q_EMIT m_coreProcess.retryIn(20);
  }
  QTimer::singleShot(0, this, &MainWindow::resizeToContent);
  return true;
}
#endif
