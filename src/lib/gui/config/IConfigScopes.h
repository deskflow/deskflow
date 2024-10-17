/*
 * Deskflow -- mouse and keyboard sharing utility
 * Copyright (C) 2024 Symless Ltd.
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

#pragma once

#include "gui/proxy/QSettingsProxy.h"

#include <QSettings>
#include <QString>
#include <QVariant>

namespace deskflow::gui {

class IConfigScopes
{
  using QSettingsProxy = deskflow::gui::proxy::QSettingsProxy;

public:
  enum class Scope
  {
    Current,
    System,
    User
  };

  virtual ~IConfigScopes() = default;

  virtual Scope activeScope() const = 0;
  virtual void setActiveScope(Scope scope = Scope::User) = 0;
  virtual bool isActiveScopeWritable() const = 0;
  virtual QSettingsProxy &activeSettings() = 0;
  virtual const QSettingsProxy &activeSettings() const = 0;
  virtual QString activeFilePath() const = 0;

  /**
   * @brief Signals to listeners that the settings that they should read.
   */
  virtual void signalReady() = 0;

  /**
   * @brief Signals to listeners to save and calls `sync` on underlying Qt
   * config.
   *
   * @param emitSaving Whether to emit the saving signal which typically
   * triggers listeners to write their current state to the config.
   */
  virtual void save(bool emitSaving = true) = 0;

  /**
   * @brief Check a scope for a config value (default is current scope).
   */
  virtual bool scopeContains(const QString &name, Scope scope = Scope::Current) const = 0;

  /**
   * @brief Load a config value from a scope (default is current scope).
   */
  virtual QVariant
  getFromScope(const QString &name, const QVariant &defaultValue = QVariant(), Scope scope = Scope::Current) const = 0;

  /**
   * @brief Set a config value in a scope (default is current scope).
   */
  virtual void setInScope(const QString &name, const QVariant &value, Scope scope = Scope::Current) = 0;
};

} // namespace deskflow::gui
