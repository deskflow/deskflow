/*
 * synergy -- mouse and keyboard sharing utility
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

#include "gui/CommonConfig.h"

#include <QSettings>
#include <QString>
#include <QVariant>

namespace synergy::gui {

class IConfigScopes {
public:
  enum class Scope { Current, System, User };

  virtual ~IConfigScopes() = default;

  virtual void registerReceiver(CommonConfig *receiver) = 0;
  virtual void loadAll() = 0;
  virtual bool
  hasSetting(const QString &name, Scope scope = Scope::Current) const = 0;
  virtual QVariant loadSetting(
      const QString &name, const QVariant &defaultValue = QVariant(),
      Scope scope = Scope::Current) const = 0;
  virtual void setSetting(
      const QString &name, const QVariant &value,
      Scope scope = Scope::Current) = 0;
  virtual Scope getScope() const = 0;
  virtual void setScope(Scope scope = Scope::User) = 0;
  virtual bool isWritable() const = 0;
  virtual QSettings *currentSettings() const = 0;
  virtual void saveAll() = 0;
};

} // namespace synergy::gui
