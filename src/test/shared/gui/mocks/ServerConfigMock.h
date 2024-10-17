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
