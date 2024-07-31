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
#include <memory>

#include "ui_MainWindowBase.h"

#include "ActivationDialog.h"
#include "AppConfig.h"
#include "ClientConnection.h"
#include "Config.h"
#include "ServerConfig.h"
#include "ServerConnection.h"
#include "TrayIcon.h"
#include "global/Ipc.h"
#include "gui/QIpcClient.h"
#include "gui/TlsUtility.h"
#include "gui/VersionChecker.h"

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

class LogDialog;
class QSynergyApplication;
class SetupWizard;
class DataDownloader;
class CommandProcess;
class TlsCertificate;

class MainWindow : public QMainWindow, public Ui::MainWindowBase {
  Q_OBJECT

  friend class QSynergyApplication;
  friend class SetupWizard;
  friend class ActivationDialog;
  friend class SettingsDialog;
  friend class ServerConnection;
  friend class ClientConnection;

public:
  enum class CoreState {
    Disconnected,
    Connecting,
    Connected,
    Listening,
    PendingRetry
  };

  enum class CoreMode { Client, Server };
  enum class LogLevel { Error, Info };
  enum class RuningState { Started, Stopped };

public:
  explicit MainWindow(AppConfig &appConfig);
  ~MainWindow() override;

  void setVisible(bool visible) override;
  CoreMode coreMode() const;
  QString address() const;
  QString appPath(const QString &name) const;
  void open();
  ServerConfig &serverConfig() { return m_ServerConfig; }
  void autoAddScreen(const QString name);
  LicenseHandler &licenseHandler();
  int showActivationDialog();
  void appendLogInfo(const QString &text);
  void appendLogDebug(const QString &text);
  void appendLogError(const QString &text);

signals:
  void created();
  void shown();

public slots:
  void onAppAboutToQuit();

private slots:
  void onCreated();
  void onShown();
  void onAppConfigLoaded();
  void onAppConfigTlsChanged();
  void onAppConfigScreenNameChanged();
  void onAppConfigInvertConnection();
  void onLicenseHandlerSerialKeyChanged(const QString &serialKey);
  void onLicenseHandlerInvalidLicense();
  void onIpcClientReadLogLine(const QString &text);
  void onIpcClientErrorMessage(const QString &text);
  void onIpcClientInfoMessage(const QString &text);
  void onCoreProcessFinished(int exitCode, QProcess::ExitStatus);
  void onVersionCheckerUpdateFound(const QString &version);
  void onTrayIconCreate(QSystemTrayIcon::ActivationReason reason);
  void onActionStartCoreTriggered();
  void onActionStopCoreTriggered();
  void onCoreProcessReadyReadStandardOutput();
  void onCoreProcessReadyReadStandardError();
  void onWindowSaveTimerTimeout();

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

private:
  QSettings &settings() { return *appConfig().config().currentSettings(); }
  AppConfig &appConfig() { return m_AppConfig; }
  AppConfig const &appConfig() const { return m_AppConfig; }
  void createMenuBar();
  void createStatusBar();
  void createTrayIcon();
  void applyConfig();
  void setIcon(CoreState state) const;
  void setCoreState(CoreState state);
  bool checkForApp(int which, QString &app);
  bool clientArgs(QStringList &args, QString &app);
  bool serverArgs(QStringList &args, QString &app);
  void setStatus(const QString &status);
  void sendIpcMessage(IpcMessageType type, const char *buffer, bool showErrors);
  void updateFromLogLine(const QString &line);
  QString getIPAddresses() const;
  void stopService();
  void stopDesktop();
  void enableServer(bool enable);
  void enableClient(bool enable);
  QString getProfileRootForArg() const;
  void checkConnected(const QString &line);
  void checkFingerprint(const QString &line);
  bool checkSecureSocket(const QString &line);
  void checkLicense(const QString &line);
  QString getTimeStamp() const;
  void restartCore();
  void showEvent(QShowEvent *) override;
  void secureSocket(bool secureSocket);
  void windowStateChanged();
  void connectSlots() const;
  void updateWindowTitle();
  void processCoreLogLine(const QString &line);
  void startCore();
  void onCoreProcessRetryStart();
  void updateLocalFingerprint();
  void updateScreenName();
  void saveSettings();
  QString configFilename();
  void showConfigureServer(const QString &message);
  void showConfigureServer() { showConfigureServer(""); }
  void showLicenseNotice();
  void stopCore();
  void restoreWindow();
  void saveWindow();
  void setupControls();
  void resizeEvent(QResizeEvent *event) override;
  void moveEvent(QMoveEvent *event) override;

#ifdef Q_OS_MAC
  void checkOSXNotification(const QString &line);
#endif

  VersionChecker m_VersionChecker;
  QIpcClient m_IpcClient;
  TrayIcon m_TrayIcon;
  QMutex m_StopDesktopMutex;
  bool m_ActivationDialogRunning = false;
  QStringList m_PendingClientNames;
  RuningState m_ExpectedRunningState = RuningState::Stopped;
  std::unique_ptr<QProcess> m_pCoreProcess;
  QMenuBar *m_pMenuBar = nullptr;
  QMenu *m_pMenuFile = nullptr;
  QMenu *m_pMenuEdit = nullptr;
  QMenu *m_pMenuWindow = nullptr;
  QMenu *m_pMenuHelp = nullptr;
  QAbstractButton *m_pCancelButton = nullptr;
  CoreState m_CoreState = CoreState::Disconnected;
  bool m_AlreadyHidden = false;
  bool m_SecureSocket = false;
  QString m_SecureSocketVersion = "";
  bool m_SaveWindow = false;

  AppConfig &m_AppConfig;
  LicenseHandler m_LicenseHandler;
  ServerConfig m_ServerConfig;
  ServerConnection m_ServerConnection;
  ClientConnection m_ClientConnection;
  synergy::gui::TlsUtility m_TlsUtility;
  QTimer m_WindowSaveTimer;
};
