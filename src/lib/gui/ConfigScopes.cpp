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

#include "ConfigScopes.h"

#include "CommonConfig.h"

#include <QCoreApplication>
#include <QFile>
#include <cassert>
#include <memory>

namespace synergy::gui {

QString getSystemSettingPath() {
  const QString settingFilename("SystemConfig.ini");
  QString path;
#if defined(Q_OS_WIN)
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
void loadWindowsLegacy(QSettings &settings) {
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

ConfigScopes::ConfigScopes() {
  QSettings::setPath(
      QSettings::Format::IniFormat, QSettings::Scope::SystemScope,
      getSystemSettingPath());

  // Config will default to User settings if they exist,
  //  otherwise it will load System setting and save them to User settings
  m_pSystemSettings = std::make_unique<QSettings>(
      QSettings::Format::IniFormat, QSettings::Scope::SystemScope,
      QCoreApplication::organizationName(),
      QCoreApplication::applicationName());

  // default to user scope.
  // if we set the scope specifically then we also have to set the application
  // name and the organisation name which breaks backwards compatibility.
  m_pUserSettings = std::make_unique<QSettings>();

  load();
}

ConfigScopes::~ConfigScopes() {
  while (!m_pReceievers.empty()) {
    m_pReceievers.pop_back();
  }
}

void ConfigScopes::load() {
#if defined(Q_OS_WIN)
  // This call is needed for backwardcapability with old settings.
  loadWindowsLegacy(*m_pSystemSettings);
#endif
}

bool ConfigScopes::hasSetting(const QString &name, Scope scope) const {
  switch (scope) {
  case Scope::User:
    return m_pUserSettings->contains(name);
  case Scope::System:
    return m_pSystemSettings->contains(name);
  default:
    return currentSettings()->contains(name);
  }
}

bool ConfigScopes::isWritable() const { return currentSettings()->isWritable(); }

QVariant ConfigScopes::loadSetting(
    const QString &name, const QVariant &defaultValue, Scope scope) const {
  switch (scope) {
  case Scope::User:
    return m_pUserSettings->value(name, defaultValue);
  case Scope::System:
    return m_pSystemSettings->value(name, defaultValue);
  default:
    return currentSettings()->value(name, defaultValue);
  }
}

void ConfigScopes::setScope(ConfigScopes::Scope scope) { m_CurrentScope = scope; }

ConfigScopes::Scope ConfigScopes::getScope() const { return m_CurrentScope; }

void ConfigScopes::loadAll() {
  for (auto &i : m_pReceievers) {
    i->loadSettings();
  }
}

void ConfigScopes::saveAll() {

  // Save if there are any unsaved changes otherwise skip
  if (unsavedChanges()) {
    for (auto &i : m_pReceievers) {
      i->saveSettings();
    }

    m_pUserSettings->sync();
    m_pSystemSettings->sync();

    m_unsavedChanges = false;
  }
}

QSettings *ConfigScopes::currentSettings() const {
  if (m_CurrentScope == Scope::User) {
    return m_pUserSettings.get();
  } else {
    return m_pSystemSettings.get();
  }
}

void ConfigScopes::registerReceiever(CommonConfig *receiver) {
  m_pReceievers.push_back(receiver);
}

bool ConfigScopes::unsavedChanges() const {
  if (m_unsavedChanges) {
    return true;
  }

  for (const auto &i : m_pReceievers) {
    if (i->modified()) {
      return true;
    }
  }
  return false;
}

void ConfigScopes::markUnsaved() { m_unsavedChanges = true; }

} // namespace synergy::gui
