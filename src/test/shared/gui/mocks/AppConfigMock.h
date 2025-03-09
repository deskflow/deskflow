/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "gui/config/IAppConfig.h"

#include "gui/config/ElevateMode.h"

#include <gmock/gmock.h>

class AppConfigMock : public deskflow::gui::IAppConfig
{

public:
  AppConfigMock()
  {
  }

  //
  // Getters
  //

  MOCK_METHOD(deskflow::gui::IConfigScopes &, scopes, (), (const, override));
  MOCK_METHOD(ElevateMode, elevateMode, (), (const, override));
  MOCK_METHOD(bool, isActiveScopeSystem, (), (const, override));
  MOCK_METHOD(bool, isActiveScopeWritable, (), (const, override));
  MOCK_METHOD(bool, clientGroupChecked, (), (const, override));

  //
  // Setters
  //

  MOCK_METHOD(void, setLoadFromSystemScope, (bool loadFromSystemScope), (override));
  MOCK_METHOD(void, setElevateMode, (ElevateMode elevateMode), (override));

private:
  const QString m_stub = "stub";
};
