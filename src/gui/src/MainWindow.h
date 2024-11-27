/*
 * Deskflow -- mouse and keyboard sharing utility
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

#include "ServerConfig.h"
#include "TrayIcon.h"
#include "VersionChecker.h"
#include "common/ipc.h"
#include "gui/config/AppConfig.h"
#include "gui/config/ConfigScopes.h"
#include "gui/config/ServerConfigDialogState.h"
#include "gui/core/ClientConnection.h"
#include "gui/core/CoreProcess.h"
#include "gui/core/ServerConnection.h"
#include "gui/core/WaylandWarnings.h"
#include "gui/tls/TlsUtility.h"

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

class DeskflowApplication;
class SetupWizard;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
  using CoreMode = deskflow::gui::CoreProcess::Mode;
  using CoreProcess = deskflow::gui::CoreProcess;

  Q_OBJECT

  friend class DeskflowApplication;
  friend class SetupWizard;
  friend class SettingsDialog;

public:
  enum class LogLevel
  {
    Error,
    Info
  };

public:
  explicit MainWindow(deskflow::gui::ConfigScopes &configScopes, AppConfig &appConfig);
  ~MainWindow() override;

  void setVisible(bool visible) override;
  CoreMode coreMode() const
  {
    return m_CoreProcess.mode();
  }
  QString address() const;
  void open();
  ServerConfig &serverConfig()
  {
    return m_ServerConfig;
  }
  void autoAddScreen(const QString name);

signals:
  void created();
  void shown();

public slots:
  void onAppAboutToQuit();

private slots:
  //
  // Manual slots
  //
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
  void onVersionCheckerUpdateFound(const QString &version);
  void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);
  void onWindowSaveTimerTimeout();
  void onServerConnectionConfigureClient(const QString &clientName);

  //
  // Auto-connect slots
  //
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
  void on_m_pActionHelp_triggered() const;
  void on_m_pActionSettings_triggered();
  void on_m_pActionStartCore_triggered();
  void on_m_pActionStopCore_triggered();
  void on_m_pActionTestFatalError_triggered() const;
  void on_m_pActionTestCriticalError_triggered() const;
  void on_m_pActionClearSettings_triggered();
  void on_m_pLineEditHostname_returnPressed();
  void on_m_pLineEditClientIp_returnPressed();
  void on_m_pLineEditHostname_textChanged(const QString &text);
  void on_m_pLineEditClientIp_textChanged(const QString &text);

private:
  std::unique_ptr<Ui::MainWindow> ui;

  AppConfig &appConfig()
  {
    return m_AppConfig;
  }
  AppConfig const &appConfig() const
  {
    return m_AppConfig;
  }
  void createMenuBar();
  void createStatusBar();
  void createTrayIcon();
  void applyConfig();
  void applyCloseToTray() const;
  void setIcon();
  bool checkForApp(int which, QString &app);
  void setStatus(const QString &status);
  void sendIpcMessage(IpcMessageType type, const char *buffer, bool showErrors);
  void updateFromLogLine(const QString &line);
  QString getIPAddresses() const;
  void enableServer(bool enable);
  void enableClient(bool enable);
  void checkConnected(const QString &line);
  void checkFingerprint(const QString &line);
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
  void showConfigureServer()
  {
    showConfigureServer("");
  }
  void restoreWindow();
  void saveWindow();
  void setupControls();
  void resizeEvent(QResizeEvent *event) override;
  void moveEvent(QMoveEvent *event) override;
  void showFirstConnectedMessage();
  void updateStatus();
  void showAndActivate();

  VersionChecker m_VersionChecker;
  deskflow::gui::TrayIcon m_TrayIcon;
  QMenuBar *m_pMenuBar = nullptr;
  QMenu *m_pMenuFile = nullptr;
  QMenu *m_pMenuEdit = nullptr;
  QMenu *m_pMenuWindow = nullptr;
  QMenu *m_pMenuHelp = nullptr;
  QAbstractButton *m_pCancelButton = nullptr;
  bool m_SecureSocket = false;
  bool m_SaveWindow = false;
  bool m_Quitting = false;
  deskflow::gui::config::ServerConfigDialogState m_ServerConfigDialogState;
  bool m_SaveOnExit = true;
  deskflow::gui::core::WaylandWarnings m_WaylandWarnings;

  deskflow::gui::ConfigScopes &m_ConfigScopes;
  AppConfig &m_AppConfig;
  ServerConfig m_ServerConfig;
  deskflow::gui::CoreProcess m_CoreProcess;
  deskflow::gui::ServerConnection m_ServerConnection;
  deskflow::gui::ClientConnection m_ClientConnection;
  deskflow::gui::TlsUtility m_TlsUtility;
  QTimer m_WindowSaveTimer;
};
