#if !defined(APPCONFIG_H)

#define APPCONFIG_H

#include <QString>

class QSettings;
class SettingsDialog;

class AppConfig
{
	friend class SettingsDialog;
	friend class MainWindow;

	public:
		AppConfig(QSettings* settings);
		~AppConfig();

	public:
		bool autoConnect() const { return m_AutoConnect; }
		const QString& synergyc() const { return m_Synergyc; }
		const QString& synergys() const { return m_Synergys; }
		const QString& screenName() const { return m_ScreenName; }
		int port() const { return m_Port; }
		const QString& interface() const { return m_Interface; }
		int logLevel() const { return m_LogLevel; }

		QString synergysName() const { return m_SynergysName; }
		QString synergycName() const { return m_SynergycName; }
		QString synergyProgramDir() const { return m_SynergyProgramDir; }

	protected:
		QSettings& settings() { return *m_pSettings; }
		void setAutoConnect(bool b) { m_AutoConnect = b; }
		void setSynergyc(const QString& s) { m_Synergyc = s; }
		void setSynergys(const QString& s) { m_Synergys = s; }
		void setScreenName(const QString& s) { m_ScreenName = s; }
		void setPort(int i) { m_Port = i; }
		void setInterface(const QString& s) { m_Interface = s; }
		void setLogLevel(int i) { m_LogLevel = i; }

		void loadSettings();
		void saveSettings();

	private:
		QSettings* m_pSettings;
		bool m_AutoConnect;
		QString m_Synergyc;
		QString m_Synergys;
		QString m_ScreenName;
		int m_Port;
		QString m_Interface;
		int m_LogLevel;

		static const char m_SynergysName[];
		static const char m_SynergycName[];
		static const char m_SynergyProgramDir[];
};

#endif
