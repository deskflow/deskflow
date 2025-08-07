/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Chris Rizzitello <sithord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2012 - 2024 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2008 Volker Lanz <vl@fidra.de>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QMainWindow>
#include <QMutex>
#include <QProcess>
#include <QRegularExpression>
#include <QSettings>
#include <QSystemTrayIcon>
#include <QThread>

#include "ServerConfig.h"
#include "VersionChecker.h"
#include "gui/core/ClientConnection.h"
#include "gui/core/CoreProcess.h"
#include "gui/core/ServerConnection.h"
#include "gui/core/WaylandWarnings.h"
#include "gui/tls/TlsUtility.h"

#ifdef Q_OS_MAC
#include "gui/OSXHelpers.h"
#endif

class QAction;
class QMenu;
class QLabel;
class QLineEdit;
class QGroupBox;
class QPushButton;
class QTextEdit;
class QComboBox;
class QTabWidget;
class QToolButton;
class QCheckBox;
class QRadioButton;
class QMessageBox;
class QAbstractButton;
class QLocalServer;

class DeskflowApplication;

namespace Ui {
class MainWindow;
}

namespace deskflow::gui::ipc {
class DaemonIpcClient;
}

class MainWindow : public QMainWindow
{
  using CoreMode = Settings::CoreMode;
  using CoreProcess = deskflow::gui::CoreProcess;

  Q_OBJECT

  friend class DeskflowApplication;
  friend class SettingsDialog;

public:
  enum class LogLevel
  {
    Error,
    Info
  };

public:
  explicit MainWindow();
  ~MainWindow() override;

  [[nodiscard]] CoreMode coreMode() const
  {
    return m_coreProcess.mode();
  }
  void open();
  ServerConfig &serverConfig()
  {
    return m_serverConfig;
  }
  void autoAddScreen(const QString &name);

  void hide();

private:
  void toggleLogVisible(bool visible);

  void settingsChanged(const QString &key = QString());
  void serverConfigSaving();
  void coreProcessStarting();
  void coreProcessError(CoreProcess::Error error);
  void coreConnectionStateChanged(CoreProcess::ConnectionState state);
  void coreProcessStateChanged(CoreProcess::ProcessState state);
  void versionCheckerUpdateFound(const QString &version);
  void trayIconActivated(QSystemTrayIcon::ActivationReason reason);
  void serverConnectionConfigureClient(const QString &clientName);

  void clearSettings();
  void openAboutDialog();
  void openHelpUrl() const;
  void openGetNewVersionUrl() const;
  void openSettings();
  void startCore();
  void stopCore();
  bool saveServerConfig();
  void resetCore();

  void showMyFingerprint();
  void updateSecurityIcon(bool visible);
  void updateNetworkInfo();

  void coreModeToggled();
  void updateModeControls(bool serverMode);

  std::unique_ptr<Ui::MainWindow> ui;

  void updateSize();
  void createMenuBar();
  void setupTrayIcon();
  void applyConfig();
  void setIcon();
  void setStatus(const QString &status);
  void updateFromLogLine(const QString &line);
  void checkConnected(const QString &line);
  void checkFingerprint(const QString &line);
  [[nodiscard]] QString getTimeStamp() const;
  void closeEvent(QCloseEvent *event) override;
  void secureSocket(bool secureSocket);
  void connectSlots();
  void handleLogLine(const QString &line);
  void updateLocalFingerprint();
  void updateScreenName();
  void saveSettings() const;
  void showConfigureServer(const QString &message);
  void restoreWindow();
  void setupControls();
  void showFirstConnectedMessage();
  void updateStatus();
  void showAndActivate();
  void showHostNameEditor();
  void setHostName();
  void daemonIpcClientConnectionFailed();
  void toggleCanRunCore(bool enableButtons);
  void remoteHostChanged(const QString &newRemoteHost);

  /**
   * @brief trustedFingerprintDatabase get the FingerprintDatabase for the trusted clients or trusted servers.
   * @return The path to the trusted fingerprint file
   */
  QString trustedFingerprintDatabase() const;

  // Generate prints if they are missing
  // Returns true if successful
  bool regenerateLocalFingerprints();

  void serverClientsChanged(const QStringList &clients);

  inline static const auto m_guiSocketName = QStringLiteral("deskflow-gui");
  inline static const auto m_nameRegEx = QRegularExpression(QStringLiteral("^[\\w\\-_\\.]{0,255}$"));

  VersionChecker m_versionChecker;
  bool m_secureSocket = false;
  bool m_saveOnExit = true;
  deskflow::gui::core::WaylandWarnings m_waylandWarnings;
  ServerConfig m_serverConfig;
  deskflow::gui::CoreProcess m_coreProcess;
  deskflow::gui::ServerConnection m_serverConnection;
  deskflow::gui::ClientConnection m_clientConnection;
  deskflow::gui::TlsUtility m_tlsUtility;
  QSize m_expandedSize = QSize();
  QStringList m_checkedClients;
  QStringList m_checkedServers;
  QSystemTrayIcon *m_trayIcon = nullptr;
  QLocalServer *m_guiDupeChecker = nullptr;
  deskflow::gui::ipc::DaemonIpcClient *m_daemonIpcClient = nullptr;

  QLabel *m_lblSecurityStatus = nullptr;
  QLabel *m_lblStatus = nullptr;
  QToolButton *m_btnFingerprint = nullptr;
  QPushButton *m_btnUpdate = nullptr;

  // Window Actions
  QAction *m_actionAbout = nullptr;
  QAction *m_actionClearSettings = nullptr;
  QAction *m_actionReportBug = nullptr;
  QAction *m_actionMinimize = nullptr;
  QAction *m_actionQuit = nullptr;
  QAction *m_actionTrayQuit = nullptr;
  QAction *m_actionRestore = nullptr;
  QAction *m_actionSettings = nullptr;
  QAction *m_actionStartCore = nullptr;
  QAction *m_actionRestartCore = nullptr;
  QAction *m_actionStopCore = nullptr;
};
