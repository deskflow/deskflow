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
	m_AutoConnect(false),
	m_ScreenName(),
	m_Port(24800),
	m_Interface(),
	m_LogLevel(0),
	m_AutoStart(false),
	m_AutoHide(false),
	m_AutoStartPrompt(false),
	m_WizardLastRun(0),
	m_CryptoPass(),
	m_CryptoMode(),
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

void AppConfig::setAutoStart(bool b)
{
	m_AutoStart = b;

	// always create or delete the links/files/entries even if they exist already,
	// in case it was broken.

#if defined(Q_OS_LINUX)

	QString desktopFileName("synergy.desktop");
	QString desktopFilePath("/usr/share/applications/" + desktopFileName);
	QString autoStartPath(QDir::homePath() + "/.config/autostart/" + desktopFileName);

	if (b)
	{
		QFile::link(desktopFilePath, autoStartPath);
	}
	else
	{
		QFile::remove(autoStartPath);
	}

#elif defined(Q_OS_WIN)

	QSettings settings("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
	QString path("Synergy");

	if (b)
	{
		settings.setValue(path, QCoreApplication::applicationFilePath());
	}
	else
	{
		settings.remove(path);
	}
	settings.sync();

#endif

	// TODO: mac os x auto start
}

void AppConfig::loadSettings()
{
	m_AutoConnect = settings().value("autoConnect", false).toBool();
	m_ScreenName = settings().value("screenName", QHostInfo::localHostName()).toString();
	m_Port = settings().value("port", 24800).toInt();
	m_Interface = settings().value("interface").toString();
	m_LogLevel = settings().value("logLevel", 2).toInt();
	m_LogToFile = settings().value("logToFile", false).toBool();
	m_LogFilename = settings().value("logFilename", synergyLogDir() + "synergy.log").toString();
	m_AutoStart = settings().value("autoStart", false).toBool();
	m_AutoHide = settings().value("autoHide", true).toBool();
	m_AutoStartPrompt = settings().value("autoStartPrompt", true).toBool();
	m_WizardLastRun = settings().value("wizardLastRun", 0).toInt();
	m_ProcessMode = (ProcessMode)settings().value("processMode2", DEFAULT_PROCESS_MODE).toInt();
	m_CryptoPass = settings().value("cryptoPass", "").toString();
	m_CryptoMode = (CryptoMode)settings().value("cryptoMode", Disabled).toInt();
}

void AppConfig::saveSettings()
{
	settings().setValue("autoConnect", m_AutoConnect);
	settings().setValue("screenName", m_ScreenName);
	settings().setValue("port", m_Port);
	settings().setValue("interface", m_Interface);
	settings().setValue("logLevel", m_LogLevel);
	settings().setValue("logToFile", m_LogToFile);
	settings().setValue("logFilename", m_LogFilename);
	settings().setValue("autoStart", m_AutoStart);
	settings().setValue("autoHide", m_AutoHide);
	settings().setValue("autoStartPrompt", m_AutoStartPrompt);
	settings().setValue("wizardLastRun", kWizardVersion);
	settings().setValue("processMode2", m_ProcessMode);
	settings().setValue("cryptoPass", m_CryptoPass);
	settings().setValue("cryptoMode", m_CryptoMode);
}

QString AppConfig::hash(const QString& string)
{
	QByteArray data = string.toUtf8();
	QByteArray hash = QCryptographicHash::hash(data, QCryptographicHash::Md5);
	return hash.toHex();
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

QString AppConfig::cryptoModeString() const
{
	switch (cryptoMode())
	{
	case OFB:
		return "ofb";

	case CFB:
		return "cfb";

	case CTR:
		return "ctr";

	case GCM:
		return "gcm";

	default:
		qCritical() << "invalid crypto mode";
		return "";
	}
}
