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

#if defined(Q_OS_WIN)
const char AppConfig::m_LogDir[] = "log/";
#else
const char AppConfig::m_LogDir[] = "/var/log/";
#endif

// TODO: instead, use key value pair table, which would be less fragile.
const char *const AppConfig::m_SettingsName[] = {
    "", // screenName moved to deskflow settings
    "", // port moved to deskflow settings
    "", // interface moved to deskflow settings
    "", // log level moved to deskflow settings
    "", // Log to file Moved to Deskflow settings
    "", // 5 logFilename, moved to deskflow settings
    "", // 6 wizardLastRun, obsolete
    "", // 7 statedBefore moved to deskflow settings
    "", // 8 elevateMode,
    "", // 9 elevateModeEnum,
    "", // 10 = edition, obsolete (using serial key instead)
    "", // 11 = kTlsEnabled (retain legacy string value) Moved to Settings
    "", // 12 AutoHide, moved to Settings
    "", // 13 = serialKey, obsolete
    "", // 14 last Version moved ot deskflow settings
    "", // 15 = lastExpiringWarningTime, obsolete
    "", // 16 = activationHasRun, obsolete
    "", // 17 = minimizeToTray, obsolete
    "", // 18 = ActivateEmail, obsolete
    "loadFromSystemScope",
    "", // kServerGroupChecked
    "", // 21 = use external config moved to deskflow settings
    "", // 22 config file moved to dekflow settings
    "", // 23 use internal config unsed, removed
    "", // Client groupchecked
    "", // 25 server host name moved to deskflow settings
    "", // 26 cert path moved to deskflow settings
    "", // 27 key length Moved to Deskflow settings
    "", // 28 Prevent sleep moved to deskflow settings
    "", // 29 Language sync moved to deskflow settings
    "", // 30 = invertscrolldriection moved to deskflow settings
    "", // 31 = guid, obsolete
    "", // 32 = licenseRegistryUrl, obsolete
    "", // 33 = licenseNextCheck, obsolete
    "", // 34 = kInvertConnection, obsolete
    "", // 35 = clientHostMode, obsolete
    "", // 36 = serverClientMode, obsolete
    "", // 37 enable service moved to deskflow settings
    "", // 38 Moved to deskflow settings
    "", // 39 window size moved to deskflow settings
    "", // 40 window position moved to deskflow settings
    "", // 41 = Show dev thanks, obsolete
    "", // 42 show Close Reminder moved to deskflow settings
    "", // 43 Moved to deskflow settings
    "", // 44, Moved to deskflow settings.
    "", // 45 Moved to deskflow settings
    "", // 46 require peer certs, Moved to deskflow settings
};

AppConfig::AppConfig(deskflow::gui::IConfigScopes &scopes, std::shared_ptr<Deps> deps) : m_Scopes(scopes), m_pDeps(deps)
{
  determineScope();
  recall();
}

void AppConfig::recall()
{
  using enum AppConfig::Setting;

  qDebug("recalling app config");

  recallFromAllScopes();
}

void AppConfig::recallFromAllScopes()
{
  using enum Setting;
  m_LoadFromSystemScope = findInAllScopes(kLoadSystemSettings, m_LoadFromSystemScope).toBool();
}

void AppConfig::commit()
{
  using enum Setting;

  qDebug("committing app config");

  saveToAllScopes(kLoadSystemSettings, m_LoadFromSystemScope);
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
  else if (m_Scopes.scopeContains(settingName(Setting::kLoadSystemSettings), ConfigScopes::Scope::System)) {
    qDebug("system settings scope contains screen name, using system scope");
    setLoadFromSystemScope(true);
  }
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
  if (m_Scopes.scopeContains(settingName(Setting::kLoadSystemSettings), m_Scopes.activeScope())) {
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

///////////////////////////////////////////////////////////////////////////////
// Begin getters
///////////////////////////////////////////////////////////////////////////////

IConfigScopes &AppConfig::scopes() const
{
  return m_Scopes;
}

///////////////////////////////////////////////////////////////////////////////
// End getters
///////////////////////////////////////////////////////////////////////////////
