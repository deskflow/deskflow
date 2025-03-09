/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "gui/config/IConfigScopes.h"

#include <QString>

namespace deskflow::gui {

class IAppConfig
{
  using IConfigScopes = deskflow::gui::IConfigScopes;

public:
  virtual ~IAppConfig() = default;

  //
  // Getters
  //

  virtual IConfigScopes &scopes() const = 0;
  virtual bool isActiveScopeSystem() const = 0;
  virtual bool isActiveScopeWritable() const = 0;
  virtual bool clientGroupChecked() const = 0;

  //
  // Setters
  //

  virtual void setLoadFromSystemScope(bool loadFromSystemScope) = 0;
};

} // namespace deskflow::gui
