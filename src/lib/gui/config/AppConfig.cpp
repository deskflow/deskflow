/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2008 Volker Lanz <vl@fidra.de>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "AppConfig.h"

#include "ConfigScopes.h"

#include <QApplication>
#include <QMessageBox>
#include <QPushButton>
#include <QVariant>

#include <functional>

using namespace deskflow::gui;

// this should be incremented each time the wizard is changed,
// which will force it to re-run for existing installations.
const int kWizardVersion = 8;

static const char *const kLogLevelNames[] = {"INFO", "DEBUG", "DEBUG1", "DEBUG2"};

#if defined(Q_OS_WIN)
const char AppConfig::m_LogDir[] = "log/";
#else
const char AppConfig::m_LogDir[] = "/var/log/";
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
    "cryptoEnabled", // 11 = kTlsEnabled (retain legacy string value)
    "autoHide",
    "", // 13 = serialKey, obsolete
    "lastVersion",
    "", // 15 = lastExpiringWarningTime, obsolete
    "", // 16 = activationHasRun, obsolete
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
    "", // 33 = licenseNextCheck, obsolete
    "", // 34 = kInvertConnection, obsolete
    "", // 35 = clientHostMode, obsolete
    "", // 36 = serverClientMode, obsolete
    "enableService",
    "closeToTray",
    "mainWindowSize",
    "mainWindowPosition",
    "", // 41 = Show dev thanks, obsolete
    "showCloseReminder",
    "enableUpdateCheck",
    "logExpanded",
    "colorfulIcon",
    "requireClientCerts",
};

AppConfig::AppConfig(deskflow::gui::IConfigScopes &scopes, std::shared_ptr<Deps> deps)
    : m_Scopes(scopes),
      m_pDeps(deps),
      m_ScreenName(deps->hostname()),
      m_TlsCertPath(deps->defaultTlsCertPath())
{
  determineScope();
  recall();
}

void AppConfig::recall()
{
  using enum AppConfig::Setting;

  qDebug("recalling app config");

  recallFromAllScopes();
  recallFromCurrentScope();
}

void AppConfig::recallFromAllScopes()
{
  using enum Setting;

  m_WizardLastRun = findInAllScopes(kWizardLastRun, m_WizardLastRun).toInt();
  m_LoadFromSystemScope = findInAllScopes(kLoadSystemSettings, m_LoadFromSystemScope).toBool();
}

void AppConfig::recallFromCurrentScope()
{
  using enum Setting;

  recallScreenName();
  recallElevateMode();

  m_Port = getFromCurrentScope(kPort, m_Port).toInt();
  m_Interface = getFromCurrentScope(kInterface, m_Interface).toString();
  m_LogLevel = getFromCurrentScope(kLogLevel, m_LogLevel).toInt();
  m_LogToFile = getFromCurrentScope(kLogToFile, m_LogToFile).toBool();
  m_LogFilename = getFromCurrentScope(kLogFilename, m_LogFilename).toString();
  m_StartedBefore = getFromCurrentScope(kStartedBefore, m_StartedBefore).toBool();
  m_AutoHide = getFromCurrentScope(kAutoHide, m_AutoHide).toBool();
  m_LastVersion = getFromCurrentScope(kLastVersion, m_LastVersion).toString();
  m_ServerGroupChecked = getFromCurrentScope(kServerGroupChecked, m_ServerGroupChecked).toBool();
  m_UseExternalConfig = getFromCurrentScope(kUseExternalConfig, m_UseExternalConfig).toBool();
  m_ConfigFile = getFromCurrentScope(kConfigFile, m_ConfigFile).toString();
  m_UseInternalConfig = getFromCurrentScope(kUseInternalConfig, m_UseInternalConfig).toBool();
  m_ClientGroupChecked = getFromCurrentScope(kClientGroupChecked, m_ClientGroupChecked).toBool();
  m_ServerHostname = getFromCurrentScope(kServerHostname, m_ServerHostname).toString();
  m_PreventSleep = getFromCurrentScope(kPreventSleep, m_PreventSleep).toBool();
  m_LanguageSync = getFromCurrentScope(kLanguageSync, m_LanguageSync).toBool();
  m_InvertScrollDirection = getFromCurrentScope(kInvertScrollDirection, m_InvertScrollDirection).toBool();
  m_EnableService = getFromCurrentScope(kEnableService, m_EnableService).toBool();
  m_CloseToTray = getFromCurrentScope(kCloseToTray, m_CloseToTray).toBool();
  m_TlsEnabled = getFromCurrentScope(kTlsEnabled, m_TlsEnabled).toBool();
  m_TlsCertPath = getFromCurrentScope(kTlsCertPath, m_TlsCertPath).toString();
  m_TlsKeyLength = getFromCurrentScope(kTlsKeyLength, m_TlsKeyLength).toInt();
  m_RequireClientCert = getFromCurrentScope(kRequireClientCert, m_RequireClientCert).toBool();
  m_MainWindowPosition =
      getFromCurrentScope<QPoint>(kMainWindowPosition, [](const QVariant &v) { return v.toPoint(); });
  m_MainWindowSize = getFromCurrentScope<QSize>(kMainWindowSize, [](const QVariant &v) { return v.toSize(); });
  m_ShowCloseReminder = getFromCurrentScope(kShowCloseReminder, m_ShowCloseReminder).toBool();
  m_EnableUpdateCheck = getFromCurrentScope<bool>(kEnableUpdateCheck, [](const QVariant &v) { return v.toBool(); });
  m_logExpanded = getFromCurrentScope(kLogExpanded, m_logExpanded).toBool();
  m_colorfulTrayIcon = getFromCurrentScope(kColorfulIcon, m_colorfulTrayIcon).toBool();
}

void AppConfig::recallScreenName()
{
  using enum Setting;

  const auto &screenName = getFromCurrentScope(kScreenName, m_ScreenName).toString().trimmed();

  // for some reason, the screen name can be saved as an empty string
  // in the config file. this is probably a bug. if this happens, then default
  // back to the hostname.
  if (screenName.isEmpty()) {
    qWarning("screen name was empty in config, setting to hostname");
    m_ScreenName = m_pDeps->hostname();
  } else {
    m_ScreenName = screenName;
  }
}

void AppConfig::commit()
{
  using enum Setting;

  qDebug("committing app config");

  saveToAllScopes(kWizardLastRun, m_WizardLastRun);
  saveToAllScopes(kLoadSystemSettings, m_LoadFromSystemScope);
  saveToAllScopes(kClientGroupChecked, m_ClientGroupChecked);
  saveToAllScopes(kServerGroupChecked, m_ServerGroupChecked);

  if (isActiveScopeWritable()) {
    setInCurrentScope(kScreenName, m_ScreenName);
    setInCurrentScope(kPort, m_Port);
    setInCurrentScope(kInterface, m_Interface);
    setInCurrentScope(kLogLevel, m_LogLevel);
    setInCurrentScope(kLogToFile, m_LogToFile);
    setInCurrentScope(kLogFilename, m_LogFilename);
    setInCurrentScope(kStartedBefore, m_StartedBefore);
    setInCurrentScope(kElevateMode, static_cast<int>(m_ElevateMode));
    setInCurrentScope(kElevateModeLegacy, m_ElevateMode == ElevateMode::kAlways);
    setInCurrentScope(kTlsEnabled, m_TlsEnabled);
    setInCurrentScope(kAutoHide, m_AutoHide);
    setInCurrentScope(kLastVersion, m_LastVersion);
    setInCurrentScope(kUseExternalConfig, m_UseExternalConfig);
    setInCurrentScope(kConfigFile, m_ConfigFile);
    setInCurrentScope(kUseInternalConfig, m_UseInternalConfig);
    setInCurrentScope(kServerHostname, m_ServerHostname);
    setInCurrentScope(kPreventSleep, m_PreventSleep);
    setInCurrentScope(kLanguageSync, m_LanguageSync);
    setInCurrentScope(kInvertScrollDirection, m_InvertScrollDirection);
    setInCurrentScope(kEnableService, m_EnableService);
    setInCurrentScope(kCloseToTray, m_CloseToTray);
    setInCurrentScope(kMainWindowSize, m_MainWindowSize);
    setInCurrentScope(kMainWindowPosition, m_MainWindowPosition);
    setInCurrentScope(kShowCloseReminder, m_ShowCloseReminder);
    setInCurrentScope(kEnableUpdateCheck, m_EnableUpdateCheck);
    setInCurrentScope(kLogExpanded, m_logExpanded);
    setInCurrentScope(kColorfulIcon, m_colorfulTrayIcon);
    setInCurrentScope(kRequireClientCert, m_RequireClientCert);
  }

  if (m_TlsChanged) {
    m_TlsChanged = false;
    Q_EMIT tlsChanged();
  }
}

void AppConfig::determineScope()
{

  qDebug("determining config scope");

  // first, try to determine if the system scope should be used according to the
  // user scope...
  if (m_Scopes.scopeContains(settingName(Setting::kLoadSystemSettings), ConfigScopes::Scope::User)) {
    auto loadFromSystemScope =
        m_Scopes
            .getFromScope(settingName(Setting::kLoadSystemSettings), m_LoadFromSystemScope, ConfigScopes::Scope::User)
            .toBool();
    if (loadFromSystemScope) {
      qDebug("user settings indicates system scope should be used");
    } else {
      qDebug("user settings indicates user scope should be used");
    }
    setLoadFromSystemScope(loadFromSystemScope);
  }

  // ...failing that, check the system scope instead to see if an arbitrary
  // required setting is present. if it is, then we can assume that the system
  // scope should be used.
  else if (m_Scopes.scopeContains(settingName(Setting::kScreenName), ConfigScopes::Scope::System)) {
    qDebug("system settings scope contains screen name, using system scope");
    setLoadFromSystemScope(true);
  }
}

void AppConfig::recallElevateMode()
{
  using enum Setting;

  if (!m_Scopes.scopeContains(settingName(kElevateMode))) {
    qDebug("elevate mode not set yet, skipping");
    return;
  }

  QVariant elevateMode = getFromCurrentScope(kElevateMode);
  if (!elevateMode.isValid()) {
    qDebug("elevate mode not valid, loading legacy setting");
    elevateMode = getFromCurrentScope(kElevateModeLegacy, QVariant(static_cast<int>(kDefaultElevateMode)));
  }

  m_ElevateMode = static_cast<ElevateMode>(elevateMode.toInt());
}

QString AppConfig::settingName(Setting name)
{
  auto index = static_cast<int>(name);
  return m_SettingsName[index];
}

template <typename T> void AppConfig::setInCurrentScope(Setting name, T value)
{
  m_Scopes.setInScope(settingName(name), value);
}

template <typename T> void AppConfig::saveToAllScopes(Setting name, T value)
{
  m_Scopes.setInScope(settingName(name), value, ConfigScopes::Scope::User);
  m_Scopes.setInScope(settingName(name), value, ConfigScopes::Scope::System);
}

QVariant AppConfig::getFromCurrentScope(Setting name, const QVariant &defaultValue) const
{
  return m_Scopes.getFromScope(settingName(name), defaultValue);
}

template <typename T>
std::optional<T> AppConfig::getFromCurrentScope(Setting name, std::function<T(const QVariant &)> toType) const
{
  if (m_Scopes.scopeContains(settingName(name))) {
    return toType(m_Scopes.getFromScope(settingName(name)));
  } else {
    return std::nullopt;
  }
}

template <typename T> void AppConfig::setInCurrentScope(Setting name, const std::optional<T> &value)
{
  if (value.has_value()) {
    m_Scopes.setInScope(settingName(name), value.value());
  }
}

QVariant AppConfig::findInAllScopes(Setting name, const QVariant &defaultValue) const
{
  using enum ConfigScopes::Scope;

  QVariant result(defaultValue);
  QString setting(settingName(name));

  if (m_Scopes.scopeContains(setting)) {
    result = m_Scopes.getFromScope(setting, defaultValue);
  } else if (m_Scopes.activeScope() == System) {
    if (m_Scopes.scopeContains(setting, User)) {
      result = m_Scopes.getFromScope(setting, defaultValue, User);
    }
  } else if (m_Scopes.scopeContains(setting, System)) {
    result = m_Scopes.getFromScope(setting, defaultValue, System);
  }

  return result;
}

void AppConfig::loadScope(ConfigScopes::Scope scope)
{
  using enum ConfigScopes::Scope;

  switch (scope) {
  case User:
    qDebug("loading user settings scope");
    break;

  case System:
    qDebug("loading system settings scope");
    break;

  default:
    qFatal("invalid scope");
  }

  if (m_Scopes.activeScope() == scope) {
    qDebug("already in required scope, skipping");
    return;
  }

  m_Scopes.setActiveScope(scope);

  qDebug("active scope file path: %s", qPrintable(m_Scopes.activeFilePath()));

  // only signal ready if there is at least one setting in the required scope.
  // this prevents the current settings from being set back to default.
  if (m_Scopes.scopeContains(settingName(Setting::kScreenName), m_Scopes.activeScope())) {
    m_Scopes.signalReady();
  } else {
    qDebug("no screen name in scope, skipping");
  }
}

void AppConfig::setLoadFromSystemScope(bool value)
{
  using enum ConfigScopes::Scope;

  if (value) {
    loadScope(System);
  } else {
    loadScope(User);
  }

  // set after loading scope since it may have been overridden.
  m_LoadFromSystemScope = value;
}

bool AppConfig::isActiveScopeWritable() const
{
  return m_Scopes.isActiveScopeWritable();
}

bool AppConfig::isActiveScopeSystem() const
{
  return m_Scopes.activeScope() == ConfigScopes::Scope::System;
}

QString AppConfig::logDir() const
{
  // by default log to home dir
  return QDir::home().absolutePath() + "/";
}

void AppConfig::persistLogDir() const
{
  QDir dir = logDir();

  // persist the log directory
  if (!dir.exists()) {
    dir.mkpath(dir.path());
  }
}

///////////////////////////////////////////////////////////////////////////////
// Begin getters
///////////////////////////////////////////////////////////////////////////////

IConfigScopes &AppConfig::scopes() const
{
  return m_Scopes;
}

const QString &AppConfig::screenName() const
{
  return m_ScreenName;
}

int AppConfig::port() const
{
  return m_Port;
}

const QString &AppConfig::networkInterface() const
{
  return m_Interface;
}

int AppConfig::logLevel() const
{
  return m_LogLevel;
}

bool AppConfig::logToFile() const
{
  return m_LogToFile;
}

const QString &AppConfig::logFilename() const
{
  return m_LogFilename;
}

QString AppConfig::logLevelText() const
{
  return kLogLevelNames[logLevel()];
}

ProcessMode AppConfig::processMode() const
{
  return m_EnableService ? ProcessMode::kService : ProcessMode::kDesktop;
}

bool AppConfig::wizardShouldRun() const
{
  return m_WizardLastRun < kWizardVersion;
}

bool AppConfig::startedBefore() const
{
  return m_StartedBefore;
}

QString AppConfig::lastVersion() const
{
  return m_LastVersion;
}

QString AppConfig::coreServerName() const
{
#ifdef Q_OS_WIN
  return s_winExeTemplate.arg(s_CoreServerName);
#else
  return s_CoreServerName;
#endif
}

QString AppConfig::coreClientName() const
{
#ifdef Q_OS_WIN
  return s_winExeTemplate.arg(s_CoreClientName);
#else
  return s_CoreClientName;
#endif
}

ElevateMode AppConfig::elevateMode() const
{
  return m_ElevateMode;
}

bool AppConfig::tlsEnabled() const
{
  return m_TlsEnabled;
}

bool AppConfig::autoHide() const
{
  return m_AutoHide;
}

bool AppConfig::invertScrollDirection() const
{
  return m_InvertScrollDirection;
}

bool AppConfig::languageSync() const
{
  return m_LanguageSync;
}

bool AppConfig::preventSleep() const
{
  return m_PreventSleep;
}

QString AppConfig::tlsCertPath() const
{
  return m_TlsCertPath;
}

int AppConfig::tlsKeyLength() const
{
  return m_TlsKeyLength;
}

bool AppConfig::enableService() const
{
  return m_EnableService;
}

bool AppConfig::closeToTray() const
{
  return m_CloseToTray;
}

bool AppConfig::serverGroupChecked() const
{
  return m_ServerGroupChecked;
}

bool AppConfig::useExternalConfig() const
{
  return m_UseExternalConfig;
}

const QString &AppConfig::configFile() const
{
  return m_ConfigFile;
}

bool AppConfig::useInternalConfig() const
{
  return m_UseInternalConfig;
}

bool AppConfig::clientGroupChecked() const
{
  return m_ClientGroupChecked;
}

bool AppConfig::requireClientCerts() const
{
  return m_RequireClientCert;
}

const QString &AppConfig::serverHostname() const
{
  return m_ServerHostname;
}

std::optional<QSize> AppConfig::mainWindowSize() const
{
  return m_MainWindowSize;
}

std::optional<QPoint> AppConfig::mainWindowPosition() const
{
  return m_MainWindowPosition;
}

bool AppConfig::showCloseReminder() const
{
  return m_ShowCloseReminder;
}

std::optional<bool> AppConfig::enableUpdateCheck() const
{
  return m_EnableUpdateCheck;
}

bool AppConfig::logExpanded() const
{
  return m_logExpanded;
}

bool AppConfig::colorfulTrayIcon() const
{
  return m_colorfulTrayIcon;
}

///////////////////////////////////////////////////////////////////////////////
// End getters
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Begin setters
///////////////////////////////////////////////////////////////////////////////

void AppConfig::setTlsEnabled(bool value)
{
  // we purposefully do not set the 'tls changed' flag when enabling/disabling
  // tls, since that would cause the certificate to regenerate, which could get
  // pretty annoying.

  m_TlsEnabled = value;
}

void AppConfig::setTlsCertPath(const QString &value)
{
  if (m_TlsCertPath != value) {
    // deliberately only set the changed flag if there was a change.
    // it's important not to set this flag to false here.
    m_TlsChanged = true;
  }
  m_TlsCertPath = value;
}

void AppConfig::setTlsKeyLength(int value)
{
  if (m_TlsKeyLength != value) {
    // deliberately only set the changed flag if there was a change.
    // it's important not to set this flag to false here.
    m_TlsChanged = true;
  }
  m_TlsKeyLength = value;
}

void AppConfig::setServerGroupChecked(bool newValue)
{
  m_ServerGroupChecked = newValue;
}

void AppConfig::setUseExternalConfig(bool newValue)
{
  m_UseExternalConfig = newValue;
}

void AppConfig::setConfigFile(const QString &newValue)
{
  m_ConfigFile = newValue;
}

void AppConfig::setUseInternalConfig(bool newValue)
{
  m_UseInternalConfig = newValue;
}

void AppConfig::setClientGroupChecked(bool newValue)
{
  m_ClientGroupChecked = newValue;
}

void AppConfig::setServerHostname(const QString &newValue)
{
  m_ServerHostname = newValue;
}
void AppConfig::setLastVersion(const QString &version)
{
  m_LastVersion = version;
}

void AppConfig::setScreenName(const QString &s)
{
  m_ScreenName = s;
  Q_EMIT screenNameChanged();
}

void AppConfig::setPort(int i)
{
  m_Port = i;
}

void AppConfig::setNetworkInterface(const QString &s)
{
  m_Interface = s;
}

void AppConfig::setLogLevel(int i)
{
  const auto changed = (m_LogLevel != i);
  m_LogLevel = i;
  if (changed)
    Q_EMIT logLevelChanged();
}

void AppConfig::setLogToFile(bool b)
{
  m_LogToFile = b;
}

void AppConfig::setLogFilename(const QString &s)
{
  m_LogFilename = s;
}

void AppConfig::setWizardHasRun()
{
  m_WizardLastRun = kWizardVersion;
}

void AppConfig::setStartedBefore(bool b)
{
  m_StartedBefore = b;
}

void AppConfig::setElevateMode(ElevateMode em)
{
  m_ElevateMode = em;
}

void AppConfig::setAutoHide(bool b)
{
  m_AutoHide = b;
}

void AppConfig::setInvertScrollDirection(bool newValue)
{
  m_InvertScrollDirection = newValue;
}

void AppConfig::setLanguageSync(bool newValue)
{
  m_LanguageSync = newValue;
}

void AppConfig::setPreventSleep(bool newValue)
{
  m_PreventSleep = newValue;
}

void AppConfig::setEnableService(bool enabled)
{
  m_EnableService = enabled;
}

void AppConfig::setCloseToTray(bool minimize)
{
  m_CloseToTray = minimize;
}

void AppConfig::setRequireClientCerts(bool requireClientCerts)
{
  if (requireClientCerts == m_RequireClientCert)
    return;
  m_RequireClientCert = requireClientCerts;
}

void AppConfig::setMainWindowSize(const QSize &size)
{
  m_MainWindowSize = size;
}

void AppConfig::setMainWindowPosition(const QPoint &position)
{
  m_MainWindowPosition = position;
}

void AppConfig::setShowCloseReminder(bool value)
{
  m_ShowCloseReminder = value;
}

void AppConfig::setEnableUpdateCheck(bool value)
{
  m_EnableUpdateCheck = value;
}

void AppConfig::setLogExpanded(bool expanded)
{
  if (expanded == m_logExpanded)
    return;
  m_logExpanded = expanded;
}

void AppConfig::setColorfulTrayIcon(bool colorful)
{
  if (colorful == m_colorfulTrayIcon)
    return;
  m_colorfulTrayIcon = colorful;
}

///////////////////////////////////////////////////////////////////////////////
// End setters
///////////////////////////////////////////////////////////////////////////////
