/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Synergy Si Ltd.
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

// this should be incremented each time a new page is added. this is
// saved to settings when the user finishes running the wizard. if
// the saved wizard version is lower than this number, the wizard
// will be displayed. each version incrememnt should be described
// here...
//
//   1: first version
//   2: added language page
//   3: added premium page and removed
//
const int kWizardVersion = 3;

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
	friend class LoginWindow;

	public:
		AppConfig(QSettings* settings);
		~AppConfig();

	public:
		const QString& screenName() const { return m_ScreenName; }
		int port() const { return m_Port; }
		const QString& interface() const { return m_Interface; }
		int logLevel() const { return m_LogLevel; }
		bool logToFile() const { return m_LogToFile; }
		const QString& logFilename() const { return m_LogFilename; }
		const QString logFilenameCmd() const;
		QString logLevelText() const;
		const QString& cryptoPass() const { return m_CryptoPass; }
		bool cryptoEnabled() const { return m_CryptoEnabled; }
		QString cryptoModeString() const;
		ProcessMode processMode() const { return m_ProcessMode; }
		bool wizardShouldRun() const { return m_WizardLastRun < kWizardVersion; }
		const QString& language() const { return m_Language; }
		bool startedBefore() const { return m_StartedBefore; }
		bool autoConfig() const { return m_AutoConfig; }
		void setAutoConfig(bool autoConfig);
		bool autoConfigPrompted()  { return m_AutoConfigPrompted; }
		void setAutoConfigPrompted(bool prompted);
		const QString& userEmail() const { return m_UserEmail; }
		const QString& userToken() const { return m_UserToken; }
		const int editionType() const { return m_EditionType; }

		QString synergysName() const { return m_SynergysName; }
		QString synergycName() const { return m_SynergycName; }
		QString synergyProgramDir() const;
		QString synergyLogDir() const;

		bool detectPath(const QString& name, QString& path);
		void persistLogDir();
		bool elevateMode();

	protected:
		QSettings& settings() { return *m_pSettings; }
		void setScreenName(const QString& s) { m_ScreenName = s; }
		void setPort(int i) { m_Port = i; }
		void setInterface(const QString& s) { m_Interface = s; }
		void setLogLevel(int i) { m_LogLevel = i; }
		void setLogToFile(bool b) { m_LogToFile = b; }
		void setLogFilename(const QString& s) { m_LogFilename = s; }
		void setCryptoEnabled(bool b) { m_CryptoEnabled = b; }
		void setWizardHasRun() { m_WizardLastRun = kWizardVersion; }
		void setLanguage(const QString language) { m_Language = language; }
		void setStartedBefore(bool b) { m_StartedBefore = b; }
		void setElevateMode(bool b) { m_ElevateMode = b; }
		void setUserEmail(const QString& e) { m_UserEmail = e; }
		void setUserToken(const QString& t) { m_UserToken = t; }
		void setEditionType(int t) { m_EditionType = t; }
		void loadSettings();
		void saveSettings();

		void setCryptoPass(const QString& s);

	private:
		QSettings* m_pSettings;
		QString m_ScreenName;
		int m_Port;
		QString m_Interface;
		int m_LogLevel;
		bool m_LogToFile;
		QString m_LogFilename;
		int m_WizardLastRun;
		bool m_CryptoEnabled;
		QString m_CryptoPass;
		ProcessMode m_ProcessMode;
		QString m_Language;
		bool m_StartedBefore;
		bool m_AutoConfig;
		bool m_ElevateMode;
		bool m_AutoConfigPrompted;
		QString m_UserEmail;
		QString m_UserToken;
		int m_EditionType;

		static const char m_SynergysName[];
		static const char m_SynergycName[];
		static const char m_SynergyLogDir[];
};

#endif
