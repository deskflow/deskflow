/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2020 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "IConfigScopes.h"

#include <QObject>
#include <QSettings>
#include <QVariant>
#include <memory>

namespace deskflow::gui {

/// @brief Encapsulates Qt config for both user and global scopes.
class ConfigScopes : public QObject, public IConfigScopes
{
  using QSettingsProxy = deskflow::gui::proxy::QSettingsProxy;

  Q_OBJECT

public:
  struct Deps
  {
    virtual ~Deps() = default;
    virtual std::shared_ptr<QSettingsProxy> makeUserSettings();
    virtual std::shared_ptr<QSettingsProxy> makeSystemSettings();
  };

  explicit ConfigScopes(std::shared_ptr<Deps> deps = std::make_shared<Deps>());
  ~ConfigScopes() override = default;

  void clear() const;

  void signalReady() override;
  void save(bool emitSaving = true) override;
  bool scopeContains(const QString &name, Scope scope = Scope::Current) const override;
  bool isActiveScopeWritable() const override;
  void setInScope(const QString &name, const QVariant &value, Scope scope = Scope::Current) override;
  QVariant getFromScope(const QString &name, const QVariant &defaultValue = QVariant(), Scope scope = Scope::Current)
      const override;
  void setActiveScope(Scope scope = Scope::User) override;
  Scope activeScope() const override;
  QSettingsProxy &activeSettings() override;
  const QSettingsProxy &activeSettings() const override;
  QString activeFilePath() const override;

signals:
  void ready();
  void saving();

private:
  Scope m_currentScope = Scope::User;
  std::shared_ptr<QSettingsProxy> m_pUserSettingsProxy;
  std::shared_ptr<QSettingsProxy> m_pSystemSettingsProxy;
};

} // namespace deskflow::gui
