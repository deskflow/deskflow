/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "gui/config/IServerConfig.h"

#include <gmock/gmock.h>

class ServerConfigMock : public deskflow::gui::IServerConfig
{
public:
  ServerConfigMock()
  {
    ON_CALL(*this, screens()).WillByDefault(testing::ReturnRef(m_screensStub));
  }

  MOCK_METHOD(bool, isFull, (), (const, override));
  MOCK_METHOD(bool, screenExists, (const QString &screenName), (const, override));
  MOCK_METHOD(bool, save, (const QString &fileName), (const, override));
  MOCK_METHOD(void, save, (QFile & file), (const, override));
  MOCK_METHOD(bool, enableDragAndDrop, (), (const, override));
  MOCK_METHOD(const ScreenList &, screens, (), (const, override));

private:
  ScreenList m_screensStub;
};
