/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2008 Volker Lanz <vl@fidra.de>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "IAppConfig.h"
#include "IConfigScopes.h"
#include "common/constants.h"
#include "gui/paths.h"

#include <QDir>
#include <QObject>
#include <QPoint>
#include <QSize>
#include <QString>
#include <QVariant>
#include <optional>

/**
 * @brief Simply reads and writes app settings.
 *
 * Important: Maintain a clear separation of concerns and keep it simple.
 * It is tempting to add logic (e.g. license checks) to this class since it
 * instance is widely accessible, but that has previously led to this class
 * becoming a god object.
 */
class AppConfig : public QObject, public deskflow::gui::IAppConfig
{
  Q_OBJECT
  using IConfigScopes = deskflow::gui::IConfigScopes;

public:
  struct Deps
  {
    virtual ~Deps() = default;
  };

  explicit AppConfig(IConfigScopes &scopes, std::shared_ptr<Deps> deps = std::make_shared<Deps>());

  void determineScope();

  IConfigScopes &scopes() const override;
  bool isActiveScopeWritable() const override;
  bool isActiveScopeSystem() const override;

  /// @brief Sets the user preference to load from SystemScope.
  /// @param [in] value
  ///             True - This will set the variable and load the global scope
  ///             settings. False - This will set the variable and load the user
  ///             scope settings.
  void setLoadFromSystemScope(bool value) override;

private:
  /// @brief This method loads config from specified scope
  /// @param [in] scope which should be loaded.
  void loadScope(IConfigScopes::Scope scope);

  deskflow::gui::IConfigScopes &m_Scopes;
  std::shared_ptr<Deps> m_pDeps;
};
