/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
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

#include "QSynergyApplication.h"
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

const ElevateMode defaultElevateMode = ElevateAsNeeded;

const char* AppConfig::m_SynergySettingsName[] = {
        "screenName",
        "port",
        "interface",
        "logLevel2",
        "logToFile",
        "logFilename",
        "wizardLastRun",
        "language",
        "startedBefore",
        "autoConfig",
        "autoConfigServer",
        "elevateMode",
        "elevateModeEnum",
        "edition",
        "cryptoEnabled",
        "autoHide",
        "serialKey",
        "lastVersion",
        "lastExpiringWarningTime",
        "activationHasRun",
        "minimizeToTray"
};



static const char* logLevelNames[] =
{
    "INFO",
    "DEBUG",
    "DEBUG1",
    "DEBUG2"
};

AppConfig::AppConfig(QSettings* userSettings, QSettings* systemSettings) :
    m_pSettings(userSettings),
    m_ScreenName(),
    m_Port(24800),
    m_Interface(),
    m_LogLevel(0),
    m_WizardLastRun(0),
    m_ProcessMode(DEFAULT_PROCESS_MODE),
    m_AutoConfig(true),
    m_ElevateMode(defaultElevateMode),
    m_CryptoEnabled(false),
    m_AutoHide(false),
    m_LastExpiringWarningTime(0),
    m_AutoConfigServer(),
    m_MinimizeToTray(false)
{
    Q_ASSERT(m_pSettings);

    //If user setting dont exist but system ones do, load the system setting and save them to user settings
    if (!settingsExist(userSettings) && settingsExist(systemSettings))
    {
        m_pSettings = systemSettings;
        loadSettings();
        m_pSettings = userSettings;
        saveSettings();
    }

    loadSettings();
}

AppConfig::~AppConfig()
{
    saveSettings();
}

const QString &AppConfig::screenName() const { return m_ScreenName; }

int AppConfig::port() const { return m_Port; }

const QString &AppConfig::networkInterface() const { return m_Interface; }

int AppConfig::logLevel() const { return m_LogLevel; }

bool AppConfig::logToFile() const { return m_LogToFile; }

const QString &AppConfig::logFilename() const { return m_LogFilename; }

QString AppConfig::synergyLogDir() const
{
    // by default log to home dir
    return QDir::home().absolutePath() + "/";
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

ProcessMode AppConfig::processMode() const { return m_ProcessMode; }

bool AppConfig::wizardShouldRun() const { return m_WizardLastRun < kWizardVersion; }

const QString &AppConfig::language() const { return m_Language; }

bool AppConfig::startedBefore() const { return m_StartedBefore; }

bool AppConfig::autoConfig() const {
#ifndef SYNERGY_ENTERPRISE
    return m_AutoConfig;
#else
    // always disable auto config for enterprise edition.
    return false;
#endif
}

QString AppConfig::autoConfigServer() const { return m_AutoConfigServer; }

void AppConfig::loadSettings()
{
    m_ScreenName        = loadSetting(ScreenName, QHostInfo::localHostName()).toString();
    m_Port              = loadSetting(Port, 24800).toInt();
    m_Interface         = loadSetting(InterfaceSetting).toString();
    m_LogLevel          = loadSetting(LogLevel, 0).toInt();
    m_LogToFile         = loadSetting(LogToFile, false).toBool();
    m_LogFilename       = loadSetting(LogFilename,synergyLogDir() + "synergy.log").toString();
    m_WizardLastRun     = loadSetting(WizardLastRun,0).toInt();
    m_Language          = loadSetting(Language, QLocale::system().name()).toString();
    m_StartedBefore     = loadSetting(StartedBefore, false).toBool();
    m_AutoConfig        = loadSetting(AutoConfig, false).toBool();
    m_AutoConfigServer  = loadSetting(AutoConfigServer,"").toString();

    {   //Scope related code together
        // TODO Investigate why ElevateModeEnum isn't loaded fully
        QVariant elevateMode = loadSetting(ElevateModeEnum);
        if (!elevateMode.isValid()) {
            elevateMode = loadSetting(ElevateModeSetting,
                                           QVariant(static_cast<int>(defaultElevateMode)));
        }
        m_ElevateMode        = static_cast<ElevateMode>(elevateMode.toInt());
    }

    m_Edition                   = static_cast<Edition>(loadSetting(EditionSetting, kUnregistered).toInt());
    m_ActivateEmail             = loadSetting(ActivateEmail, "").toString();
    m_CryptoEnabled             = loadSetting(CryptoEnabled, true).toBool();
    m_AutoHide                  = loadSetting(AutoHide, false).toBool();
    m_Serialkey                 = loadSetting(SerialKey, "").toString().trimmed();
    m_lastVersion               = loadSetting(LastVersion, "Unknown").toString();
    m_LastExpiringWarningTime   = loadSetting(LastExpireWarningTime, 0).toInt();
    m_ActivationHasRun          = loadSetting(ActivationHasRun, false).toBool();
    m_MinimizeToTray            = loadSetting(MinimizeToTray, false).toBool();

}

void AppConfig::saveSettings()
{
    setSetting(ScreenName, m_ScreenName);
    setSetting(Port, m_Port);
    setSetting(InterfaceSetting, m_Interface);
    setSetting(LogLevel, m_LogLevel);
    setSetting(LogToFile, m_LogToFile);
    setSetting(LogFilename, m_LogFilename);
    setSetting(WizardLastRun, kWizardVersion);
    setSetting(Language, m_Language);
    setSetting(StartedBefore, m_StartedBefore);
    setSetting(AutoConfig, m_AutoConfig);
    setSetting(AutoConfigServer, m_AutoConfigServer);
    // Refer to enum ElevateMode declaration for insight in to why this
    // flag is mapped this way
    setSetting(ElevateModeSetting, m_ElevateMode == ElevateAlways);
    setSetting(ElevateModeEnum, static_cast<int>(m_ElevateMode));
    setSetting(EditionSetting, m_Edition);
    setSetting(CryptoEnabled, m_CryptoEnabled);
    setSetting(AutoHide, m_AutoHide);
    setSetting(SerialKey, m_Serialkey);
    setSetting(LastVersion, m_lastVersion);
    setSetting(LastExpireWarningTime, m_LastExpiringWarningTime);
    setSetting(ActivationHasRun, m_ActivationHasRun);
    setSetting(MinimizeToTray, m_MinimizeToTray);

    settings().sync();
}

#ifndef SYNERGY_ENTERPRISE
bool AppConfig::activationHasRun() const
{
    return m_ActivationHasRun;
}

AppConfig& AppConfig::activationHasRun(bool value)
{
    m_ActivationHasRun = value;
    return *this;
}
#endif

QString AppConfig::lastVersion() const
{
    return m_lastVersion;
}

void AppConfig::setLastVersion(QString version) {
    m_lastVersion = version;
}

QSettings &AppConfig::settings() { return *m_pSettings; }

void AppConfig::setScreenName(const QString &s) { m_ScreenName = s; }

void AppConfig::setPort(int i) { m_Port = i; }

void AppConfig::setNetworkInterface(const QString &s) { m_Interface = s; }

void AppConfig::setLogLevel(int i) { m_LogLevel = i; }

void AppConfig::setLogToFile(bool b) { m_LogToFile = b; }

void AppConfig::setLogFilename(const QString &s) { m_LogFilename = s; }

void AppConfig::setWizardHasRun() { m_WizardLastRun = kWizardVersion; }

void AppConfig::setLanguage(const QString language) { m_Language = language; }

void AppConfig::setStartedBefore(bool b) { m_StartedBefore = b; }

void AppConfig::setElevateMode(ElevateMode em) { m_ElevateMode = em; }

void AppConfig::setAutoConfig(bool autoConfig)
{
    m_AutoConfig = autoConfig;
}

void AppConfig::setAutoConfigServer(QString autoConfigServer)
{
    m_AutoConfigServer = autoConfigServer;
}

#ifndef SYNERGY_ENTERPRISE
void AppConfig::setEdition(Edition e) {
    m_Edition = e;
}

Edition AppConfig::edition() const { return m_Edition; }

QString AppConfig::setSerialKey(QString serial) {
    using std::swap;
    swap (serial, m_Serialkey);
    return serial;
}

void AppConfig::clearSerialKey()
{
    m_Serialkey.clear();
}

QString AppConfig::serialKey() { return m_Serialkey; }

int AppConfig::lastExpiringWarningTime() const { return m_LastExpiringWarningTime; }

void AppConfig::setLastExpiringWarningTime(int t) { m_LastExpiringWarningTime = t; }
#endif

QString AppConfig::synergysName() const { return m_SynergysName; }

QString AppConfig::synergycName() const { return m_SynergycName; }

ElevateMode AppConfig::elevateMode()
{
    return m_ElevateMode;
}

void AppConfig::setCryptoEnabled(bool e) {
    m_CryptoEnabled = e;
    emit sslToggled(e);
}

bool AppConfig::getCryptoEnabled() const {
    return
#ifndef SYNERGY_ENTERPRISE
    (edition() == kPro) &&
#endif
    m_CryptoEnabled;
}

void AppConfig::setAutoHide(bool b) { m_AutoHide = b; }

bool AppConfig::getAutoHide() { return m_AutoHide; }

void AppConfig::setMinimizeToTray(bool b) { m_MinimizeToTray = b; }

bool AppConfig::getMinimizeToTray() { return m_MinimizeToTray; }

bool AppConfig::settingsExist(QSettings* settings) {
    //Use screen name as the test to see if the settings have been saved to this location
    return settings->contains(settingName(ScreenName));
}

QString AppConfig::settingName(AppConfig::Setting name) {
    return m_SynergySettingsName[name];
}

template<typename T>
void AppConfig::setSetting(AppConfig::Setting name, T value) {
    settings().setValue(settingName(name), value);
}

QVariant AppConfig::loadSetting(AppConfig::Setting name, const QVariant& defaultValue) {
    return settings().value(settingName(name), defaultValue);
}
