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
#include "SslCertificate.h"

#include <QApplication>
#include <QPushButton>
#include <QtCore>
#include <QtNetwork>
#include <QtWidgets/QMessageBox>
#include <qapplication.h>

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
    "tlsCertPath",
    "tlsKeyLength",
    "preventSleep",
    "languageSync",
    "invertScrollDirection",
    "guid",
    "licenseRegistryUrl",
    "licenseNextCheck",
    "initiateConnectionFromServer",
    "clientHostMode",
    "serverClientMode",
    "serviceEnabled",
    "closeToTray"};

static const char *logLevelNames[] = {"INFO", "DEBUG", "DEBUG1", "DEBUG2"};

AppConfig::AppConfig() {
  m_Config.registerReceiever(this);
  load();
}

void AppConfig::load() {
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

  m_ScreenName =
      loadSetting(kScreenName, QHostInfo::localHostName()).toString();
  if (m_ScreenName.isEmpty()) {
    m_ScreenName = QHostInfo::localHostName();
  }

  m_Port = loadSetting(kPort, 24800).toInt();
  m_Interface = loadSetting(kInterfaceSetting).toString();
  m_LogLevel = loadSetting(kLogLevel, 0).toInt();
  m_LogToFile = loadSetting(kLogToFile, false).toBool();
  m_LogFilename =
      loadSetting(kLogFilename, logDir() + "synergy.log").toString();
  m_WizardLastRun = loadCommonSetting(kWizardLastRun, 0).toInt();
  m_StartedBefore = loadSetting(kStartedBefore, false).toBool();

  QVariant elevateMode = loadSetting(kElevateModeEnum);
  if (!elevateMode.isValid()) {
    elevateMode = loadSetting(
        kElevateModeSetting, QVariant(static_cast<int>(kDefaultElevateMode)));
  }
  m_ElevateMode = static_cast<ElevateMode>(elevateMode.toInt());

  m_ActivateEmail = loadSetting(kActivateEmail, "").toString();
  m_CryptoEnabled = loadSetting(kCryptoEnabled, true).toBool();
  m_AutoHide = loadSetting(kAutoHide, false).toBool();
  m_LastVersion = loadSetting(kLastVersion, "Unknown").toString();
  m_LastExpiringWarningTime = loadSetting(kLastExpireWarningTime, 0).toInt();
  m_ActivationHasRun = loadSetting(kActivationHasRun, false).toBool();
  m_MinimizeToTray = loadSetting(kMinimizeToTray, false).toBool();
  m_LoadFromSystemScope =
      loadCommonSetting(kLoadSystemSettings, false).toBool();
  m_ServerGroupChecked = loadSetting(kGroupServerCheck, false).toBool();
  m_UseExternalConfig = loadSetting(kUseExternalConfig, false).toBool();
  m_ConfigFile =
      loadSetting(kConfigFile, QDir::homePath() + "/" + m_ConfigFilename)
          .toString();
  m_UseInternalConfig = loadSetting(kUseInternalConfig, false).toBool();
  m_ClientGroupChecked = loadSetting(kGroupClientCheck, false).toBool();
  m_ServerHostname = loadSetting(kServerHostname).toString();
  m_PreventSleep = loadSetting(kPreventSleep, false).toBool();
  m_LanguageSync = loadSetting(kLanguageSync, false).toBool();
  m_InvertScrollDirection = loadSetting(kInvertScrollDirection, false).toBool();
  m_Guid = loadCommonSetting(kGuid, QUuid::createUuid()).toString();
  m_licenseNextCheck = loadCommonSetting(kLicenseNextCheck, 0).toULongLong();
  m_ClientHostMode = loadSetting(kClientHostMode, true).toBool();
  m_ServerClientMode = loadSetting(kServerClientMode, true).toBool();
  m_InitiateConnectionFromServer =
      loadSetting(kInitiateConnectionFromServer, false).toBool();

  // only change the serial key if the settings being loaded contains a key
  bool updateSerial = m_Config.hasSetting(
      settingName(kLoadSystemSettings), Config::Scope::Current);
  // if the setting exists and is not empty
  updateSerial = updateSerial &&
                 !loadSetting(kSerialKey, "").toString().trimmed().isEmpty();

  if (updateSerial) {
    m_Serialkey = loadSetting(kSerialKey, "").toString().trimmed();
    m_Edition = static_cast<Edition>(
        loadSetting(kEditionSetting, kUnregistered).toInt());
  }

  m_ServiceEnabled = loadSetting(kServiceEnabled, m_ServiceEnabled).toBool();
  m_CloseToTray = loadSetting(kCloseToTray, m_CloseToTray).toBool();

  try {
    // Set the default path of the TLS certificate file in the users DIR
    QString certificateFilename =
        QString("%1/%2/%3")
            .arg(m_CoreInterface.getProfileDir(), "SSL", "Synergy.pem");

    m_TlsCertPath = loadSetting(kTlsCertPath, certificateFilename).toString();
    m_TlsKeyLength = loadSetting(kTlsKeyLength, "2048").toString();
  } catch (const std::exception &e) {
    qDebug() << e.what();
    qFatal("Failed to get profile dir, unable to configure TLS");
  }

  if (cryptoEnabled()) {
    generateCertificate();
  }

  applyAppSettings();
}

void AppConfig::saveSettings() {
  using enum Setting;

  setCommonSetting(kWizardLastRun, m_WizardLastRun);
  setCommonSetting(kLoadSystemSettings, m_LoadFromSystemScope);
  setCommonSetting(kGroupClientCheck, m_ClientGroupChecked);
  setCommonSetting(kGroupServerCheck, m_ServerGroupChecked);
  setCommonSetting(kGuid, m_Guid);
  setCommonSetting(kLicenseNextCheck, m_licenseNextCheck);

  if (isWritable()) {
    setSetting(kScreenName, m_ScreenName);
    setSetting(kPort, m_Port);
    setSetting(kInterfaceSetting, m_Interface);
    setSetting(kLogLevel, m_LogLevel);
    setSetting(kLogToFile, m_LogToFile);
    setSetting(kLogFilename, m_LogFilename);
    setSetting(kStartedBefore, m_StartedBefore);
    setSetting(kElevateModeEnum, static_cast<int>(m_ElevateMode));
    setSetting(kEditionSetting, m_Edition);
    setSetting(kCryptoEnabled, m_CryptoEnabled);
    setSetting(kAutoHide, m_AutoHide);
    setSetting(kSerialKey, m_Serialkey);
    setSetting(kLastVersion, m_LastVersion);
    setSetting(kLastExpireWarningTime, m_LastExpiringWarningTime);
    setSetting(kActivationHasRun, m_ActivationHasRun);
    setSetting(kMinimizeToTray, m_MinimizeToTray);
    setSetting(kUseExternalConfig, m_UseExternalConfig);
    setSetting(kConfigFile, m_ConfigFile);
    setSetting(kUseInternalConfig, m_UseInternalConfig);
    setSetting(kServerHostname, m_ServerHostname);
    setSetting(kPreventSleep, m_PreventSleep);
    setSetting(kLanguageSync, m_LanguageSync);
    setSetting(kInvertScrollDirection, m_InvertScrollDirection);
    setSetting(kClientHostMode, m_ClientHostMode);
    setSetting(kServerClientMode, m_ServerClientMode);
    setSetting(kServiceEnabled, m_ServiceEnabled);
    setSetting(kCloseToTray, m_CloseToTray);

    // See enum ElevateMode declaration to understand why this setting is bool
    setSetting(kElevateModeSetting, m_ElevateMode == ElevateAlways);
  }

  setModified(false);
  applyAppSettings();
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

void AppConfig::setDefaultValues() { m_InitiateConnectionFromServer = false; }

void AppConfig::setLoadFromSystemScope(bool value) {

  if (value) {
    loadScope(Config::Scope::System);
  } else {
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

void AppConfig::generateCertificate(bool forceGeneration) const {
  try {
    SslCertificate sslCertificate;
    sslCertificate.generateCertificate(
        tlsCertPath(), tlsKeyLength(), forceGeneration);
    emit sslToggled();
  } catch (const std::exception &e) {
    qDebug() << e.what();
    qFatal("Failed to configure TLS");
  }
}

void AppConfig::applyAppSettings() const {
  QApplication::setQuitOnLastWindowClosed(!m_CloseToTray);
}

///////////////////////////////////////////////////////////////////////////////
// Begin getters and setters
///////////////////////////////////////////////////////////////////////////////

bool AppConfig::activationHasRun() const { return m_ActivationHasRun; }

void AppConfig::setActivationHasRun(bool value) { m_ActivationHasRun = value; }

void AppConfig::setEdition(Edition e) {
  setSettingModified(m_Edition, e);
  setCommonSetting(Setting::kEditionSetting, m_Edition);
}

Edition AppConfig::edition() const { return m_Edition; }

void AppConfig::setSerialKey(const QString &serial) {
  setSettingModified(m_Serialkey, serial);
  setCommonSetting(Setting::kSerialKey, m_Serialkey);
}

void AppConfig::clearSerialKey() { m_Serialkey.clear(); }

QString AppConfig::serialKey() const { return m_Serialkey; }

int AppConfig::lastExpiringWarningTime() const {
  return m_LastExpiringWarningTime;
}

void AppConfig::setLastExpiringWarningTime(int newValue) {
  setSettingModified(m_LastExpiringWarningTime, newValue);
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

void AppConfig::setClientHostMode(bool newValue) {
  setSettingModified(m_ClientHostMode, newValue);
}

void AppConfig::setServerClientMode(bool newValue) {
  setSettingModified(m_ServerClientMode, newValue);
}

Config &AppConfig::config() { return m_Config; }

const QString &AppConfig::screenName() const { return m_ScreenName; }

int AppConfig::port() const { return m_Port; }

const QString &AppConfig::networkInterface() const { return m_Interface; }

int AppConfig::logLevel() const { return m_LogLevel; }

bool AppConfig::logToFile() const { return m_LogToFile; }

const QString &AppConfig::logFilename() const { return m_LogFilename; }

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

QString AppConfig::logLevelText() const { return logLevelNames[logLevel()]; }

ProcessMode AppConfig::processMode() const {
  return m_ServiceEnabled ? ProcessMode::kService : ProcessMode::kDesktop;
}

bool AppConfig::wizardShouldRun() const {
  return m_WizardLastRun < kWizardVersion;
}

bool AppConfig::startedBefore() const { return m_StartedBefore; }

QString AppConfig::lastVersion() const { return m_LastVersion; }

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

QString AppConfig::coreServerName() const { return m_CoreServerName; }

QString AppConfig::coreClientName() const { return m_CoreClientName; }

ElevateMode AppConfig::elevateMode() const { return m_ElevateMode; }

void AppConfig::setCryptoEnabled(bool newValue) {
  if (m_CryptoEnabled != newValue && newValue) {
    generateCertificate();
  } else {
    emit sslToggled();
  }
  setSettingModified(m_CryptoEnabled, newValue);
}

bool AppConfig::cryptoAvailable() const {
  bool result{true};

#ifdef SYNERGY_ENABLE_LICENSING
  result =
      (edition() == kPro || edition() == kProChina || edition() == kBusiness ||
       edition() == kUltimate);
#endif // SYNERGY_ENABLE_LICENSING

  return result;
}

bool AppConfig::cryptoEnabled() const {
  return cryptoAvailable() && m_CryptoEnabled;
}

void AppConfig::setAutoHide(bool b) { setSettingModified(m_AutoHide, b); }

bool AppConfig::autoHide() const { return m_AutoHide; }

void AppConfig::setMinimizeToTray(bool newValue) {
  setSettingModified(m_MinimizeToTray, newValue);
}

bool AppConfig::invertScrollDirection() const {
  return m_InvertScrollDirection;
}

void AppConfig::setLicenseNextCheck(unsigned long long time) {
  setSettingModified(m_licenseNextCheck, time);
}

unsigned long long AppConfig::licenseNextCheck() const {
  return m_licenseNextCheck;
}

const QString &AppConfig::guid() const { return m_Guid; }

bool AppConfig::languageSync() const { return m_LanguageSync; }

void AppConfig::setInvertScrollDirection(bool newValue) {
  setSettingModified(m_InvertScrollDirection, newValue);
}

void AppConfig::setLanguageSync(bool newValue) {
  setSettingModified(m_LanguageSync, newValue);
}

bool AppConfig::preventSleep() const { return m_PreventSleep; }

bool AppConfig::clientHostMode() const {
  return (m_ClientHostMode && initiateConnectionFromServer());
}

bool AppConfig::serverClientMode() const {
  return (m_ServerClientMode && initiateConnectionFromServer());
}

bool AppConfig::initiateConnectionFromServer() const {
  return m_InitiateConnectionFromServer;
}

void AppConfig::setPreventSleep(bool newValue) {
  setSettingModified(m_PreventSleep, newValue);
}

bool AppConfig::minimizeToTray() const { return m_MinimizeToTray; }

void AppConfig::setTlsCertPath(const QString &path) { m_TlsCertPath = path; }

QString AppConfig::tlsCertPath() const { return m_TlsCertPath; }

QString AppConfig::tlsKeyLength() const { return m_TlsKeyLength; }

void AppConfig::setTlsKeyLength(const QString &length) {
  if (m_TlsKeyLength != length) {
    m_TlsKeyLength = length;
    generateCertificate(true);
  }
}

void AppConfig::setServiceEnabled(bool enabled) {
  setSettingModified(m_ServiceEnabled, enabled);
}

bool AppConfig::serviceEnabled() const { return m_ServiceEnabled; }

void AppConfig::setCloseToTray(bool minimize) {
  setSettingModified(m_CloseToTray, minimize);
}

bool AppConfig::closeToTray() const { return m_CloseToTray; }

bool AppConfig::serverGroupChecked() const { return m_ServerGroupChecked; }

bool AppConfig::useExternalConfig() const { return m_UseExternalConfig; }

const QString &AppConfig::configFile() const { return m_ConfigFile; }

bool AppConfig::useInternalConfig() const { return m_UseInternalConfig; }

bool AppConfig::clientGroupChecked() const { return m_ClientGroupChecked; }

QString AppConfig::serverHostname() const { return m_ServerHostname; }

///////////////////////////////////////////////////////////////////////////////
// End getters and setters
///////////////////////////////////////////////////////////////////////////////
