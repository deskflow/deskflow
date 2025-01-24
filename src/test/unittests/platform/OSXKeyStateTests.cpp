/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2011 Nick Bolton
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/OSXKeyState.h"
#include "test/mock/deskflow/MockEventQueue.h"
#include "test/mock/deskflow/MockKeyMap.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

TEST(OSXKeyStateTests, mapModifiersFromOSX_OSXMask_returnDeskflowMask)
{
  deskflow::KeyMap keyMap;
  MockEventQueue eventQueue;
  OSXKeyState keyState(&eventQueue, keyMap, {"en"}, true);

  KeyModifierMask outMask = 0;

  uint32_t shiftMask = 0 | kCGEventFlagMaskShift;
  outMask = keyState.mapModifiersFromOSX(shiftMask);
  EXPECT_EQ(KeyModifierShift, outMask);

  uint32_t ctrlMask = 0 | kCGEventFlagMaskControl;
  outMask = keyState.mapModifiersFromOSX(ctrlMask);
  EXPECT_EQ(KeyModifierControl, outMask);

  uint32_t altMask = 0 | kCGEventFlagMaskAlternate;
  outMask = keyState.mapModifiersFromOSX(altMask);
  EXPECT_EQ(KeyModifierAlt, outMask);

  uint32_t cmdMask = 0 | kCGEventFlagMaskCommand;
  outMask = keyState.mapModifiersFromOSX(cmdMask);
  EXPECT_EQ(KeyModifierSuper, outMask);

  uint32_t capsMask = 0 | kCGEventFlagMaskAlphaShift;
  outMask = keyState.mapModifiersFromOSX(capsMask);
  EXPECT_EQ(KeyModifierCapsLock, outMask);

  uint32_t numMask = 0 | kCGEventFlagMaskNumericPad;
  outMask = keyState.mapModifiersFromOSX(numMask);
  EXPECT_EQ(KeyModifierNumLock, outMask);
}
