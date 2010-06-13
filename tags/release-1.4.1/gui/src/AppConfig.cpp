#include "AppConfig.h"

#include <QtCore>
#include <QtNetwork>

#if defined(Q_OS_WIN)
const char AppConfig::m_SynergysName[] = "synergys.exe";
const char AppConfig::m_SynergycName[] = "synergyc.exe";
const char AppConfig::m_SynergyProgramDir[] = "bin/";
const char AppConfig::m_SynergyLogDir[] = "log/";
#else
const char AppConfig::m_SynergysName[] = "synergys";
const char AppConfig::m_SynergycName[] = "synergyc";
const char AppConfig::m_SynergyProgramDir[] = "/usr/bin/";
const char AppConfig::m_SynergyLogDir[] = "/var/log/";
#endif

static const char* logLevelNames[] =
{
	"ERROR",
	"WARNING",
	"NOTE",
	"INFO",
	"DEBUG",
	"DEBUG1",
	"DEBUG2"
};

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

QString AppConfig::synergyLogDir()
{
#if defined(Q_OS_WIN)
	// on windows, we want to log to program files
	return QString(QDir::currentPath() + "/log/");
#else
	// on unix, we'll log to the standard log dir
	return "/var/log/";
#endif
}

void AppConfig::persistLogDir()
{
	QDir dir = synergyLogDir();

	// persist the log directory
	if (!dir.exists())
	{
		dir.mkpath(dir.path());
	}
}

QString AppConfig::logLevelText() const
{
	return logLevelNames[logLevel()];
}

void AppConfig::loadSettings()
{
	m_AutoConnect = settings().value("autoConnectChecked", false).toBool();
	m_Synergyc = settings().value("synergyc", QString(synergyProgramDir()) + synergycName()).toString();
	m_Synergys = settings().value("synergys", QString(synergyProgramDir()) + synergysName()).toString();
	m_ScreenName = settings().value("screenName", QHostInfo::localHostName()).toString();
	m_Port = settings().value("port", 24800).toInt();
	m_Interface = settings().value("interface").toString();
	m_LogLevel = settings().value("logLevel", 2).toInt();
	m_AutoDetectPaths = settings().value("autoDetectPaths", true).toBool();
	m_LogToFile = settings().value("logToFile", false).toBool();
	m_LogFilename = settings().value("logFilename", synergyLogDir() + "synergy.log").toString();
}

void AppConfig::saveSettings()
{
	settings().setValue("autoConnectChecked", m_AutoConnect);
	settings().setValue("synergyc", m_Synergyc);
	settings().setValue("synergys", m_Synergys);
	settings().setValue("screenName", m_ScreenName);
	settings().setValue("port", m_Port);
	settings().setValue("interface", m_Interface);
	settings().setValue("logLevel", m_LogLevel);
	settings().setValue("autoDetectPaths", m_AutoDetectPaths);
	settings().setValue("logToFile", m_LogToFile);
	settings().setValue("logFilename", m_LogFilename);
}

bool AppConfig::detectPath(const QString& name, QString& path)
{
	// look in current working dir and default dir
	QStringList searchDirs;
	searchDirs.append("./");
	searchDirs.append(synergyProgramDir());

	// use the first valid path we find
	for (int i = 0; i < searchDirs.length(); i++)
	{
		QFile f(searchDirs[i] + name);
		if (f.exists())
		{
			path = f.fileName();
			return true;
		}
	}

	// nothing found!
	return false;
}
