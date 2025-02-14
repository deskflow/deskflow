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
#include <QSettings>
#include <QSystemTrayIcon>
#include <QThread>

#include "ServerConfig.h"
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

#ifdef Q_OS_MAC
#include "gui/OSXHelpers.h"
#endif

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
class QLocalServer;

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

  CoreMode coreMode() const
  {
    return m_coreProcess.mode();
  }
  QString address() const;
  void open();
  ServerConfig &serverConfig()
  {
    return m_serverConfig;
  }
  void autoAddScreen(const QString name);

  void hide();

signals:
  void shown();

private:
  void toggleLogVisible(bool visible);

  void firstShown();

  void configScopesSaving();
  void appConfigTlsChanged();
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
  void openSettings();
  void startCore();
  void stopCore();
  bool saveConfig();
  void testFatalError() const;
  void testCriticalError() const;
  void resetCore();

  void showMyFingerprint();
  void setModeServer();
  void setModeClient();
  void updateSecurityIcon(bool visible);

  std::unique_ptr<Ui::MainWindow> ui;

  void updateSize();
  AppConfig &appConfig()
  {
    return m_appConfig;
  }
  AppConfig const &appConfig() const
  {
    return m_appConfig;
  }
  void createMenuBar();
  void setupTrayIcon();
  void applyConfig();
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
  void restoreWindow();
  void setupControls();
  void showFirstConnectedMessage();
  void updateStatus();
  void showAndActivate();

  QString getTlsPath();

  // Generate prints if they are missing
  // Returns true if successful
  bool regenerateLocalFingerprints();

  VersionChecker m_versionChecker;
  bool m_secureSocket = false;
  deskflow::gui::config::ServerConfigDialogState m_serverConfigDialogState;
  bool m_saveOnExit = true;
  deskflow::gui::core::WaylandWarnings m_waylandWarnings;

  deskflow::gui::ConfigScopes &m_configScopes;
  AppConfig &m_appConfig;
  ServerConfig m_serverConfig;
  deskflow::gui::CoreProcess m_coreProcess;
  deskflow::gui::ServerConnection m_serverConnection;
  deskflow::gui::ClientConnection m_clientConnection;
  deskflow::gui::TlsUtility m_tlsUtility;
  QSize m_expandedSize = QSize();

  QSystemTrayIcon *m_trayIcon = nullptr;
  QLocalServer *m_guiDupeChecker = nullptr;
  inline static const auto m_guiSocketName = QStringLiteral("deskflow-gui");

  // Window Actions
  QAction *m_actionAbout = nullptr;
  QAction *m_actionClearSettings = nullptr;
  QAction *m_actionReportBug = nullptr;
  QAction *m_actionMinimize = nullptr;
  QAction *m_actionQuit = nullptr;
  QAction *m_actionTrayQuit = nullptr;
  QAction *m_actionRestore = nullptr;
  QAction *m_actionSave = nullptr;
  QAction *m_actionSettings = nullptr;
  QAction *m_actionStartCore = nullptr;
  QAction *m_actionStopCore = nullptr;
  QAction *m_actionTestCriticalError = nullptr;
  QAction *m_actionTestFatalError = nullptr;
};
