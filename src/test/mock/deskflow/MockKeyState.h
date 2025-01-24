/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2011 Nick Bolton
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "deskflow/KeyState.h"

#include <gmock/gmock.h>

class MockKeyMap;
class MockEventQueue;

// NOTE: do not mock methods that are not pure virtual. this mock exists only
// to provide an implementation of the KeyState abstract class.
class MockKeyState : public KeyState
{
public:
  MockKeyState(const MockEventQueue &eventQueue) : KeyState((IEventQueue *)&eventQueue, {"en"}, true)
  {
  }

  MockKeyState(const MockEventQueue &eventQueue, const deskflow::KeyMap &keyMap)
      : KeyState((IEventQueue *)&eventQueue, (deskflow::KeyMap &)keyMap, {"en"}, true)
  {
  }

  MOCK_METHOD(int32_t, pollActiveGroup, (), (const, override));
  MOCK_METHOD(KeyModifierMask, pollActiveModifiers, (), (const, override));
  MOCK_METHOD(bool, fakeCtrlAltDel, (), (override));
  MOCK_METHOD(void, getKeyMap, (deskflow::KeyMap &), (override));
  MOCK_METHOD(void, fakeKey, (const Keystroke &), (override));
  MOCK_METHOD(bool, fakeMediaKey, (KeyID), (override));
  MOCK_METHOD(void, pollPressedKeys, (KeyButtonSet &), (const, override));
};

typedef ::testing::NiceMock<MockKeyState> KeyStateImpl;

using KeyID = uint32_t;

typedef void (*ForeachKeyCallback)(KeyID, int32_t group, deskflow::KeyMap::KeyItem &, void *userData);
