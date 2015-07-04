/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Synergy Si Ltd.
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

#if !defined(MAINWINDOW__H)

#define MAINWINDOW__H

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
class QTemporaryFile;
class QMessageBox;
class QAbstractButton;

class LogDialog;
class QSynergyApplication;
class SetupWizard;
class ZeroconfService;
class DataDownloader;
class CommandProcess;

class MainWindow : public QMainWindow, public Ui::MainWindowBase
{
	Q_OBJECT

	friend class QSynergyApplication;
	friend class SetupWizard;

	public:
		enum qSynergyState
		{
			synergyDisconnected,
			synergyConnecting,
			synergyConnected
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

	public:
		MainWindow(QSettings& settings, AppConfig& appConfig);
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
		void updateZeroconfService();
		void serverDetected(const QString name);
		void setEdition(int type);
		void updateLocalFingerprint();

	public slots:
		void appendLogRaw(const QString& text);
		void appendLogNote(const QString& text);
		void appendLogDebug(const QString& text);
		void appendLogError(const QString& text);
		void startSynergy();

	protected slots:
		void on_m_pGroupClient_toggled(bool on);
		void on_m_pGroupServer_toggled(bool on);
		bool on_m_pButtonBrowseConfigFile_clicked();
		void on_m_pButtonConfigureServer_clicked();
		bool on_m_pActionSave_triggered();
		void on_m_pActionAbout_triggered();
		void on_m_pActionSettings_triggered();
		void on_m_pActionWizard_triggered();
		void synergyFinished(int exitCode, QProcess::ExitStatus);
		void trayActivated(QSystemTrayIcon::ActivationReason reason);
		void stopSynergy();
		void logOutput();
		void logError();
		void updateFound(const QString& version);
		void bonjourInstallFinished();

	protected:
		QSettings& settings() { return m_Settings; }
		AppConfig& appConfig() { return m_AppConfig; }
		QProcess*& synergyProcess() { return m_pSynergy; }
		void setSynergyProcess(QProcess* p) { m_pSynergy = p; }
		void initConnections();
		void createMenuBar();
		void createStatusBar();
		void createTrayIcon();
		void loadSettings();
		void saveSettings();
		void setIcon(qSynergyState state);
		void setSynergyState(qSynergyState state);
		bool checkForApp(int which, QString& app);
		bool clientArgs(QStringList& args, QString& app);
		bool serverArgs(QStringList& args, QString& app);
		void setStatus(const QString& status);
		void sendIpcMessage(qIpcMessageType type, const char* buffer, bool showErrors);
		void onModeChanged(bool startDesktop, bool applyService);
		void updateStateFromLogLine(const QString& line);
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
		bool isBonjourRunning();
		void downloadBonjour();
		void promptAutoConfig();
		void updateEdition();
		QString getProfileRootForArg();
		void checkFingerprint(const QString& line);
		bool autoHide();

	private:
		QSettings& m_Settings;
		AppConfig& m_AppConfig;
		QProcess* m_pSynergy;
		int m_SynergyState;
		ServerConfig m_ServerConfig;
		QTemporaryFile* m_pTempConfigFile;
		QSystemTrayIcon* m_pTrayIcon;
		QMenu* m_pTrayIconMenu;
		bool m_AlreadyHidden;
		VersionChecker m_VersionChecker;
		IpcClient m_IpcClient;
		QMenuBar* m_pMenuBar;
		QMenu* m_pMenuFile;
		QMenu* m_pMenuEdit;
		QMenu* m_pMenuWindow;
		QMenu* m_pMenuHelp;
		ZeroconfService* m_pZeroconfService;
		DataDownloader* m_pDataDownloader;
		QMessageBox* m_DownloadMessageBox;
		QAbstractButton* m_pCancelButton;
		QMutex m_Mutex;
		bool m_SuppressAutoConfigWarning;
		CommandProcess* m_BonjourInstall;
		bool m_SuppressEmptyServerWarning;

private slots:
	void on_m_pCheckBoxAutoConfig_toggled(bool checked);
	void on_m_pComboServerList_currentIndexChanged(QString );
	void on_m_pButtonApply_clicked();
	void installBonjour();
};

#endif

