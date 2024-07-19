/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2020 Symless Ltd.
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

#include "Config.h"

#include "CommonConfig.h"

#include <QCoreApplication>
#include <QFile>
#include <cassert>
#include <memory>

namespace {

QString getSystemSettingPath() {
  const QString settingFilename("SystemConfig.ini");
  QString path;
#if defined(Q_OS_WIN)
  // Program file
  path = QCoreApplication::applicationDirPath() + "\\";
#elif defined(Q_OS_DARWIN)
  // Global preferances dir
  //  Would be nice to use /library, but QT has no elevate system in place
  path = "/usr/local/etc/symless/";
#elif defined(Q_OS_LINUX)
  // QT adds application and filename to the end of the path already on linux
  path = "/usr/local/etc/symless/";
  return path;
#else
  assert("OS not supported");
#endif
  return path + settingFilename;
}

#if defined(Q_OS_WIN)
void loadOldSystemSettings(QSettings &settings) {
  if (!QFile(settings.fileName()).exists()) {
    QSettings::setPath(
        QSettings::IniFormat, QSettings::SystemScope, "SystemConfig.ini");
    QSettings oldSystemSettings(
        QSettings::IniFormat, QSettings::SystemScope,
        QCoreApplication::organizationName(),
        QCoreApplication::applicationName());

    if (QFile(oldSystemSettings.fileName()).exists()) {
      for (const auto &key : oldSystemSettings.allKeys()) {
        settings.setValue(key, oldSystemSettings.value(key));
      }
    }

    // Restore system settings path
    QSettings::setPath(
        QSettings::IniFormat, QSettings::SystemScope, getSystemSettingPath());
  }
}
#endif

} // namespace

namespace synergy::gui {

std::unique_ptr<Config> Config::s_pSettings;

Config *Config::get() {
  if (!s_pSettings) {
    s_pSettings = std::make_unique<Config>();
  }
  return s_pSettings.get();
}

Config::Config() {
  QSettings::setPath(
      QSettings::Format::IniFormat, QSettings::Scope::SystemScope,
      getSystemSettingPath());

  // Config will default to User settings if they exist,
  //  otherwise it will load System setting and save them to User settings
  m_pSystemSettings = std::make_unique<QSettings>(
      QSettings::Format::IniFormat, QSettings::Scope::SystemScope,
      QCoreApplication::organizationName(),
      QCoreApplication::applicationName());

#if defined(Q_OS_WIN)
  // This call is needed for backwardcapability with old settings.
  loadOldSystemSettings(*m_pSystemSettings);
#endif

  // default to user scope.
  // if we set the scope specifically then we also have to set the application
  // name and the organisation name which breaks backwards compatibility.
  m_pUserSettings = std::make_unique<QSettings>();
}

Config::~Config() {
  while (!m_pCallerList.empty()) {
    m_pCallerList.pop_back();
  }
}

bool Config::hasSetting(const QString &name, Scope scope) const {
  switch (scope) {
  case Scope::User:
    return m_pUserSettings->contains(name);
  case Scope::System:
    return m_pSystemSettings->contains(name);
  default:
    return currentSettings()->contains(name);
  }
}

bool Config::isWritable() const { return currentSettings()->isWritable(); }

QVariant Config::loadSetting(
    const QString &name, const QVariant &defaultValue, Scope scope) {
  switch (scope) {
  case Scope::User:
    return m_pUserSettings->value(name, defaultValue);
  case Scope::System:
    return m_pSystemSettings->value(name, defaultValue);
  default:
    return currentSettings()->value(name, defaultValue);
  }
}

void Config::setScope(Config::Scope scope) { m_CurrentScope = scope; }

Config::Scope Config::getScope() const { return m_CurrentScope; }

void Config::globalLoad() {
  for (auto &i : m_pCallerList) {
    i->loadSettings();
  }
}

void Config::globalSave() {

  // Save if there are any unsaved changes otherwise skip
  if (unsavedChanges()) {
    for (auto &i : m_pCallerList) {
      i->saveSettings();
    }

    m_pUserSettings->sync();
    m_pSystemSettings->sync();

    m_unsavedChanges = false;
  }
}

QSettings *Config::currentSettings() const {
  if (m_CurrentScope == Scope::User) {
    return m_pUserSettings.get();
  } else {
    return m_pSystemSettings.get();
  }
}

void Config::registerClass(CommonConfig *receiver) {
  m_pCallerList.push_back(receiver);
}

bool Config::unsavedChanges() const {
  if (m_unsavedChanges) {
    return true;
  }

  for (const auto &i : m_pCallerList) {
    if (i->modified()) {
      // If any class returns true there is no point checking more
      return true;
    }
  }
  // If this line is reached no class has unsaved changes
  return false;
}

void Config::markUnsaved() { m_unsavedChanges = true; }

} // namespace synergy::gui
