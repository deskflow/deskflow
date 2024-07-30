/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Symless Ltd.
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

#include "Config.h"
#include "gui/constants.h"

#include <QApplication>
#include <QMessageBox>
#include <QPushButton>
#include <QtCore>
#include <QtNetwork>

using synergy::gui::Config;

// this should be incremented each time the wizard is changed,
// which will force it to re-run for existing installations.
const int kWizardVersion = 8;

#if defined(Q_OS_WIN)
const char AppConfig::m_CoreServerName[] = "synergys.exe";
const char AppConfig::m_CoreClientName[] = "synergyc.exe";
const char AppConfig::m_LogDir[] = "log/";
const char AppConfig::m_ConfigFilename[] = "synergy.sgc";
#else
const char AppConfig::m_CoreServerName[] = "synergys";
const char AppConfig::m_CoreClientName[] = "synergyc";
const char AppConfig::m_LogDir[] = "/var/log/";
const char AppConfig::m_ConfigFilename[] = "synergy.conf";
#endif

// TODO: instead, use key value pair table, which would be less fragile.
const char *const AppConfig::m_SettingsName[] = {
    "screenName",
    "port",
    "interface",
    "logLevel2",
    "logToFile",
    "logFilename",
    "wizardLastRun",
    "startedBefore",
    "elevateMode",
    "elevateModeEnum",
    "",              // 10 = edition, obsolete (using serial key instead)
    "cryptoEnabled", // kTlsEnabled (retain legacy string value)
    "autoHide",
    "serialKey",
    "lastVersion",
    "", // 15 = lastExpiringWarningTime, obsolete
    "activationHasRun",
    "", // 17 = minimizeToTray, obsolete
    "", // 18 = ActivateEmail, obsolete
    "loadFromSystemScope",
    "groupServerChecked", // kServerGroupChecked
    "useExternalConfig",
    "configFile",
    "useInternalConfig",
    "groupClientChecked",
    "serverHostname",
    "tlsCertPath",
    "tlsKeyLength",
    "preventSleep",
    "languageSync",
    "invertScrollDirection",
    "", // 31 = guid, obsolete
    "", // 32 = licenseRegistryUrl, obsolete
    "licenseNextCheck",
    "initiateConnectionFromServer", // kInvertConnection
    "",                             // 35 = clientHostMode, obsolete
    "",                             // 36 = serverClientMode, obsolete
    "serviceEnabled",
    "closeToTray"};

static const char *logLevelNames[] = {"INFO", "DEBUG", "DEBUG1", "DEBUG2"};

AppConfig::AppConfig() { m_Config.registerReceiever(this); }

void AppConfig::loadAllScopes() {
  m_Config.loadAll();

  // User settings exist and the load from system scope variable is true
  if (m_Config.hasSetting(
          settingName(Setting::kLoadSystemSettings), Config::Scope::User)) {
    setLoadFromSystemScope(m_LoadFromSystemScope);
  }
  // If user setting don't exist but system ones do, load the system settings
  else if (m_Config.hasSetting(
               settingName(Setting::kScreenName), Config::Scope::System)) {
    setLoadFromSystemScope(true);
  }
}

void AppConfig::loadSettings() {
  using enum AppConfig::Setting;

  qDebug("loading settings");

  m_ScreenName =
      loadSetting(kScreenName, QHostInfo::localHostName()).toString();
  if (m_ScreenName.isEmpty()) {
    m_ScreenName = QHostInfo::localHostName();
  }

  m_Port = loadSetting(kPort, 24800).toInt();
  m_Interface = loadSetting(kInterface).toString();
  m_LogLevel = loadSetting(kLogLevel, 0).toInt();
  m_LogToFile = loadSetting(kLogToFile, false).toBool();
  m_LogFilename =
      loadSetting(kLogFilename, logDir() + "synergy.log").toString();
  m_WizardLastRun = loadCommonSetting(kWizardLastRun, 0).toInt();
  m_StartedBefore = loadSetting(kStartedBefore, false).toBool();

  QVariant elevateMode = loadSetting(kElevateModeEnum);
  if (!elevateMode.isValid()) {
    elevateMode = loadSetting(
        kElevateMode, QVariant(static_cast<int>(kDefaultElevateMode)));
  }
  m_ElevateMode = static_cast<ElevateMode>(elevateMode.toInt());

  m_AutoHide = loadSetting(kAutoHide, false).toBool();
  m_LastVersion = loadSetting(kLastVersion, "Unknown").toString();
  m_ActivationHasRun = loadSetting(kActivationHasRun, false).toBool();
  m_LoadFromSystemScope =
      loadCommonSetting(kLoadSystemSettings, false).toBool();
  m_ServerGroupChecked = loadSetting(kServerGroupChecked, false).toBool();
  m_UseExternalConfig = loadSetting(kUseExternalConfig, false).toBool();
  m_ConfigFile =
      loadSetting(kConfigFile, QDir::homePath() + "/" + m_ConfigFilename)
          .toString();
  m_UseInternalConfig = loadSetting(kUseInternalConfig, false).toBool();
  m_ClientGroupChecked = loadSetting(kClientGroupChecked, false).toBool();
  m_ServerHostname = loadSetting(kServerHostname).toString();
  m_PreventSleep = loadSetting(kPreventSleep, false).toBool();
  m_LanguageSync = loadSetting(kLanguageSync, false).toBool();
  m_InvertScrollDirection = loadSetting(kInvertScrollDirection, false).toBool();
  m_licenseNextCheck = loadCommonSetting(kLicenseNextCheck, 0).toULongLong();
  m_InvertConnection = loadSetting(kInvertConnection, false).toBool();

  // only change the serial key if the settings being loaded contains a key
  bool loadSerial = m_Config.hasSetting(
      settingName(kLoadSystemSettings), Config::Scope::Current);

  if (loadSerial) {
    const auto &serialKey = loadSetting(kSerialKey, "").toString().trimmed();
    if (!serialKey.isEmpty()) {
      m_SerialKey = serialKey;
    }
  }

  m_ServiceEnabled = loadSetting(kServiceEnabled, m_ServiceEnabled).toBool();
  m_CloseToTray = loadSetting(kCloseToTray, m_CloseToTray).toBool();
  m_TlsEnabled = loadSetting(kTlsEnabled, m_TlsEnabled).toBool();
  m_TlsCertPath = loadSetting(kTlsCertPath, defaultTlsCertPath()).toString();
  m_TlsKeyLength = loadSetting(kTlsKeyLength, m_TlsKeyLength).toString();

  emit loaded();
}

void AppConfig::saveSettings() {
  using enum Setting;

  qDebug("saving settings");

  setCommonSetting(kWizardLastRun, m_WizardLastRun);
  setCommonSetting(kLoadSystemSettings, m_LoadFromSystemScope);
  setCommonSetting(kClientGroupChecked, m_ClientGroupChecked);
  setCommonSetting(kServerGroupChecked, m_ServerGroupChecked);
  setCommonSetting(kLicenseNextCheck, m_licenseNextCheck);

  if (isWritable()) {
    setSetting(kScreenName, m_ScreenName);
    setSetting(kPort, m_Port);
    setSetting(kInterface, m_Interface);
    setSetting(kLogLevel, m_LogLevel);
    setSetting(kLogToFile, m_LogToFile);
    setSetting(kLogFilename, m_LogFilename);
    setSetting(kStartedBefore, m_StartedBefore);
    setSetting(kElevateModeEnum, static_cast<int>(m_ElevateMode));
    setSetting(kTlsEnabled, m_TlsEnabled);
    setSetting(kAutoHide, m_AutoHide);
    setSetting(kSerialKey, m_SerialKey);
    setSetting(kLastVersion, m_LastVersion);
    setSetting(kActivationHasRun, m_ActivationHasRun);
    setSetting(kUseExternalConfig, m_UseExternalConfig);
    setSetting(kConfigFile, m_ConfigFile);
    setSetting(kUseInternalConfig, m_UseInternalConfig);
    setSetting(kServerHostname, m_ServerHostname);
    setSetting(kPreventSleep, m_PreventSleep);
    setSetting(kLanguageSync, m_LanguageSync);
    setSetting(kInvertScrollDirection, m_InvertScrollDirection);
    setSetting(kInvertConnection, m_InvertConnection);
    setSetting(kServiceEnabled, m_ServiceEnabled);
    setSetting(kCloseToTray, m_CloseToTray);

    // See enum ElevateMode declaration to understand why this setting is bool
    setSetting(kElevateMode, m_ElevateMode == ElevateAlways);
  }

  setModified(false);
  saved();

  if (m_TlsChanged) {
    m_TlsChanged = false;
    emit tlsChanged();
  }
}

QString AppConfig::defaultTlsCertPath() const {
  QDir path(m_CoreInterface.getProfileDir());
  path = path.filePath("SSL");
  path = path.filePath("Synergy.pem");
  return path.absolutePath();
}

QString AppConfig::settingName(Setting name) {
  auto index = static_cast<int>(name);
  return m_SettingsName[index];
}

template <typename T> void AppConfig::setSetting(Setting name, T value) {
  m_Config.setSetting(settingName(name), value);
}

template <typename T> void AppConfig::setCommonSetting(Setting name, T value) {
  m_Config.setSetting(settingName(name), value, Config::Scope::User);
  m_Config.setSetting(settingName(name), value, Config::Scope::System);
}

QVariant AppConfig::loadSetting(Setting name, const QVariant &defaultValue) {
  return m_Config.loadSetting(settingName(name), defaultValue);
}

QVariant
AppConfig::loadCommonSetting(Setting name, const QVariant &defaultValue) const {
  QVariant result(defaultValue);
  QString setting(settingName(name));

  if (m_Config.hasSetting(setting)) {
    result = m_Config.loadSetting(setting, defaultValue);
  } else if (m_Config.getScope() == Config::Scope::System) {
    if (m_Config.hasSetting(setting, Config::Scope::User)) {
      result = m_Config.loadSetting(setting, defaultValue, Config::Scope::User);
    }
  } else if (m_Config.hasSetting(setting, Config::Scope::System)) {
    result = m_Config.loadSetting(setting, defaultValue, Config::Scope::System);
  }

  return result;
}

void AppConfig::loadScope(Config::Scope scope) {

  if (m_Config.getScope() != scope) {
    setDefaultValues();
    m_Config.setScope(scope);
    if (m_Config.hasSetting(
            settingName(Setting::kScreenName), m_Config.getScope())) {
      // If the user already has settings, then load them up now.
      m_Config.loadAll();
    }
  }
}

void AppConfig::setDefaultValues() { m_InvertConnection = false; }

void AppConfig::setLoadFromSystemScope(bool value) {

  if (value) {
    qDebug("loading system settings scope");
    loadScope(Config::Scope::System);
  } else {
    qDebug("loading user settings scope");
    loadScope(Config::Scope::User);
  }

  /*
   * It's very imprortant to set this variable after loadScope
   * because during scope loading this variable can be rewritten with old value
   */
  m_LoadFromSystemScope = value;
}

bool AppConfig::isWritable() const { return m_Config.isWritable(); }

bool AppConfig::isSystemScoped() const {
  return m_Config.getScope() == Config::Scope::System;
}

template <typename T>
void AppConfig::setSettingModified(T &variable, const T &newValue) {
  if (variable != newValue) {
    variable = newValue;
    setModified(true);
  }
}
QString AppConfig::logDir() const {
  // by default log to home dir
  return QDir::home().absolutePath() + "/";
}

void AppConfig::persistLogDir() const {
  QDir dir = logDir();

  // persist the log directory
  if (!dir.exists()) {
    dir.mkpath(dir.path());
  }
}

///////////////////////////////////////////////////////////////////////////////
// Begin getters
///////////////////////////////////////////////////////////////////////////////

bool AppConfig::activationHasRun() const { return m_ActivationHasRun; }

QString AppConfig::serialKey() const { return m_SerialKey; }

Config &AppConfig::config() { return m_Config; }

const QString &AppConfig::screenName() const { return m_ScreenName; }

int AppConfig::port() const { return m_Port; }

const QString &AppConfig::networkInterface() const { return m_Interface; }

int AppConfig::logLevel() const { return m_LogLevel; }

bool AppConfig::logToFile() const { return m_LogToFile; }

const QString &AppConfig::logFilename() const { return m_LogFilename; }

QString AppConfig::logLevelText() const { return logLevelNames[logLevel()]; }

ProcessMode AppConfig::processMode() const {
  return m_ServiceEnabled ? ProcessMode::kService : ProcessMode::kDesktop;
}

bool AppConfig::wizardShouldRun() const {
  return m_WizardLastRun < kWizardVersion;
}

bool AppConfig::startedBefore() const { return m_StartedBefore; }

QString AppConfig::lastVersion() const { return m_LastVersion; }

QString AppConfig::coreServerName() const { return m_CoreServerName; }

QString AppConfig::coreClientName() const { return m_CoreClientName; }

ElevateMode AppConfig::elevateMode() const { return m_ElevateMode; }

bool AppConfig::tlsEnabled() const { return m_TlsEnabled; }

bool AppConfig::autoHide() const { return m_AutoHide; }

bool AppConfig::invertScrollDirection() const {
  return m_InvertScrollDirection;
}

unsigned long long AppConfig::licenseNextCheck() const {
  return m_licenseNextCheck;
}

bool AppConfig::languageSync() const { return m_LanguageSync; }

bool AppConfig::preventSleep() const { return m_PreventSleep; }

bool AppConfig::invertConnection() const { return m_InvertConnection; }

QString AppConfig::tlsCertPath() const { return m_TlsCertPath; }

QString AppConfig::tlsKeyLength() const { return m_TlsKeyLength; }

bool AppConfig::serviceEnabled() const { return m_ServiceEnabled; }

bool AppConfig::closeToTray() const { return m_CloseToTray; }

bool AppConfig::serverGroupChecked() const { return m_ServerGroupChecked; }

bool AppConfig::useExternalConfig() const { return m_UseExternalConfig; }

const QString &AppConfig::configFile() const { return m_ConfigFile; }

bool AppConfig::useInternalConfig() const { return m_UseInternalConfig; }

bool AppConfig::clientGroupChecked() const { return m_ClientGroupChecked; }

QString AppConfig::serverHostname() const { return m_ServerHostname; }

void AppConfig::setActivationHasRun(bool value) { m_ActivationHasRun = value; }

///////////////////////////////////////////////////////////////////////////////
// End getters
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Begin setters
///////////////////////////////////////////////////////////////////////////////

void AppConfig::clearSerialKey() { m_SerialKey.clear(); }

void AppConfig::setTlsEnabled(bool value) {
  setSettingModified(m_TlsEnabled, value);
  m_TlsChanged = true;
}

void AppConfig::setTlsCertPath(const QString &value) {
  setSettingModified(m_TlsCertPath, value);
  m_TlsChanged = true;
}

void AppConfig::setTlsKeyLength(const QString &value) {
  setSettingModified(m_TlsKeyLength, value);
  m_TlsChanged = true;
}
void AppConfig::setSerialKey(const QString &serialKey) {
  setSettingModified(m_SerialKey, serialKey);
  setCommonSetting(Setting::kSerialKey, m_SerialKey);
}
void AppConfig::setServerGroupChecked(bool newValue) {
  setSettingModified(m_ServerGroupChecked, newValue);
}

void AppConfig::setUseExternalConfig(bool newValue) {
  setSettingModified(m_UseExternalConfig, newValue);
}

void AppConfig::setConfigFile(const QString &newValue) {
  setSettingModified(m_ConfigFile, newValue);
}

void AppConfig::setUseInternalConfig(bool newValue) {
  setSettingModified(m_UseInternalConfig, newValue);
}

void AppConfig::setClientGroupChecked(bool newValue) {
  setSettingModified(m_ClientGroupChecked, newValue);
}

void AppConfig::setServerHostname(const QString &newValue) {
  setSettingModified(m_ServerHostname, newValue);
}
void AppConfig::setLastVersion(const QString &version) {
  setSettingModified(m_LastVersion, version);
}

void AppConfig::setScreenName(const QString &s) {
  setSettingModified(m_ScreenName, s);
  emit screenNameChanged();
}

void AppConfig::setPort(int i) { setSettingModified(m_Port, i); }

void AppConfig::setNetworkInterface(const QString &s) {
  setSettingModified(m_Interface, s);
}

void AppConfig::setLogLevel(int i) { setSettingModified(m_LogLevel, i); }

void AppConfig::setLogToFile(bool b) { setSettingModified(m_LogToFile, b); }

void AppConfig::setLogFilename(const QString &s) {
  setSettingModified(m_LogFilename, s);
}

void AppConfig::setWizardHasRun() {
  setSettingModified(m_WizardLastRun, kWizardVersion);
}

void AppConfig::setStartedBefore(bool b) {
  setSettingModified(m_StartedBefore, b);
}

void AppConfig::setElevateMode(ElevateMode em) {
  setSettingModified(m_ElevateMode, em);
}

void AppConfig::setAutoHide(bool b) { setSettingModified(m_AutoHide, b); }

void AppConfig::setLicenseNextCheck(unsigned long long time) {
  setSettingModified(m_licenseNextCheck, time);
}

void AppConfig::setInvertScrollDirection(bool newValue) {
  setSettingModified(m_InvertScrollDirection, newValue);
}

void AppConfig::setLanguageSync(bool newValue) {
  setSettingModified(m_LanguageSync, newValue);
}

void AppConfig::setPreventSleep(bool newValue) {
  setSettingModified(m_PreventSleep, newValue);
}

void AppConfig::setServiceEnabled(bool enabled) {
  setSettingModified(m_ServiceEnabled, enabled);
}

void AppConfig::setCloseToTray(bool minimize) {
  setSettingModified(m_CloseToTray, minimize);
}

void AppConfig::setInvertConnection(bool value) {
  setSettingModified(m_InvertConnection, value);
  emit invertConnectionChanged();
}

///////////////////////////////////////////////////////////////////////////////
// End setters
///////////////////////////////////////////////////////////////////////////////
