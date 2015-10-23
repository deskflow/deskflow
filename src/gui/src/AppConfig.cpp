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

#include "AppConfig.h"
#include "EditionType.h"
#include "QUtility.h"

#include <QtCore>
#include <QtNetwork>

#if defined(Q_OS_WIN)
const char AppConfig::m_SynergysName[] = "synergys.exe";
const char AppConfig::m_SynergycName[] = "synergyc.exe";
const char AppConfig::m_SynergyLogDir[] = "log/";
#define DEFAULT_PROCESS_MODE Service
#else
const char AppConfig::m_SynergysName[] = "synergys";
const char AppConfig::m_SynergycName[] = "synergyc";
const char AppConfig::m_SynergyLogDir[] = "/var/log/";
#define DEFAULT_PROCESS_MODE Desktop
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
	m_ScreenName(),
	m_Port(24800),
	m_Interface(),
	m_LogLevel(0),
	m_WizardLastRun(0),
	m_ProcessMode(DEFAULT_PROCESS_MODE),
	m_AutoConfig(true),
	m_ElevateMode(false),
	m_AutoConfigPrompted(false),
	m_CryptoEnabled(false),
	m_AutoHide(false)
{
	Q_ASSERT(m_pSettings);

	loadSettings();
}

AppConfig::~AppConfig()
{
	saveSettings();
}

QString AppConfig::synergyLogDir() const
{
#if defined(Q_OS_WIN)
	// on windows, we want to log to program files
	return synergyProgramDir() + "log/";
#else
	// on unix, we'll log to the standard log dir
	return "/var/log/";
#endif
}

QString AppConfig::synergyProgramDir() const
{
	// synergy binaries should be in the same dir.
	return QCoreApplication::applicationDirPath() + "/";
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

const QString AppConfig::logFilenameCmd() const
{
	QString filename = m_LogFilename;
#if defined(Q_OS_WIN)
	// wrap in quotes in case username contains spaces.
	filename = QString("\"%1\"").arg(filename);
#endif
	return filename;
}

QString AppConfig::logLevelText() const
{
	return logLevelNames[logLevel()];
}

void AppConfig::loadSettings()
{
	m_ScreenName = settings().value("screenName", QHostInfo::localHostName()).toString();
	m_Port = settings().value("port", 24800).toInt();
	m_Interface = settings().value("interface").toString();
	m_LogLevel = settings().value("logLevel", 3).toInt(); // level 3: INFO
	m_LogToFile = settings().value("logToFile", false).toBool();
	m_LogFilename = settings().value("logFilename", synergyLogDir() + "synergy.log").toString();
	m_WizardLastRun = settings().value("wizardLastRun", 0).toInt();
	m_Language = settings().value("language", QLocale::system().name()).toString();
	m_StartedBefore = settings().value("startedBefore", false).toBool();
	m_AutoConfig = settings().value("autoConfig", true).toBool();
	m_ElevateMode = settings().value("elevateMode", false).toBool();
	m_AutoConfigPrompted = settings().value("autoConfigPrompted", false).toBool();
	m_Edition = settings().value("edition", Unknown).toInt();
	m_ActivateEmail = settings().value("activateEmail", "").toString();
	m_UserToken = settings().value("userToken", "").toString();
	m_CryptoEnabled = settings().value("cryptoEnabled", false).toBool();
	m_AutoHide = settings().value("autoHide", false).toBool();
	m_Serialkey = settings().value("serialKey", "").toString();
}

void AppConfig::saveSettings()
{
	settings().setValue("screenName", m_ScreenName);
	settings().setValue("port", m_Port);
	settings().setValue("interface", m_Interface);
	settings().setValue("logLevel", m_LogLevel);
	settings().setValue("logToFile", m_LogToFile);
	settings().setValue("logFilename", m_LogFilename);
	settings().setValue("wizardLastRun", kWizardVersion);
	settings().setValue("language", m_Language);
	settings().setValue("startedBefore", m_StartedBefore);
	settings().setValue("autoConfig", m_AutoConfig);
	settings().setValue("elevateMode", m_ElevateMode);
	settings().setValue("autoConfigPrompted", m_AutoConfigPrompted);
	settings().setValue("edition", m_Edition);
	settings().setValue("activateEmail", m_ActivateEmail);
	settings().setValue("userToken", m_UserToken);
	settings().setValue("cryptoEnabled", m_CryptoEnabled);
	settings().setValue("autoHide", m_AutoHide);
	settings().setValue("serialKey", m_Serialkey);
}

void AppConfig::setAutoConfig(bool autoConfig)
{
	m_AutoConfig = autoConfig;
}

void AppConfig::setAutoConfigPrompted(bool prompted)
{
	m_AutoConfigPrompted = prompted;
}

bool AppConfig::elevateMode()
{
	return m_ElevateMode;
}
