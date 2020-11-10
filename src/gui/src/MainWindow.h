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
#include <QSystemTrayIcon>
#include <QSettings>
#include <QProcess>
#include <QThread>

#include "ui_MainWindowBase.h"

#include "ServerConfig.h"
#include "AppConfig.h"
#include "VersionChecker.h"
#include "IpcClient.h"
#include "Ipc.h"
#include "ActivationDialog.h"
#include "ConfigWriter.h"

#include <QMutex>

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
class Zeroconf;

class MainWindow : public QMainWindow, public Ui::MainWindowBase
{
    Q_OBJECT

    friend class QSynergyApplication;
    friend class SetupWizard;
    friend class ActivationDialog;
    friend class SettingsDialog;

    public:
        enum qSynergyState
        {
            synergyDisconnected,
            synergyConnecting,
            synergyConnected,
            synergyListening,
            synergyPendingRetry
        };

        enum qSynergyType
        {
            synergyClient,
            synergyServer
        };

        enum qLevel {
            Error,
            Info
        };

        enum qRuningState {
            kStarted,
            kStopped
        };

    public:
#ifdef SYNERGY_ENTERPRISE
        MainWindow(AppConfig& appConfig);
#else
        MainWindow(AppConfig& appConfig,
                   LicenseManager& licenseManager);
#endif
        ~MainWindow();

    public:
        void setVisible(bool visible);
        int synergyType() const { return m_pGroupClient->isChecked() ? synergyClient : synergyServer; }
        int synergyState() const { return m_SynergyState; }
        QString hostname() const { return m_pLineEditHostname->text(); }
        QString configFilename();
        QString address();
        QString appPath(const QString& name);
        void open();
        void clearLog();
        VersionChecker& versionChecker() { return m_VersionChecker; }
        QString getScreenName();
        ServerConfig& serverConfig() { return m_ServerConfig; }
        void showConfigureServer(const QString& message);
        void showConfigureServer() { showConfigureServer(""); }
        void autoAddScreen(const QString name);
        void addZeroconfServer(const QString name);
        Zeroconf& zeroconf() { return *m_pZeroconf; }
#ifndef SYNERGY_ENTERPRISE
        LicenseManager& licenseManager() const;
        int raiseActivationDialog();
#endif

        void updateZeroconfService();

public slots:
        void setEdition(Edition edition);
#ifndef SYNERGY_ENTERPRISE
		void InvalidLicense();
        void showLicenseNotice(const QString& message);
#endif
        void appendLogRaw(const QString& text);
        void appendLogInfo(const QString& text);
        void appendLogDebug(const QString& text);
        void appendLogError(const QString& text);
        void startSynergy();
        void retryStart(); // If the connection failed this will retry a startSynergy

    protected slots:
        void updateLocalFingerprint();
        void on_m_pGroupClient_toggled(bool on);
        void on_m_pGroupServer_toggled(bool on);
        bool on_m_pButtonBrowseConfigFile_clicked();
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
        void updateFound(const QString& version);
        void saveSettings();

        /// @brief Receives the signal that the auto config option has changed
        void zeroConfToggled();

    protected:
        // TODO This should be properly using the ConfigWriter system.
        QSettings& settings() { return GUI::Config::ConfigWriter::make()->settings(); }
        AppConfig& appConfig() { return *m_AppConfig; }
        QProcess* synergyProcess() { return m_pSynergy; }
        void setSynergyProcess(QProcess* p) { m_pSynergy = p; }
        void initConnections();
        void createMenuBar();
        void createStatusBar();
        void createTrayIcon();
        void loadSettings();
        void setIcon(qSynergyState state);
        void setSynergyState(qSynergyState state);
        bool checkForApp(int which, QString& app);
        bool clientArgs(QStringList& args, QString& app);
        bool serverArgs(QStringList& args, QString& app);
        void setStatus(const QString& status);
        void sendIpcMessage(qIpcMessageType type, const char* buffer, bool showErrors);
        void updateFromLogLine(const QString& line);
        QString getIPAddresses();
        void stopService();
        void stopDesktop();
        void changeEvent(QEvent* event);
        void retranslateMenuBar();

#if defined(Q_OS_WIN)
        bool isServiceRunning(QString name);
#else
        bool isServiceRunning();
#endif

        QString getProfileRootForArg();
        void checkConnected(const QString& line);
        void checkFingerprint(const QString& line);
        void checkSecureSocket(const QString& line);
#ifndef SYNERGY_ENTERPRISE
        void checkLicense(const QString& line);
#endif
        QString getTimeStamp();
        void restartSynergy();
        void proofreadInfo();

        void showEvent (QShowEvent*);
        void secureSocket(bool secureSocket);

        void windowStateChanged();


    private:
#ifndef SYNERGY_ENTERPRISE
        LicenseManager*     m_LicenseManager;
        bool                m_ActivationDialogRunning;
        QStringList         m_PendingClientNames;
#endif
        Zeroconf*           m_pZeroconf;
        AppConfig*          m_AppConfig;
        QProcess*           m_pSynergy;
        int                 m_SynergyState;
        ServerConfig        m_ServerConfig;
        QSystemTrayIcon*    m_pTrayIcon;
        QMenu*              m_pTrayIconMenu;
        bool                m_AlreadyHidden;
        VersionChecker      m_VersionChecker;
        IpcClient           m_IpcClient;
        QMenuBar*           m_pMenuBar;
        QMenu*              m_pMenuFile;
        QMenu*              m_pMenuEdit;
        QMenu*              m_pMenuWindow;
        QMenu*              m_pMenuHelp;
        QAbstractButton*    m_pCancelButton;
        qRuningState        m_ExpectedRunningState;
        QMutex              m_StopDesktopMutex;
        SslCertificate*     m_pSslCertificate;
        bool                m_SecureSocket;             // brief Is the program running a secure socket protocol (SSL/TLS)
        QString             m_SecureSocketVersion;      // brief Contains the version of the Secure Socket currently active

        void                updateAutoConfigWidgets();
        
private slots:
    void on_m_pButtonApply_clicked();
    void on_windowShown();

    void on_m_pLabelAutoConfig_linkActivated(const QString &link);

    void on_m_pComboServerList_currentIndexChanged(const QString &arg1);

signals:
    void windowShown();
};
