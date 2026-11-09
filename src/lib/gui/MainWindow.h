/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2024 - 2026 Chris Rizzitello <sithord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2012 - 2024 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2008 Volker Lanz <vl@fidra.de>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "core/DiscoveryScope.h"
#include <QMainWindow>
#include <QProcess>
#include <QRegularExpression>
#include <QSystemTrayIcon>
#include <QUrl>

#include "VersionChecker.h"
#include "common/DesktopWorkspace.h"
#include "config/ServerConfig.h"
#include "gui/IdentityBindings.h"
#include "gui/core/CoreProcess.h"
#include "gui/core/NetworkMonitor.h"
#include "net/Fingerprint.h"

#ifdef Q_OS_MACOS
#include "gui/OSXHelpers.h"
#endif

class QAction;
class QMenu;
class QLocalServer;
class QToolButton;
class QStackedWidget;

class DeskflowApplication;
class LogDock;
class StatusBar;
class RecentServersButton;
class ConnectionDiagnostics;
namespace deskflow::gui {
class ConnectionHome;
class AutomaticRecovery;
class PowerMonitor;
class NearbyDiscovery;
class NearbyPublisher;
class NearbyDevicesDialog;
class ScopedAddressResolver;
class TlsProbeExecutor;
} // namespace deskflow::gui

namespace Ui {
class MainWindow;
}
namespace deskflow::gui {
class DesktopWorkspaceView;
}

namespace deskflow::gui::ipc {
class DaemonIpcClient;
}

class MainWindow : public QMainWindow
{
  friend class FirstSetupFlowTests;
  friend class DesktopProductFlowTests;
  using ConnectionState = deskflow::core::ConnectionState;
  using CoreMode = Settings::CoreMode;
  using CoreProcess = deskflow::gui::CoreProcess;
  using NetworkMonitor = deskflow::gui::NetworkMonitor;
  using ProcessState = deskflow::core::ProcessState;

  Q_OBJECT

public:
  enum class LogLevel
  {
    Error,
    Info
  };

public:
  enum class StartupMode
  {
    Normal,
    PermissionReview
  };
  explicit MainWindow(StartupMode startupMode = StartupMode::Normal);
  ~MainWindow() override;

  [[nodiscard]] CoreMode coreMode() const
  {
    return m_coreProcess.mode();
  }
  void open();
#ifdef DESKFLOW_LAB_BUILD
  void labUserStop()
  {
    stopCore();
  }
  void labUserStart()
  {
    startCore();
  }
  bool labScenario(const QString &name);
#endif
  ServerConfig &serverConfig()
  {
    return m_serverConfig;
  }

  void hide();

protected:
  void changeEvent(QEvent *e) override;
  bool eventFilter(QObject *obj, QEvent *event) override;

private:
  /**
   * @brief updateText Update all text not in the UI
   */
  void updateText();
  void toggleLogVisible(bool visible);

  void settingsChanged(const QString &key = QString());
  void serverConfigSaving();
  void coreProcessError(CoreProcess::Error error);
  void coreConnectionStateChanged(ConnectionState state);
  void coreProcessStateChanged(ProcessState state);

  void trayIconActivated(QSystemTrayIcon::ActivationReason reason);
  void serverConnectionConfigureClient(const QString &clientName);

  void clearSettings();
  void openAboutDialog();
  void openGetNewVersionUrl() const;
  void openSettings();
  void openSettingsPage(int page);
  void startCore();
  void stopCore();
  bool saveServerConfig();
  void resetCore();

  void showMyFingerprint();
  void updateSecurityIcon(bool visible);
  void updateNetworkInfo();

  void coreModeToggled(bool checked);
  void updateModeControls();
  void updateModeControlLabels();
  const StartupMode m_startupMode;
  std::unique_ptr<Ui::MainWindow> ui;

  void createMenuBar();
  void setupTrayIcon();
  void applyConfig();
  void setTrayIcon();
  void handleUnrecognisedClient(const QString &clientName);
  void handleConnectionRefused(deskflow::core::ConnectionRefusal reason);
  void handlePeerFingerprint(const QString &fingerprint);
  void handleMissingKeyboardLayouts(const QString &layouts);
  void closeEvent(QCloseEvent *event) override;
  bool maybeHideToTray();
  void secureSocket(bool secureSocket);
  void connectSlots();
  void handleLogLine(const QString &line);
  void updateFingerprintButton();
  void updateComputerName();
  void saveSettings() const;
  void showConfigureServer(const QString &message);
  void showConfigureClient();
  void restoreWindow();
  void resizeToContent();
  void setupControls();
  void setupConnectionHome();
  void setupDesktopWorkspace();
  void refreshDesktopWorkspace();
  void saveDesktopWorkspace(const QByteArray &state, const QByteArray &layout, bool apply);
  void showDesktopArrangement();
  void showAdvancedServerSettings();
  void refreshConnectionHome();
  void showConnectionOptions(bool diagnostics = false);
  void showAddComputer();
  void updateSecondaryPage();
  void setupAutomaticRecovery(
      deskflow::gui::DiscoveryScopeSource source = deskflow::gui::DiscoveryScope::current,
      deskflow::gui::ScopedAddressResolver *resolver = nullptr, deskflow::gui::TlsProbeExecutor *executor = nullptr,
      deskflow::gui::NearbyDiscovery *discovery = nullptr, std::function<qint64()> clock = {}
  );
  deskflow::gui::NearbyDevicesDialog *createRecoveryDialog(
      const QString &fp, deskflow::gui::DiscoveryScopeSource source,
      deskflow::gui::ScopedAddressResolver *resolver = nullptr, deskflow::gui::TlsProbeExecutor *executor = nullptr
  );
  void showFirstConnectedMessage();
  void updateStatus();
  void showAndActivate();
  void showHostNameEditor();
  void setHostName();
  void daemonIpcClientConnectionFailed();
  void toggleCanRunCore(bool enableButtons);
  void remoteHostChanged(const QString &newRemoteHost);
  bool canQuickReconnect() const;
  void updateQuickReconnectControls();
  void quickReconnect(const QString &address, int port);
  void updateIpLabel(const QStringList &addresses);
  void updateTimeoutDelay(int newDelay);
  void setHelpFilePath();
  void showHelpViewer() const;

  bool canRunCore() const;

  /**
   * @brief trustedFingerprintDatabase get the FingerprintDatabase for the trusted clients or trusted servers.
   * @return The path to the trusted fingerprint file
   */
  QString trustedFingerprintDatabase() const;

  /**
   * @brief generateCertificate Generate a new certificate
   * @return true when successful
   */
  bool generateCertificate();

  Fingerprint m_fingerprint;

  void serverClientsChanged(const QStringList &clients);

  inline static const auto m_guiSocketName = QStringLiteral("%1-gui").arg(kAppId);
  inline static const auto m_nameRegEx = QRegularExpression(QStringLiteral("^[\\w\\-_\\.]{0,255}$"));

  VersionChecker m_versionChecker;
  bool m_secureSocket = false;
  bool m_saveOnExit = true;
  bool m_clientErrorVisible = false;
  ServerConfig m_serverConfig;
  deskflow::gui::CoreProcess m_coreProcess;
  deskflow::gui::IdentityBindings m_identityBindings;
  QString m_pendingPairFingerprint, m_pendingPairLabel;
  QStringList m_connectedClientNames;
  QString m_recoveryPreparationError;
  QSet<QString> m_ignoredClients;
  bool m_newClientPromptShowing = false;
  bool m_serverConfigDialogVisible = false;
  QSize m_expandedSize = QSize();
  QStringList m_checkedClients;
  QStringList m_checkedServers;
  QSystemTrayIcon *m_trayIcon = nullptr;
  QLocalServer *m_guiDupeChecker = nullptr;
  deskflow::gui::ipc::DaemonIpcClient *m_daemonIpcClient = nullptr;

  LogDock *m_logDock;
  StatusBar *m_statusBar = nullptr;
  RecentServersButton *m_recentServers = nullptr;
  ConnectionDiagnostics *m_connectionDiagnostics = nullptr;
  deskflow::gui::AutomaticRecovery *m_automaticRecovery = nullptr;
  deskflow::gui::TlsProbeExecutor *m_automaticProbe = nullptr;
  deskflow::gui::PowerMonitor *m_powerMonitor = nullptr;
  deskflow::gui::NearbyPublisher *m_nearbyPublisher = nullptr;
  QToolButton *m_connectionOptions = nullptr;
  QWidget *m_deviceActions = nullptr;
  deskflow::gui::ConnectionHome *m_home = nullptr;
  deskflow::gui::DesktopWorkspaceView *m_desktopWorkspace = nullptr;
  deskflow::display::Topology m_localWorkspaceTopology;
  std::optional<deskflow::display::WorkspaceSnapshot> m_workspaceSnapshot;
  QByteArray m_loadedWorkspace, m_loadedWorkspaceLayout;
  QString m_workspaceController, m_workspaceSourceFingerprint, m_workspaceSaveStatus;
  QString m_workspacePendingRequest;
  QByteArray m_workspacePendingState;
  CoreMode m_workspaceRole = CoreMode::None;
  bool m_workspaceLive = false, m_workspaceSaving = false;
  QStackedWidget *m_homePages = nullptr;
  QWidget *m_connectionOptionsPage = nullptr;
  bool m_secondaryDiagnostics = false, m_secondaryAddComputer = false, m_secondaryReturnToSettings = false;
  QWidget *m_serverDiscovery = nullptr;

  // Window Menu
  QMenu *m_menuFile = nullptr;
  QMenu *m_menuEdit = nullptr;
  QMenu *m_menuView = nullptr;
  QMenu *m_menuHelp = nullptr;

  // Window Actions
  QAction *m_actionAbout = nullptr;
  QAction *m_actionMinimize = nullptr;
  QAction *m_actionQuit = nullptr;
  QAction *m_actionTrayQuit = nullptr;
  QAction *m_actionRestore = nullptr;
  QAction *m_actionSettings = nullptr;
  QAction *m_actionStartCore = nullptr;
  QAction *m_actionRestartCore = nullptr;
  QAction *m_actionStopCore = nullptr;
  QAction *m_actionShowHelp = nullptr;

  // Network monitoring
  NetworkMonitor *m_networkMonitor = nullptr;
  QString m_currentIpAddress;

  // Server IP strategy optimization
  QStringList m_serverStartIPs;
  QString m_serverStartSuggestedIP;
  QUrl m_helpPath;
};
