/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Bolton Software Ltd.
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
#include "CryptoMode.h"

// this should be incremented each time a new page is added. this is
// saved to settings when the user finishes running the wizard. if
// the saved wizard version is lower than this number, the wizard
// will be displayed.
const int kWizardVersion = 1;

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
		bool autoStart() const { return m_AutoStart; }
		bool autoHide() const { return m_AutoHide; }
		bool autoStartPrompt() const { return m_AutoStartPrompt; }
		const QString& cryptoPass() const { return m_CryptoPass; }
		CryptoMode cryptoMode() const { return m_CryptoMode; }
		QString cryptoModeString() const;
		ProcessMode processMode() const { return m_ProcessMode; }
		bool wizardShouldRun() const { return m_WizardLastRun < kWizardVersion; }

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
		void setAutoStart(bool b);
		void setAutoHide(bool b) { m_AutoHide = b; }
		void setAutoStartPrompt(bool b) { m_AutoStartPrompt = b; }
		void setCryptoMode(CryptoMode c) { m_CryptoMode = c; }
		void setProcessMode(ProcessMode p) { m_ProcessMode = p; }
		void setWizardHasRun() { m_WizardLastRun = kWizardVersion; }

		void loadSettings();
		void saveSettings();

		void setCryptoPass(const QString& s);
		static QString hash(const QString& string);

	private:
		QSettings* m_pSettings;
		bool m_AutoConnect;
		QString m_ScreenName;
		int m_Port;
		QString m_Interface;
		int m_LogLevel;
		bool m_LogToFile;
		QString m_LogFilename;
		bool m_AutoStart;
		bool m_AutoHide;
		bool m_AutoStartPrompt;
		int m_WizardLastRun;
		QString m_CryptoPass;
		CryptoMode m_CryptoMode;
		ProcessMode m_ProcessMode;

		static const char m_SynergysName[];
		static const char m_SynergycName[];
		static const char m_SynergyLogDir[];
};

#endif
