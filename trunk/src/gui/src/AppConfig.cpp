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

#include "AppConfig.h"
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
	m_CryptoPass(),
	m_ProcessMode(DEFAULT_PROCESS_MODE)
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
	m_CryptoPass = settings().value("cryptoPass", "").toString();
	m_CryptoEnabled = settings().value("cryptoEnabled", false).toBool();
	m_Language = settings().value("language", QLocale::system().name()).toString();
	m_PremiumEmail = settings().value("premiumEmail", "").toString();
	m_PremiumToken = settings().value("premiumToken", "").toString();
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
	settings().setValue("cryptoPass", m_CryptoPass);
	settings().setValue("cryptoEnabled", m_CryptoEnabled);
	settings().setValue("language", m_Language);
	settings().setValue("premiumEmail", m_PremiumEmail);
	settings().setValue("premiumToken", m_PremiumToken);
}

void AppConfig::setCryptoPass(const QString &s)
{
	// clear field to user doesn't get confused.
	if (s.isEmpty())
	{
		m_CryptoPass.clear();
		return;
	}

	// only hash if password changes -- don't re-hash the hash.
	if (m_CryptoPass != s)
	{
		m_CryptoPass = hash(s);
	}
}

bool AppConfig::isPremium()
{
	QString hashSrc = m_PremiumEmail + getFirstMacAddress();
	QString hashResult = hash(hashSrc);
	return hashResult == m_PremiumToken;
}
