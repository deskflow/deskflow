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
#include "gui/QIpcClient.h"
#include "gui/VersionChecker.h"
#include "shared/Ipc.h"

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
class SslCertificate;
class LicenseManager;

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
#ifdef SYNERGY_ENABLE_LICENSING
  MainWindow(AppConfig &appConfig, LicenseManager &licenseManager);
#else
  explicit MainWindow(AppConfig &appConfig);
#endif
  ~MainWindow() override;

public:
  void setVisible(bool visible);
  CoreMode coreMode() const {
    auto isClient = m_pRadioGroupClient->isChecked();
    return isClient ? CoreMode::Client : CoreMode::Server;
  }
  CoreState coreState() const { return m_CoreState; }
  QString hostname() const { return m_pLineEditHostname->text(); }
  QString configFilename();
  QString address() const;
  QString appPath(const QString &name);
  void open();
  void clearLog();
  VersionChecker &versionChecker() { return m_VersionChecker; }
  ServerConfig &serverConfig() { return m_ServerConfig; }
  void showConfigureServer(const QString &message);
  void showConfigureServer() { showConfigureServer(""); }
  void autoAddScreen(const QString name);
#ifdef SYNERGY_ENABLE_LICENSING
  LicenseManager &licenseManager() const;
  int raiseActivationDialog();
#endif

public slots:
  void setEdition(Edition edition);
#ifdef SYNERGY_ENABLE_LICENSING
  void InvalidLicense();
  void showLicenseNotice(const QString &message);
#endif
  void appendLogRaw(const QString &text);
  void appendLogInfo(const QString &text);
  void appendLogDebug(const QString &text);
  void appendLogError(const QString &text);
  void startCore();
  void retryStart();
  void actionStart();
  void handleIdleService(const QString &text);

protected slots:
  void updateLocalFingerprint();
  void updateScreenName();
  void on_m_pRadioGroupServer_clicked(bool);
  void on_m_pRadioGroupClient_clicked(bool);
  void on_m_pButtonConfigureServer_clicked();
  bool on_m_pActionSave_triggered();
  void on_m_pActionAbout_triggered();
  void on_m_pActionHelp_triggered();
  void on_m_pActionSettings_triggered();
  void on_m_pActivate_triggered();
  void coreProcessExit(int exitCode, QProcess::ExitStatus);
  void trayActivated(QSystemTrayIcon::ActivationReason reason);
  void stopCore();
  void logOutput();
  void logError();
  void updateFound(const QString &version);
  void saveSettings();

protected:
  QSettings &settings() { return *appConfig().config().currentSettings(); }
  AppConfig &appConfig() { return m_AppConfig; }
  AppConfig const &appConfig() const { return m_AppConfig; }
  AppConfig *appConfigPtr() { return &m_AppConfig; }
  void initConnections();
  void createMenuBar();
  void createStatusBar();
  void createTrayIcon();
  void loadSettings();
  void setIcon(CoreState state) const;
  void setCoreState(CoreState state);
  bool checkForApp(int which, QString &app);
  bool clientArgs(QStringList &args, QString &app);
  bool serverArgs(QStringList &args, QString &app);
  void setStatus(const QString &status);
  void sendIpcMessage(IpcMessageType type, const char *buffer, bool showErrors);
  void updateFromLogLine(const QString &line);
  QString getIPAddresses();
  void stopService();
  void stopDesktop();
  void enableServer(bool enable);
  void enableClient(bool enable);

  QString getProfileRootForArg();
  void checkConnected(const QString &line);
  void checkFingerprint(const QString &line);
  bool checkSecureSocket(const QString &line);
#ifdef Q_OS_MAC
  void checkOSXNotification(const QString &line);
#endif
#ifdef SYNERGY_ENABLE_LICENSING
  void checkLicense(const QString &line);
#endif
  QString getTimeStamp();
  void restartCore();

  void showEvent(QShowEvent *) override;
  void secureSocket(bool secureSocket);

  void windowStateChanged();

private:
  void updateWindowTitle();

#ifdef SYNERGY_ENABLE_LICENSING
  LicenseManager *m_LicenseManager = nullptr;
#endif

  AppConfig &m_AppConfig;
  ServerConfig m_ServerConfig;
  ServerConnection m_serverConnection;
  ClientConnection m_clientConnection;
  VersionChecker m_VersionChecker;
  QIpcClient m_IpcClient;
  TrayIcon m_trayIcon;
  QMutex m_StopDesktopMutex;

#ifdef SYNERGY_ENABLE_LICENSING
  bool m_ActivationDialogRunning = false;
  QStringList m_PendingClientNames;
#endif

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

  /// @brief Is the program running a secure socket protocol (SSL/TLS)
  bool m_SecureSocket = false;

  /// @brief Contains the version of the Secure Socket currently active
  QString m_SecureSocketVersion = "";

private slots:
  void on_m_pButtonApply_clicked();
  void on_windowShown();

  void on_m_pLabelComputerName_linkActivated(const QString &link);
  void on_m_pLabelFingerprint_linkActivated(const QString &link);

  void on_m_pButtonConnect_clicked();
  void on_m_pButtonConnectToClient_clicked();

signals:
  void windowShown();
};
