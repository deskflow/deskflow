/*
 * Deskflow -- mouse and keyboard sharing utility
 * Copyright (C) 2013-2016 Symless Ltd.
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

#define TEST_ENV

#include "server/PrimaryClient.h"

#include <gmock/gmock.h>

class MockPrimaryClient : public PrimaryClient
{
public:
  MOCK_METHOD(void *, getEventTarget, (), (const, override));
  MOCK_METHOD(void, getCursorPos, (int32_t &, int32_t &), (const, override));
  MOCK_METHOD(void, setJumpCursorPos, (int32_t, int32_t), (const));
  MOCK_METHOD(void, reconfigure, (uint32_t), (override));
  MOCK_METHOD(void, resetOptions, (), (override));
  MOCK_METHOD(void, setOptions, (const OptionsList &), (override));
  MOCK_METHOD(void, enable, (), (override));
  MOCK_METHOD(void, disable, (), (override));
  MOCK_METHOD(uint32_t, registerHotKey, (KeyID, KeyModifierMask), (override));
  MOCK_METHOD(KeyModifierMask, getToggleMask, (), (const, override));
  MOCK_METHOD(void, unregisterHotKey, (uint32_t), (override));
};
