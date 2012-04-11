/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2008 Volker Lanz (vl@fidra.de)
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#if !defined(APPCONFIG_H)

#define APPCONFIG_H

#include <QString>

class QSettings;
class SettingsDialog;

enum ProcessMode {
	Service,
	Desktop
};

class AppConfig
{
	friend class SettingsDialog;
	friend class MainWindow;
	friend class SetupWizard;

	public:
		AppConfig(QSettings* settings);
		~AppConfig();

	public:
		bool autoConnect() const { return m_AutoConnect; }
		const QString& screenName() const { return m_ScreenName; }
		int port() const { return m_Port; }
		const QString& interface() const { return m_Interface; }
		int logLevel() const { return m_LogLevel; }
		bool logToFile() const { return m_LogToFile; }
		const QString& logFilename() const { return m_LogFilename; }
		QString logLevelText() const;
		bool gameDevice() const { return m_GameDevice; }
		bool autoStart() const { return m_AutoStart; }
		bool autoHide() const { return m_AutoHide; }
		bool autoStartPrompt() const { return m_AutoStartPrompt; }
		bool wizardHasRun() const { return m_WizardHasRun; }
		ProcessMode processMode() const { return m_ProcessMode; }

		QString synergysName() const { return m_SynergysName; }
		QString synergycName() const { return m_SynergycName; }
		QString synergyProgramDir() const;
		QString synergyLogDir() const;

		bool detectPath(const QString& name, QString& path);
		void persistLogDir();

	protected:
		QSettings& settings() { return *m_pSettings; }
		void setAutoConnect(bool b) { m_AutoConnect = b; }
		void setScreenName(const QString& s) { m_ScreenName = s; }
		void setPort(int i) { m_Port = i; }
		void setInterface(const QString& s) { m_Interface = s; }
		void setLogLevel(int i) { m_LogLevel = i; }
		void setLogToFile(bool b) { m_LogToFile = b; }
		void setLogFilename(const QString& s) { m_LogFilename = s; }
		void setGameDevice(bool b) { m_GameDevice = b; }
		void setAutoStart(bool b);
		void setAutoHide(bool b) { m_AutoHide = b; }
		void setAutoStartPrompt(bool b) { m_AutoStartPrompt = b; }
		void setWizardHasRun(bool b) { m_WizardHasRun = b; }
		void setProcessMode(ProcessMode p) { m_ProcessMode = p; }

		void loadSettings();
		void saveSettings();

	private:
		QSettings* m_pSettings;
		bool m_AutoConnect;
		QString m_ScreenName;
		int m_Port;
		QString m_Interface;
		int m_LogLevel;
		bool m_LogToFile;
		QString m_LogFilename;
		bool m_GameDevice;
		bool m_AutoStart;
		bool m_AutoHide;
		bool m_AutoStartPrompt;
		bool m_WizardHasRun;
		ProcessMode m_ProcessMode;

		static const char m_SynergysName[];
		static const char m_SynergycName[];
		static const char m_SynergyLogDir[];
};

#endif
