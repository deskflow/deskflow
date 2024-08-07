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

#pragma once

#include "IConfigScopes.h"

#include <QObject>
#include <QSettings>
#include <QVariant>
#include <memory>

namespace synergy::gui {

/// @brief Encapsulates Qt config for both user and global scopes.
class ConfigScopes : public QObject, public IConfigScopes {
  Q_OBJECT

public:
  explicit ConfigScopes();
  ~ConfigScopes() override = default;

  void signalReady() override;
  void save() override;
  bool scopeContains(
      const QString &name, Scope scope = Scope::Current) const override;
  bool isActiveScopeWritable() const override;
  void setInScope(
      const QString &name, const QVariant &value,
      Scope scope = Scope::Current) override;
  QVariant getFromScope(
      const QString &name, const QVariant &defaultValue = QVariant(),
      Scope scope = Scope::Current) const override;
  void setActiveScope(Scope scope = Scope::User) override;
  Scope activeScope() const override;
  QSettings *activeSettings() const override;

signals:
  void ready();
  void saving();

private:
  Scope m_currentScope = Scope::User;
  std::unique_ptr<QSettings> m_pUserSettings;
  std::unique_ptr<QSettings> m_pSystemSettings;
};

} // namespace synergy::gui
