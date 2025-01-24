/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
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
