/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2011 Nick Bolton
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once
#include "deskflow/KeyMap.h"

#include <gmock/gmock.h>

class MockKeyMap : public deskflow::KeyMap
{
public:
  MOCK_METHOD(void, swap, (KeyMap &), (override));
  MOCK_METHOD(void, finish, (), (override));
  MOCK_METHOD(void, foreachKey, (ForeachKeyCallback, void *), (override));
  MOCK_METHOD(void, addHalfDuplexModifier, (KeyID), (override));
  MOCK_METHOD(bool, isHalfDuplex, (KeyID, KeyButton), (const, override));
  MOCK_METHOD(
      const KeyMap::KeyItem *, mapKey,
      (Keystrokes &, KeyID, int32_t, ModifierToKeys &, KeyModifierMask &, KeyModifierMask, bool, const std::string &),
      (const, override)
  );
};
