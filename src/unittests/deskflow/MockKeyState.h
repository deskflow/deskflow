/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2011 Nick Bolton
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */
#pragma once

#include "base/IEventQueue.h"
#include "deskflow/KeyState.h"

// NOTE: do not mock methods that are not pure virtual. this mock exists only
// to provide an implementation of the KeyState abstract class.
class MockKeyState : public KeyState
{
public:
  MockKeyState(const IEventQueue &eventQueue) : KeyState((IEventQueue *)&eventQueue, {"en"}, true)
  {
  }

  MockKeyState(const IEventQueue &eventQueue, const deskflow::KeyMap &keyMap)
      : KeyState((IEventQueue *)&eventQueue, (deskflow::KeyMap &)keyMap, {"en"}, true)
  {
  }

  int32_t pollActiveGroup() const override
  {
    return 0;
  }
  KeyModifierMask pollActiveModifiers() const override
  {
    return 0;
  }
  bool fakeCtrlAltDel() override
  {
    return false;
  }
  void getKeyMap(deskflow::KeyMap &) override
  {
  }
  void fakeKey(const Keystroke &) override
  {
  }
  bool fakeMediaKey(KeyID) override
  {
    return false;
  }
  void pollPressedKeys(KeyButtonSet &pressedKeys) const override
  {
    pressedKeys.insert(1);
  }
  void fakeKeyDown(KeyID id, KeyModifierMask mask, KeyButton serverID, const std::string &lang) override
  {
    m_key.m_button = id;
  }

  bool isKeyDown(KeyButton id) const override
  {
    return m_key.m_button == id;
  }

  void updateKeyState() override
  {
    // do Nothing
  }

private:
  deskflow::KeyMap::KeyItem m_key;
};

using KeyID = uint32_t;
