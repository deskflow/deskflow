/*
 * Deskflow -- mouse and keyboard sharing utility
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

#include "proxy/QSettingsProxy.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QtCore/qfileinfo.h>
#include <QtCore/qlogging.h>
#include <memory>

namespace deskflow::gui {

using namespace proxy;

//
// ConfigScopes::Deps
//

std::shared_ptr<QSettingsProxy> ConfigScopes::Deps::makeUserSettings()
{
  return std::make_shared<QSettingsProxy>();
}

std::shared_ptr<QSettingsProxy> ConfigScopes::Deps::makeSystemSettings()
{
  return std::make_shared<QSettingsProxy>();
}

//
// ConfigScopes
//

ConfigScopes::ConfigScopes(std::shared_ptr<Deps> deps)
    : m_pUserSettingsProxy(deps->makeUserSettings()),
      m_pSystemSettingsProxy(deps->makeSystemSettings())
{

  m_pUserSettingsProxy->loadUser();
  m_pSystemSettingsProxy->loadSystem();
}

void ConfigScopes::clear() const
{
  m_pUserSettingsProxy->clear();
  m_pSystemSettingsProxy->clear();
}

void ConfigScopes::signalReady()
{
  Q_EMIT ready();
}

void ConfigScopes::save(bool emitSaving)
{
  if (emitSaving) {
    qDebug("emitting config saving signal");
    Q_EMIT saving();
  }

  qDebug("writing config to filesystem");
  m_pUserSettingsProxy->sync();
  m_pSystemSettingsProxy->sync();
}

bool ConfigScopes::isActiveScopeWritable() const
{
  return activeSettings().isWritable();
}

void ConfigScopes::setActiveScope(ConfigScopes::Scope scope)
{
  m_currentScope = scope;
}

ConfigScopes::Scope ConfigScopes::activeScope() const
{
  return m_currentScope;
}

bool ConfigScopes::scopeContains(const QString &name, Scope scope) const
{
  switch (scope) {
  case Scope::User:
    return m_pUserSettingsProxy->contains(name);
  case Scope::System:
    return m_pSystemSettingsProxy->contains(name);
  default:
    return activeSettings().contains(name);
  }
}

QSettingsProxy &ConfigScopes::activeSettings()
{
  if (m_currentScope == Scope::User) {
    return *m_pUserSettingsProxy;
  } else {
    return *m_pSystemSettingsProxy;
  }
}

const QSettingsProxy &ConfigScopes::activeSettings() const
{
  if (m_currentScope == Scope::User) {
    return *m_pUserSettingsProxy;
  } else {
    return *m_pSystemSettingsProxy;
  }
}

QString ConfigScopes::activeFilePath() const
{
  return activeSettings().fileName();
}

QVariant ConfigScopes::getFromScope(const QString &name, const QVariant &defaultValue, Scope scope) const
{
  switch (scope) {
  case Scope::User:
    return m_pUserSettingsProxy->value(name, defaultValue);
  case Scope::System:
    return m_pSystemSettingsProxy->value(name, defaultValue);
  default:
    return activeSettings().value(name, defaultValue);
  }
}

void ConfigScopes::setInScope(const QString &name, const QVariant &value, Scope scope)
{
  switch (scope) {
  case Scope::User:
    m_pUserSettingsProxy->setValue(name, value);
    break;
  case Scope::System:
    m_pSystemSettingsProxy->setValue(name, value);
    break;
  default:
    activeSettings().setValue(name, value);
    break;
  }
}

} // namespace deskflow::gui
