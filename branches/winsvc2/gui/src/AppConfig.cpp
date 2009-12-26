#include "AppConfig.h"

#include <QtCore>
#include <QtNetwork>

#if defined(Q_OS_WIN)
const char AppConfig::m_SynergysName[] = "synergys.exe";
const char AppConfig::m_SynergycName[] = "synergyc.exe";
const char AppConfig::m_SynergyProgramDir[] = "c:/program files/synergy/";
#else
const char AppConfig::m_SynergysName[] = "synergys";
const char AppConfig::m_SynergycName[] = "synergyc";
const char AppConfig::m_SynergyProgramDir[] = "/usr/bin/";
#endif


AppConfig::AppConfig(QSettings* settings) :
	m_pSettings(settings),
	m_AutoConnect(false),
	m_Synergyc(),
	m_Synergys(),
	m_ScreenName(),
	m_Port(24800),
	m_Interface(),
	m_LogLevel(0)
{
	Q_ASSERT(m_pSettings);

	loadSettings();
}

AppConfig::~AppConfig()
{
	saveSettings();
}

void AppConfig::loadSettings()
{
	m_AutoConnect = settings().value("autoConnectChecked", false).toBool();
	m_Synergyc = settings().value("synergyc", QString(synergyProgramDir()) + synergycName()).toString();
	m_Synergys = settings().value("synergys", QString(synergyProgramDir()) + synergysName()).toString();
	m_ScreenName = settings().value("screenName", QHostInfo::localHostName()).toString();
	m_Port = settings().value("port", 24800).toInt();
	m_Interface = settings().value("interface").toString();
	m_LogLevel = settings().value("logLevelIndex", 0).toInt();
}

void AppConfig::saveSettings()
{
	settings().setValue("autoConnectChecked", m_AutoConnect);
	settings().setValue("synergyc", m_Synergyc);
	settings().setValue("synergys", m_Synergys);
	settings().setValue("screenName", m_ScreenName);
	settings().setValue("port", m_Port);
	settings().setValue("interface", m_Interface);
	settings().setValue("logLevelIndex", m_LogLevel);
}

