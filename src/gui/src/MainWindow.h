/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
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

#pragma once

#include <QMainWindow>
#include <QMutex>
#include <QProcess>
#include <QSettings>
#include <QSystemTrayIcon>
#include <QThread>

#include "ActivationDialog.h"
#include "ServerConfig.h"
#include "global/Ipc.h"
#include "gui/AppConfig.h"
#include "gui/ClientConnection.h"
#include "gui/ConfigScopes.h"
#include "gui/CoreProcess.h"
#include "gui/ServerConnection.h"
#include "gui/TlsUtility.h"
#include "gui/TrayIcon.h"
#include "gui/VersionChecker.h"
#include "ui_MainWindowBase.h"

class QAction;
class QMenu;
class QLineEdit;
class QGroupBox;
class QPushButton;
class QTextEdit;
class QComboBox;
class QTabWidget;
class QCheckBox;
class QRadioButton;
class QMessageBox;
class QAbstractButton;

class QSynergyApplication;
class SetupWizard;

class MainWindow : public QMainWindow, public Ui::MainWindowBase {
  using CoreMode = synergy::gui::CoreProcess::Mode;

  Q_OBJECT

  friend class QSynergyApplication;
  friend class SetupWizard;
  friend class ActivationDialog;
  friend class SettingsDialog;

public:
  enum class LogLevel { Error, Info };

public:
  explicit MainWindow(
      synergy::gui::ConfigScopes &configScopes, AppConfig &appConfig);
  ~MainWindow() override;

  void setVisible(bool visible) override;
  CoreMode coreMode() const { return m_CoreProcess.mode(); }
  QString address() const;
  void open();
  ServerConfig &serverConfig() { return m_ServerConfig; }
  void autoAddScreen(const QString name);
  int showActivationDialog();

signals:
  void created();
  void shown();

public slots:
  void onAppAboutToQuit();

private slots:
  void onCreated();
  void onShown();
  void onConfigScopesSaving();
  void onAppConfigTlsChanged();
  void onAppConfigScreenNameChanged();
  void onAppConfigInvertConnection();
  void onCoreProcessStarting();
  void onCoreProcessError(CoreProcess::Error error);
  void onCoreConnectionStateChanged(CoreProcess::ConnectionState state);
  void onCoreProcessStateChanged(CoreProcess::ProcessState state);
  void onCoreProcessSecureSocket(bool enabled);
  void onLicenseHandlerSerialKeyChanged(const QString &serialKey);
  void onLicenseHandlerInvalidLicense();
  void onVersionCheckerUpdateFound(const QString &version);
  void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);
  void onActionStartCoreTriggered();
  void onActionStopCoreTriggered();
  void onWindowSaveTimerTimeout();
  void onServerConnectionConfigureClient(const QString &clientName);

  // autoconnect slots
  void on_m_pButtonApply_clicked();
  void on_m_pLabelComputerName_linkActivated(const QString &link);
  void on_m_pLabelFingerprint_linkActivated(const QString &link);
  void on_m_pButtonConnect_clicked();
  void on_m_pButtonConnectToClient_clicked();
  void on_m_pRadioGroupServer_clicked(bool);
  void on_m_pRadioGroupClient_clicked(bool);
  void on_m_pButtonConfigureServer_clicked();
  bool on_m_pActionSave_triggered();
  void on_m_pActionAbout_triggered();
  void on_m_pActionHelp_triggered();
  void on_m_pActionSettings_triggered();
  void on_m_pActivate_triggered();
  void on_m_pLineEditHostname_returnPressed();
  void on_m_pLineEditClientIp_returnPressed();
  void on_m_pLineEditHostname_textChanged(const QString &text);
  void on_m_pLineEditClientIp_textChanged(const QString &text);

private:
  AppConfig &appConfig() { return m_AppConfig; }
  AppConfig const &appConfig() const { return m_AppConfig; }
  void createMenuBar();
  void createStatusBar();
  void createTrayIcon();
  void applyConfig();
  void applyCloseToTray() const;
  void setIcon(CoreProcess::ConnectionState state);
  bool checkForApp(int which, QString &app);
  void setStatus(const QString &status);
  void sendIpcMessage(IpcMessageType type, const char *buffer, bool showErrors);
  void updateFromLogLine(const QString &line);
  QString getIPAddresses() const;
  void enableServer(bool enable);
  void enableClient(bool enable);
  void checkConnected(const QString &line);
  void checkFingerprint(const QString &line);
  void checkLicense(const QString &line);
  QString getTimeStamp() const;
  void showEvent(QShowEvent *) override;
  void closeEvent(QCloseEvent *event) override;
  void secureSocket(bool secureSocket);
  void windowStateChanged();
  void connectSlots();
  void updateWindowTitle();
  void handleLogLine(const QString &line);
  void updateLocalFingerprint();
  void updateScreenName();
  void saveSettings();
  QString configFilename();
  void showConfigureServer(const QString &message);
  void showConfigureServer() { showConfigureServer(""); }
  void showLicenseNotice();
  void restoreWindow();
  void saveWindow();
  void setupControls();
  void resizeEvent(QResizeEvent *event) override;
  void moveEvent(QMoveEvent *event) override;
  void showFirstRunMessage();
  void showDevThanksMessage();
  QString productName() const;
  void updateStatus();

  VersionChecker m_VersionChecker;
  TrayIcon m_TrayIcon;
  bool m_ActivationDialogRunning = false;
  QStringList m_PendingClientNames;
  QMenuBar *m_pMenuBar = nullptr;
  QMenu *m_pMenuFile = nullptr;
  QMenu *m_pMenuEdit = nullptr;
  QMenu *m_pMenuWindow = nullptr;
  QMenu *m_pMenuHelp = nullptr;
  QAbstractButton *m_pCancelButton = nullptr;
  bool m_SecureSocket = false;
  bool m_SaveWindow = false;
  LicenseHandler m_LicenseHandler;

  synergy::gui::ConfigScopes &m_ConfigScopes;
  AppConfig &m_AppConfig;
  ServerConfig m_ServerConfig;
  synergy::gui::CoreProcess m_CoreProcess;
  synergy::gui::ServerConnection m_ServerConnection;
  synergy::gui::ClientConnection m_ClientConnection;
  synergy::gui::TlsUtility m_TlsUtility;
  QTimer m_WindowSaveTimer;
};
