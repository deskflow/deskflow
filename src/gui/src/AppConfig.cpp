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
#include <QtWidgets/QMessageBox>
#include <QPushButton>

#include "ConfigWriter.h"

#if defined(Q_OS_WIN)
const char AppConfig::m_SynergysName[] = "synergys.exe";
const char AppConfig::m_SynergycName[] = "synergyc.exe";
const char AppConfig::m_SynergyLogDir[] = "log/";
const char AppConfig::synergyConfigName[] = "synergy.sgc";
#define DEFAULT_PROCESS_MODE Service
#else
const char AppConfig::m_SynergysName[] = "synergys";
const char AppConfig::m_SynergycName[] = "synergyc";
const char AppConfig::m_SynergyLogDir[] = "/var/log/";
const char AppConfig::synergyConfigName[] = "synergy.conf";
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
        "minimizeToTray",
        "ActivateEmail",
        "loadFromSystemScope",
        "groupServerChecked",
        "useExternalConfig",
        "configFile",
        "useInternalConfig",
        "groupClientChecked",
        "serverHostname",
};

static const char* logLevelNames[] =
{
    "INFO",
    "DEBUG",
    "DEBUG1",
    "DEBUG2"
};

AppConfig::AppConfig() :
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
    m_MinimizeToTray(false),
    m_Edition(kUnregistered),
    m_LogToFile(),
    m_StartedBefore(),
    m_ActivationHasRun(),
    m_ServerGroupChecked(),
    m_UseExternalConfig(),
    m_UseInternalConfig(),
    m_ClientGroupChecked(),
    m_LoadFromSystemScope(),
{

    using GUI::Config::ConfigWriter;

    auto writer = ConfigWriter::make();

    //Register this class to receive global load and saves
    writer->registerClass(this);

    //User settings exist and the load from system scope variable is true
    if (writer->hasSetting(settingName(LoadSystemSettings), ConfigWriter::kUser) &&
        writer->loadSetting(settingName(LoadSystemSettings), false,ConfigWriter::kUser).toBool())
    {
        writer->setScope(ConfigWriter::kSystem);
    }
    //If user setting don't exist but system ones do, load the system settings
    else if (!writer->hasSetting(settingName(ScreenName), ConfigWriter::kUser) &&
        writer->hasSetting(settingName(ScreenName), ConfigWriter::kSystem))
    {
        writer->setScope(ConfigWriter::kSystem);
    } else { // Otherwise just load to user scope
        writer->setScope(ConfigWriter::kUser);
    }

    //setScope triggers a global load so no need to call it again

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
    m_LoadFromSystemScope       = loadSetting(LoadSystemSettings, false).toBool();
    m_ServerGroupChecked        = loadSetting(GroupServerCheck, false).toBool();
    m_UseExternalConfig         = loadSetting(UseExternalConfig, false).toBool();
    m_ConfigFile                = loadSetting(ConfigFile, QDir::homePath() + "/" + synergyConfigName).toString();
    m_UseInternalConfig         = loadSetting(UseInternalConfig, false).toBool();
    m_ClientGroupChecked        = loadSetting(GroupClientCheck, true).toBool();
    m_ServerHostname            = loadSetting(ServerHostname).toString();


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
    setSetting(LoadSystemSettings, m_LoadFromSystemScope);
    setSetting(GroupServerCheck, m_ServerGroupChecked);
    setSetting(UseExternalConfig, m_UseExternalConfig);
    setSetting(ConfigFile, m_ConfigFile);
    setSetting(UseInternalConfig, m_UseInternalConfig);
    setSetting(GroupClientCheck, m_ClientGroupChecked);
    setSetting(ServerHostname, m_ServerHostname);

    m_unsavedChanges = false;
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

void AppConfig::setLastVersion(const QString& version) {
    setSettingModified(m_lastVersion, version);
}

void AppConfig::setScreenName(const QString &s) {
    setSettingModified(m_ScreenName, s);
}

void AppConfig::setPort(int i) {
    setSettingModified(m_Port, i);
}

void AppConfig::setNetworkInterface(const QString &s) {
    setSettingModified(m_Interface, s);
}

void AppConfig::setLogLevel(int i) {
    setSettingModified(m_LogLevel, i);
}

void AppConfig::setLogToFile(bool b) {
    setSettingModified(m_LogToFile, b);
}

void AppConfig::setLogFilename(const QString &s) {
    setSettingModified(m_LogFilename, s);
}

void AppConfig::setWizardHasRun() {
    setSettingModified(m_WizardLastRun, kWizardVersion);
}

void AppConfig::setLanguage(const QString& language) {
    setSettingModified(m_Language, language);
}

void AppConfig::setStartedBefore(bool b) {
    setSettingModified(m_StartedBefore, b);
}

void AppConfig::setElevateMode(ElevateMode em) {
    setSettingModified(m_ElevateMode, em);
}

void AppConfig::setAutoConfig(bool autoConfig)
{
    setSettingModified(m_AutoConfig, autoConfig);
    emit zeroConfToggled();
}

void AppConfig::setAutoConfigServer(const QString& autoConfigServer)
{
    setSettingModified(m_AutoConfigServer, autoConfigServer);
}

#ifndef SYNERGY_ENTERPRISE
void AppConfig::setEdition(Edition e) {
    setSettingModified(m_Edition, e);
}

Edition AppConfig::edition() const { return m_Edition; }

void AppConfig::setSerialKey(const QString& serial) {
    setSettingModified(m_Serialkey, serial);
}

void AppConfig::clearSerialKey()
{
    m_Serialkey.clear();
}

QString AppConfig::serialKey() { return m_Serialkey; }

int AppConfig::lastExpiringWarningTime() const { return m_LastExpiringWarningTime; }

void AppConfig::setLastExpiringWarningTime(int newValue) {
    setSettingModified(m_LastExpiringWarningTime, newValue);
}
#endif

QString AppConfig::synergysName() const { return m_SynergysName; }

QString AppConfig::synergycName() const { return m_SynergycName; }

ElevateMode AppConfig::elevateMode()
{
    return m_ElevateMode;
}

void AppConfig::setCryptoEnabled(bool newValue) {
    setSettingModified(m_CryptoEnabled, newValue);
    emit sslToggled(m_CryptoEnabled);
}

bool AppConfig::getCryptoEnabled() const {
    return
#ifndef SYNERGY_ENTERPRISE
    (edition() == kPro) &&
#endif
    m_CryptoEnabled;
}

void AppConfig::setAutoHide(bool b) {
    setSettingModified(m_MinimizeToTray, b);
}

bool AppConfig::getAutoHide() { return m_AutoHide; }

void AppConfig::setMinimizeToTray(bool newValue) {
    setSettingModified(m_MinimizeToTray, newValue);
}

bool AppConfig::getMinimizeToTray() { return m_MinimizeToTray; }

QString AppConfig::settingName(AppConfig::Setting name) {
    return m_SynergySettingsName[name];
}

template<typename T>
void AppConfig::setSetting(AppConfig::Setting name, T value) {
    using GUI::Config::ConfigWriter;
    ConfigWriter::make()->setSetting(settingName(name), value);
}

QVariant AppConfig::loadSetting(AppConfig::Setting name, const QVariant& defaultValue) {
    using GUI::Config::ConfigWriter;
    return ConfigWriter::make()->loadSetting(settingName(name), defaultValue);
}


void AppConfig::setLoadFromSystemScope(bool value) {
    using GUI::Config::ConfigWriter;

    auto writer = ConfigWriter::make();

    if (value && writer->getScope() != ConfigWriter::kSystem)
    {
        m_LoadFromSystemScope = value;
        writer->globalSave();     //Save user prefs
        writer->setScope(ConfigWriter::kSystem);   //Switch the the System Scope and reload

    }
    else if (!value && writer->getScope() == ConfigWriter::kSystem)
    {
        writer->setScope(ConfigWriter::kUser);      // Switch to UserScope
        m_LoadFromSystemScope = value;     // Set the user pref
        saveSettings();                    // Save user prefs
    }
}

bool AppConfig::isSystemScoped() const {
    return GUI::Config::ConfigWriter::make()->getScope() == GUI::Config::ConfigWriter::kSystem;
}

bool AppConfig::getServerGroupChecked() const {
    return m_ServerGroupChecked;
}

bool AppConfig::getUseExternalConfig() const {
    return m_UseExternalConfig;
}

QString AppConfig::getConfigFile() const {
    return m_ConfigFile;
}

bool AppConfig::getUseInternalConfig() const {
    return m_UseInternalConfig;
}

bool AppConfig::getClientGroupChecked() const {
    return m_ClientGroupChecked;
}

QString AppConfig::getServerHostname() const {
    return m_ServerHostname;
}

void AppConfig::setServerGroupChecked(bool newValue) {
    setSettingModified(m_ServerGroupChecked, newValue);
}

void AppConfig::setUseExternalConfig(bool newValue) {
    setSettingModified(m_UseExternalConfig, newValue);
}

void AppConfig::setConfigFile(const QString& newValue) {
    setSettingModified(m_ConfigFile, newValue);
}

void AppConfig::setUseInternalConfig(bool newValue) {
    setSettingModified(m_UseInternalConfig, newValue);
}

void AppConfig::setClientGroupChecked(bool newValue) {
    setSettingModified(m_ClientGroupChecked, newValue);
}

void AppConfig::setServerHostname(const QString& newValue) {
    setSettingModified(m_ServerHostname, newValue);
}

template<typename T>
void AppConfig::setSettingModified(T &variable, const T& newValue) {
    if (variable != newValue)
    {
        variable = newValue;
        m_unsavedChanges = true;
    }
}

