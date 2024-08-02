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

#include "ConfigScopes.h"

#include <QApplication>
#include <QMessageBox>
#include <QPushButton>
#include <QVariant>
#include <QtCore>
#include <QtNetwork>
#include <functional>

using ConfigScopes = synergy::gui::ConfigScopes;
using IConfigScopes = synergy::gui::IConfigScopes;

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
    "enableService",
    "closeToTray",
    "mainWindowSize",
    "mainWindowPosition",
    "showDevThanks",
    "showCloseReminder",
};

static const char *logLevelNames[] = {"INFO", "DEBUG", "DEBUG1", "DEBUG2"};

AppConfig::AppConfig(
    synergy::gui::IConfigScopes &scopes, std::shared_ptr<Deps> deps)
    : m_scopes(scopes),
      m_pDeps(deps),
      m_ScreenName(deps->hostname()) {
  determineScope();
}

void AppConfig::recall() {
  using enum AppConfig::Setting;

  qDebug("recalling app config");

  recallFromAllScopes();
  recallFromCurrentScope();
  recallSerialKey();
  recallElevateMode();

  emit ready();
}

void AppConfig::recallFromAllScopes() {
  using enum Setting;

  m_WizardLastRun = findInAllScopes(kWizardLastRun, m_WizardLastRun).toInt();
  m_LoadFromSystemScope =
      findInAllScopes(kLoadSystemSettings, m_LoadFromSystemScope).toBool();
  m_licenseNextCheck =
      findInAllScopes(kLicenseNextCheck, m_licenseNextCheck).toULongLong();
}

void AppConfig::recallFromCurrentScope() {
  using enum Setting;

  m_ScreenName = getFromScope(kScreenName, m_ScreenName).toString();
  m_Port = getFromScope(kPort, m_Port).toInt();
  m_Interface = getFromScope(kInterface, m_Interface).toString();
  m_LogLevel = getFromScope(kLogLevel, m_LogLevel).toInt();
  m_LogToFile = getFromScope(kLogToFile, m_LogToFile).toBool();
  m_LogFilename = getFromScope(kLogFilename, m_LogFilename).toString();
  m_StartedBefore = getFromScope(kStartedBefore, m_StartedBefore).toBool();
  m_AutoHide = getFromScope(kAutoHide, m_AutoHide).toBool();
  m_LastVersion = getFromScope(kLastVersion, m_LastVersion).toString();
  m_ActivationHasRun =
      getFromScope(kActivationHasRun, m_ActivationHasRun).toBool();
  m_ServerGroupChecked =
      getFromScope(kServerGroupChecked, m_ServerGroupChecked).toBool();
  m_UseExternalConfig =
      getFromScope(kUseExternalConfig, m_UseExternalConfig).toBool();
  m_ConfigFile = getFromScope(kConfigFile, m_ConfigFile).toString();
  m_UseInternalConfig =
      getFromScope(kUseInternalConfig, m_UseInternalConfig).toBool();
  m_ClientGroupChecked =
      getFromScope(kClientGroupChecked, m_ClientGroupChecked).toBool();
  m_ServerHostname = getFromScope(kServerHostname, m_ServerHostname).toString();
  m_PreventSleep = getFromScope(kPreventSleep, m_PreventSleep).toBool();
  m_LanguageSync = getFromScope(kLanguageSync, m_LanguageSync).toBool();
  m_InvertScrollDirection =
      getFromScope(kInvertScrollDirection, m_InvertScrollDirection).toBool();
  m_InvertConnection =
      getFromScope(kInvertConnection, m_InvertConnection).toBool();
  m_EnableService = getFromScope(kEnableService, m_EnableService).toBool();
  m_CloseToTray = getFromScope(kCloseToTray, m_CloseToTray).toBool();
  m_TlsEnabled = getFromScope(kTlsEnabled, m_TlsEnabled).toBool();
  m_TlsCertPath = getFromScope(kTlsCertPath, m_TlsCertPath).toString();
  m_TlsKeyLength = getFromScope(kTlsKeyLength, m_TlsKeyLength).toString();
  m_MainWindowPosition = getFromScopeOptional<QPoint>(
      kMainWindowPosition, [](QVariant v) { return v.toPoint(); });
  m_MainWindowSize = getFromScopeOptional<QSize>(
      kMainWindowSize, [](QVariant v) { return v.toSize(); });
  m_ShowDevThanks = getFromScope(kShowDevThanks, m_ShowDevThanks).toBool();
  m_ShowCloseReminder =
      getFromScope(kShowCloseReminder, m_ShowCloseReminder).toBool();
}

void AppConfig::commit() {
  using enum Setting;

  qDebug("comitting app config");

  saveToAllScopes(kWizardLastRun, m_WizardLastRun);
  saveToAllScopes(kLoadSystemSettings, m_LoadFromSystemScope);
  saveToAllScopes(kClientGroupChecked, m_ClientGroupChecked);
  saveToAllScopes(kServerGroupChecked, m_ServerGroupChecked);
  saveToAllScopes(kLicenseNextCheck, m_licenseNextCheck);

  if (isCurrentScopeWritable()) {
    saveToCurrentScope(kScreenName, m_ScreenName);
    saveToCurrentScope(kPort, m_Port);
    saveToCurrentScope(kInterface, m_Interface);
    saveToCurrentScope(kLogLevel, m_LogLevel);
    saveToCurrentScope(kLogToFile, m_LogToFile);
    saveToCurrentScope(kLogFilename, m_LogFilename);
    saveToCurrentScope(kStartedBefore, m_StartedBefore);
    saveToCurrentScope(kElevateMode, static_cast<int>(m_ElevateMode));
    saveToCurrentScope(kElevateModeLegacy, m_ElevateMode == ElevateAlways);
    saveToCurrentScope(kTlsEnabled, m_TlsEnabled);
    saveToCurrentScope(kAutoHide, m_AutoHide);
    saveToCurrentScope(kSerialKey, m_SerialKey);
    saveToCurrentScope(kLastVersion, m_LastVersion);
    saveToCurrentScope(kActivationHasRun, m_ActivationHasRun);
    saveToCurrentScope(kUseExternalConfig, m_UseExternalConfig);
    saveToCurrentScope(kConfigFile, m_ConfigFile);
    saveToCurrentScope(kUseInternalConfig, m_UseInternalConfig);
    saveToCurrentScope(kServerHostname, m_ServerHostname);
    saveToCurrentScope(kPreventSleep, m_PreventSleep);
    saveToCurrentScope(kLanguageSync, m_LanguageSync);
    saveToCurrentScope(kInvertScrollDirection, m_InvertScrollDirection);
    saveToCurrentScope(kInvertConnection, m_InvertConnection);
    saveToCurrentScope(kEnableService, m_EnableService);
    saveToCurrentScope(kCloseToTray, m_CloseToTray);
    saveToCurrentScope(kShowDevThanks, m_ShowDevThanks);
    saveToCurrentScope(kShowCloseReminder, m_ShowCloseReminder);

    saveToCurrentScopeOptional(kMainWindowSize, m_MainWindowSize);
    saveToCurrentScopeOptional(kMainWindowPosition, m_MainWindowPosition);
  }

  if (m_TlsChanged) {
    m_TlsChanged = false;
    emit tlsChanged();
  }
}

void AppConfig::determineScope() {

  // first, try to determine if the system scope should be used according to the
  // user scope...
  if (m_scopes.scopeContains(
          settingName(Setting::kLoadSystemSettings),
          ConfigScopes::Scope::User)) {
    setLoadFromSystemScope(m_LoadFromSystemScope);
  }

  // ...failing that, check the system scope instead to see if an arbitrary
  // required setting is present. if it is, then we can assume that the system
  // scope should be used.
  else if (m_scopes.scopeContains(
               settingName(Setting::kScreenName),
               ConfigScopes::Scope::System)) {
    setLoadFromSystemScope(true);
  }
}

void AppConfig::recallSerialKey() {
  using enum Setting;

  // only set the serial key if the current settings scope has they key.
  bool shouldLoad = m_scopes.scopeContains(
      settingName(kLoadSystemSettings), ConfigScopes::Scope::Current);

  if (!shouldLoad) {
    qDebug("no serial key in current scope, skipping");
    return;
  }

  const auto &serialKey =
      getFromScope(kSerialKey, m_SerialKey).toString().trimmed();

  if (serialKey.isEmpty()) {
    qDebug("serial key is empty, skipping");
    return;
  }

  m_SerialKey = serialKey;
}

void AppConfig::recallElevateMode() {
  using enum Setting;

  if (!m_scopes.scopeContains(settingName(kElevateMode))) {
    qDebug("elevate mode not set yet, skipping");
    return;
  }

  QVariant elevateMode = getFromScope(kElevateMode);
  if (!elevateMode.isValid()) {
    qDebug("elevate mode not valid, loading legacy setting");
    elevateMode = getFromScope(
        kElevateModeLegacy, QVariant(static_cast<int>(kDefaultElevateMode)));
  }

  m_ElevateMode = static_cast<ElevateMode>(elevateMode.toInt());
}

QString AppConfig::defaultTlsCertPath() const {
  QDir path(m_pDeps->profileDir());
  path = path.filePath("SSL");
  path = path.filePath("Synergy.pem");
  return path.absolutePath();
}

QString AppConfig::settingName(Setting name) {
  auto index = static_cast<int>(name);
  return m_SettingsName[index];
}

template <typename T>
void AppConfig::saveToCurrentScope(Setting name, T value) {
  m_scopes.setToScope(settingName(name), value);
}

template <typename T> void AppConfig::saveToAllScopes(Setting name, T value) {
  m_scopes.setToScope(settingName(name), value, ConfigScopes::Scope::User);
  m_scopes.setToScope(settingName(name), value, ConfigScopes::Scope::System);
}

QVariant AppConfig::getFromScope(Setting name, const QVariant &defaultValue) {
  return m_scopes.getFromScope(settingName(name), defaultValue);
}

template <typename T>
std::optional<T> AppConfig::getFromScopeOptional(
    Setting name, std::function<T(QVariant)> toType) const {
  if (m_scopes.scopeContains(settingName(name))) {
    return toType(m_scopes.getFromScope(settingName(name)));
  } else {
    return std::nullopt;
  }
}

template <typename T>
void AppConfig::saveToCurrentScopeOptional(
    Setting name, const std::optional<T> &value) {
  if (value.has_value()) {
    m_scopes.setToScope(settingName(name), value.value());
  }
}

QVariant
AppConfig::findInAllScopes(Setting name, const QVariant &defaultValue) const {
  QVariant result(defaultValue);
  QString setting(settingName(name));

  if (m_scopes.scopeContains(setting)) {
    result = m_scopes.getFromScope(setting, defaultValue);
  } else if (m_scopes.activeScope() == ConfigScopes::Scope::System) {
    if (m_scopes.scopeContains(setting, ConfigScopes::Scope::User)) {
      result = m_scopes.getFromScope(
          setting, defaultValue, ConfigScopes::Scope::User);
    }
  } else if (m_scopes.scopeContains(setting, ConfigScopes::Scope::System)) {
    result = m_scopes.getFromScope(
        setting, defaultValue, ConfigScopes::Scope::System);
  }

  return result;
}

void AppConfig::loadScope(ConfigScopes::Scope scope) {

  if (m_scopes.activeScope() != scope) {
    setDefaultValues();
    m_scopes.setActiveScope(scope);
    if (m_scopes.scopeContains(
            settingName(Setting::kScreenName), m_scopes.activeScope())) {
      // If the user already has settings, then load them up now.
      m_scopes.signalReady();
    }
  }
}

void AppConfig::setDefaultValues() { m_InvertConnection = false; }

void AppConfig::setLoadFromSystemScope(bool value) {

  if (value) {
    qDebug("loading system settings scope");
    loadScope(ConfigScopes::Scope::System);
  } else {
    qDebug("loading user settings scope");
    loadScope(ConfigScopes::Scope::User);
  }

  /*
   * It's very imprortant to set this variable after loadScope
   * because during scope loading this variable can be rewritten with old value
   */
  m_LoadFromSystemScope = value;
}

bool AppConfig::isCurrentScopeWritable() const {
  return m_scopes.isActiveScopeWritable();
}

bool AppConfig::isCurrentScopeSystem() const {
  return m_scopes.activeScope() == ConfigScopes::Scope::System;
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

IConfigScopes &AppConfig::scopes() { return m_scopes; }

bool AppConfig::activationHasRun() const { return m_ActivationHasRun; }

QString AppConfig::serialKey() const { return m_SerialKey; }

const QString &AppConfig::screenName() const { return m_ScreenName; }

int AppConfig::port() const { return m_Port; }

const QString &AppConfig::networkInterface() const { return m_Interface; }

int AppConfig::logLevel() const { return m_LogLevel; }

bool AppConfig::logToFile() const { return m_LogToFile; }

const QString &AppConfig::logFilename() const { return m_LogFilename; }

QString AppConfig::logLevelText() const { return logLevelNames[logLevel()]; }

ProcessMode AppConfig::processMode() const {
  return m_EnableService ? ProcessMode::kService : ProcessMode::kDesktop;
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

bool AppConfig::enableService() const { return m_EnableService; }

bool AppConfig::closeToTray() const { return m_CloseToTray; }

bool AppConfig::serverGroupChecked() const { return m_ServerGroupChecked; }

bool AppConfig::useExternalConfig() const { return m_UseExternalConfig; }

const QString &AppConfig::configFile() const { return m_ConfigFile; }

bool AppConfig::useInternalConfig() const { return m_UseInternalConfig; }

bool AppConfig::clientGroupChecked() const { return m_ClientGroupChecked; }

QString AppConfig::serverHostname() const { return m_ServerHostname; }

void AppConfig::setActivationHasRun(bool value) { m_ActivationHasRun = value; }

std::optional<QSize> AppConfig::mainWindowSize() const {
  return m_MainWindowSize;
}

std::optional<QPoint> AppConfig::mainWindowPosition() const {
  return m_MainWindowPosition;
}

bool AppConfig::showDevThanks() const { return m_ShowDevThanks; }

bool AppConfig::showCloseReminder() const { return m_ShowCloseReminder; }

///////////////////////////////////////////////////////////////////////////////
// End getters
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Begin setters
///////////////////////////////////////////////////////////////////////////////

void AppConfig::clearSerialKey() { m_SerialKey.clear(); }

void AppConfig::setTlsEnabled(bool value) {
  // we purposefully do not set the 'tls changed' flag when enabling/disabling
  // tls, since that would cause the certificate to regenerate, which could get
  // pretty annoying.

  m_TlsEnabled = value;
}

void AppConfig::setTlsCertPath(const QString &value) {
  m_TlsCertPath = value;
  m_TlsChanged = true;
}

void AppConfig::setTlsKeyLength(const QString &value) {
  m_TlsKeyLength = value;
  m_TlsChanged = true;
}
void AppConfig::setSerialKey(const QString &serialKey) {
  m_SerialKey = serialKey;
  saveToAllScopes(Setting::kSerialKey, m_SerialKey);
}
void AppConfig::setServerGroupChecked(bool newValue) {
  m_ServerGroupChecked = newValue;
}

void AppConfig::setUseExternalConfig(bool newValue) {
  m_UseExternalConfig = newValue;
}

void AppConfig::setConfigFile(const QString &newValue) {
  m_ConfigFile = newValue;
}

void AppConfig::setUseInternalConfig(bool newValue) {
  m_UseInternalConfig = newValue;
}

void AppConfig::setClientGroupChecked(bool newValue) {
  m_ClientGroupChecked = newValue;
}

void AppConfig::setServerHostname(const QString &newValue) {
  m_ServerHostname = newValue;
}
void AppConfig::setLastVersion(const QString &version) {
  m_LastVersion = version;
}

void AppConfig::setScreenName(const QString &s) {
  m_ScreenName = s;
  emit screenNameChanged();
}

void AppConfig::setPort(int i) { m_Port = i; }

void AppConfig::setNetworkInterface(const QString &s) { m_Interface = s; }

void AppConfig::setLogLevel(int i) { m_LogLevel = i; }

void AppConfig::setLogToFile(bool b) { m_LogToFile = b; }

void AppConfig::setLogFilename(const QString &s) { m_LogFilename = s; }

void AppConfig::setWizardHasRun() { m_WizardLastRun = kWizardVersion; }

void AppConfig::setStartedBefore(bool b) { m_StartedBefore = b; }

void AppConfig::setElevateMode(ElevateMode em) { m_ElevateMode = em; }

void AppConfig::setAutoHide(bool b) { m_AutoHide = b; }

void AppConfig::setLicenseNextCheck(unsigned long long time) {
  m_licenseNextCheck = time;
}

void AppConfig::setInvertScrollDirection(bool newValue) {
  m_InvertScrollDirection = newValue;
}

void AppConfig::setLanguageSync(bool newValue) { m_LanguageSync = newValue; }

void AppConfig::setPreventSleep(bool newValue) { m_PreventSleep = newValue; }

void AppConfig::setEnableService(bool enabled) { m_EnableService = enabled; }

void AppConfig::setCloseToTray(bool minimize) { m_CloseToTray = minimize; }

void AppConfig::setInvertConnection(bool value) {
  m_InvertConnection = value;
  emit invertConnectionChanged();
}

void AppConfig::setMainWindowSize(const QSize &size) {
  m_MainWindowSize = size;
}

void AppConfig::setMainWindowPosition(const QPoint &position) {
  m_MainWindowPosition = position;
}

void AppConfig::setShowDevThanks(bool value) { m_ShowDevThanks = value; }

void AppConfig::setShowCloseReminder(bool value) {
  m_ShowCloseReminder = value;
}

///////////////////////////////////////////////////////////////////////////////
// End setters
///////////////////////////////////////////////////////////////////////////////
