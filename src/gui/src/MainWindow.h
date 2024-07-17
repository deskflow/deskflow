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

#include "ui_MainWindowBase.h"

#include "ActivationDialog.h"
#include "AppConfig.h"
#include "ClientConnection.h"
#include "ConfigWriter.h"
#include "QIpcClient.h"
#include "ServerConfig.h"
#include "ServerConnection.h"
#include "TrayIcon.h"
#include "VersionChecker.h"
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
  enum qSynergyState {
    synergyDisconnected,
    synergyConnecting,
    synergyConnected,
    synergyListening,
    synergyPendingRetry
  };

  enum qSynergyType { synergyClient, synergyServer };

  enum qLevel { Error, Info };

  enum qRuningState { kStarted, kStopped };

public:
#ifdef SYNERGY_ENABLE_LICENSING
  MainWindow(AppConfig &appConfig, LicenseManager &licenseManager);
#else
  MainWindow(AppConfig &appConfig);
#endif
  ~MainWindow();

public:
  void setVisible(bool visible);
  int synergyType() const {
    return m_pRadioGroupClient->isChecked() ? synergyClient : synergyServer;
  }
  int synergyState() const { return m_SynergyState; }
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
  void startSynergy();
  void retryStart(); // If the connection failed this will retry a startSynergy
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
  void synergyFinished(int exitCode, QProcess::ExitStatus);
  void trayActivated(QSystemTrayIcon::ActivationReason reason);
  void stopSynergy();
  void logOutput();
  void logError();
  void updateFound(const QString &version);
  void saveSettings();

protected:
  // TODO This should be properly using the ConfigWriter system.
  QSettings &settings() {
    return GUI::Config::ConfigWriter::make()->settings();
  }
  AppConfig &appConfig() { return *m_AppConfig; }
  AppConfig const &appConfig() const { return *m_AppConfig; }
  QProcess *synergyProcess() { return m_pSynergy; }
  void setSynergyProcess(QProcess *p) { m_pSynergy = p; }
  void initConnections();
  void createMenuBar();
  void createStatusBar();
  void createTrayIcon();
  void loadSettings();
  void setIcon(qSynergyState state) const;
  void setSynergyState(qSynergyState state);
  bool checkForApp(int which, QString &app);
  bool clientArgs(QStringList &args, QString &app);
  bool serverArgs(QStringList &args, QString &app);
  void setStatus(const QString &status);
  void sendIpcMessage(EIpcMessage type, const char *buffer, bool showErrors);
  void updateFromLogLine(const QString &line);
  QString getIPAddresses();
  void stopService();
  void stopDesktop();
  void enableServer(bool enable);
  void enableClient(bool enable);
  void closeEvent(QCloseEvent *event) override;

#if defined(Q_OS_WIN)
  bool isServiceRunning(QString name);
#else
  bool isServiceRunning();
#endif

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
  void restartSynergy();

  void showEvent(QShowEvent *);
  void secureSocket(bool secureSocket);

  void windowStateChanged();

private:
#ifdef SYNERGY_ENABLE_LICENSING
  LicenseManager *m_LicenseManager;
  bool m_ActivationDialogRunning;
  QStringList m_PendingClientNames;
#endif
  AppConfig *m_AppConfig;
  QProcess *m_pSynergy;
  int m_SynergyState;
  ServerConfig m_ServerConfig;
  bool m_AlreadyHidden;
  VersionChecker m_VersionChecker;
  QIpcClient m_IpcClient;
  QMenuBar *m_pMenuBar;
  QMenu *m_pMenuFile;
  QMenu *m_pMenuEdit;
  QMenu *m_pMenuWindow;
  QMenu *m_pMenuHelp;
  QAbstractButton *m_pCancelButton;
  TrayIcon m_trayIcon;
  qRuningState m_ExpectedRunningState;
  QMutex m_StopDesktopMutex;
  bool m_SecureSocket; // brief Is the program running a secure socket protocol
                       // (SSL/TLS)
  QString m_SecureSocketVersion; // brief Contains the version of the Secure
                                 // Socket currently active
  ServerConnection m_serverConnection;
  ClientConnection m_clientConnection;

  void updateWindowTitle();

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
