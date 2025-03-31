/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2011 Nick Bolton
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

// TODO: fix  Assertion `s_instance != nullptr' failed.
#if 0

#define TEST_ENV

#include "test/mock/deskflow/MockEventQueue.h"
#include "test/mock/deskflow/MockKeyMap.h"

#include "base/Log.h"
#include "platform/XWindowsKeyState.h"

#define XK_LATIN1
#define XK_MISCELLANY
#include <X11/keysymdef.h>

#if HAVE_XKB_EXTENSION
#include <X11/XKBlib.h>
#endif

#include <errno.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

class XWindowsKeyStateTests : public ::testing::Test {
protected:
  XWindowsKeyStateTests() : m_display(NULL) {}

  ~XWindowsKeyStateTests() {
    if (m_display != NULL) {
      LOG((CLOG_DEBUG "closing display"));
      XCloseDisplay(m_display);
    }
  }

  virtual void SetUp() {
    // open the display only once for the entire test suite
    if (this->m_display == NULL) {
      LOG((CLOG_DEBUG "opening display"));
      this->m_display = XOpenDisplay(NULL);

      ASSERT_TRUE(this->m_display != NULL)
          << "unable to open display: " << errno;
    }
  }

  virtual void TearDown() {}

  Display *m_display;
};

TEST_F(XWindowsKeyStateTests, setActiveGroup_pollAndSet_groupIsZero) {
  MockKeyMap keyMap;
  MockEventQueue eventQueue;
  XWindowsKeyState keyState(m_display, true, &eventQueue, keyMap);

  keyState.setActiveGroup(XWindowsKeyState::kGroupPollAndSet);

  ASSERT_EQ(0, keyState.group());
}

TEST_F(XWindowsKeyStateTests, setActiveGroup_poll_groupIsNotSet) {
  MockKeyMap keyMap;
  MockEventQueue eventQueue;
  XWindowsKeyState keyState(m_display, true, &eventQueue, keyMap);

  keyState.setActiveGroup(XWindowsKeyState::kGroupPoll);

  ASSERT_LE(-1, keyState.group());
}

TEST_F(XWindowsKeyStateTests, setActiveGroup_customGroup_groupWasSet) {
  MockKeyMap keyMap;
  MockEventQueue eventQueue;
  XWindowsKeyState keyState(m_display, true, &eventQueue, keyMap);

  keyState.setActiveGroup(1);

  ASSERT_EQ(1, keyState.group());
}

TEST_F(XWindowsKeyStateTests, mapModifiersFromX_zeroState_zeroMask) {
  MockKeyMap keyMap;
  MockEventQueue eventQueue;
  XWindowsKeyState keyState(m_display, true, &eventQueue, keyMap);

  int mask = keyState.mapModifiersFromX(0);

  ASSERT_EQ(0, mask);
}

TEST_F(XWindowsKeyStateTests, mapModifiersToX_zeroMask_resultIsTrue) {
  MockKeyMap keyMap;
  MockEventQueue eventQueue;
  XWindowsKeyState keyState(m_display, true, &eventQueue, keyMap);

  unsigned int modifiers = 0;
  bool result = keyState.mapModifiersToX(0, modifiers);

  ASSERT_TRUE(result);
}

TEST_F(XWindowsKeyStateTests, fakeCtrlAltDel_default_returnsFalse) {
  MockKeyMap keyMap;
  MockEventQueue eventQueue;
  XWindowsKeyState keyState(m_display, true, &eventQueue, keyMap);

  bool result = keyState.fakeCtrlAltDel();

  ASSERT_FALSE(result);
}

TEST_F(XWindowsKeyStateTests, pollActiveModifiers_defaultState_returnsZero) {
  MockKeyMap keyMap;
  MockEventQueue eventQueue;
  XWindowsKeyState keyState(m_display, true, &eventQueue, keyMap);

  KeyModifierMask actual = keyState.pollActiveModifiers();

  ASSERT_EQ(0, actual);
}

#if 0 // TODO: fix, causes sigsegv
TEST_F(XWindowsKeyStateTests, pollActiveModifiers_shiftKeyDownThenUp_masksAreCorrect)
{
    MockKeyMap keyMap;
    MockEventQueue eventQueue;
    XWindowsKeyState keyState(
        m_display, true, &eventQueue, keyMap);

    // set mock modifier mapping
    std::fill(keyState.modifierFromX().begin(), keyState.modifierFromX().end(), 0);
    keyState.modifierFromX()[ShiftMapIndex] = KeyModifierShift;

    KeyCode key = XKeysymToKeycode(m_display, XK_Shift_L);

    // fake shift key down (without using deskflow)
    XTestFakeKeyEvent(m_display, key, true, CurrentTime);

    // function under test (1st call)
    KeyModifierMask modDown = keyState.pollActiveModifiers();

    // fake shift key up (without using deskflow)
    XTestFakeKeyEvent(m_display, key, false, CurrentTime);

    // function under test (2nd call)
    KeyModifierMask modUp = keyState.pollActiveModifiers();

    EXPECT_TRUE((modDown & KeyModifierShift) == KeyModifierShift)
        << "shift key not in mask - key was not pressed";

    EXPECT_TRUE((modUp & KeyModifierShift) == 0)
        << "shift key still in mask - make sure no keys are being held down";
}
#endif

TEST_F(XWindowsKeyStateTests, pollActiveGroup_defaultState_returnsZero) {
  MockKeyMap keyMap;
  MockEventQueue eventQueue;
  XWindowsKeyState keyState(m_display, true, &eventQueue, keyMap);

  int32_t actual = keyState.pollActiveGroup();

  ASSERT_EQ(0, actual);
}

TEST_F(XWindowsKeyStateTests, pollActiveGroup_positiveGroup_returnsGroup) {
  MockKeyMap keyMap;
  MockEventQueue eventQueue;
  XWindowsKeyState keyState(m_display, true, &eventQueue, keyMap);

  keyState.group(3);

  int32_t actual = keyState.pollActiveGroup();

  ASSERT_EQ(3, actual);
}

TEST_F(XWindowsKeyStateTests, pollActiveGroup_xkb_areEqual) {
#if HAVE_XKB_EXTENSION
  MockKeyMap keyMap;
  MockEventQueue eventQueue;
  XWindowsKeyState keyState(m_display, true, &eventQueue, keyMap);

  // reset the group
  keyState.group(-1);

  XkbStateRec state;

  // compare pollActiveGroup() with XkbGetState()
  if (XkbGetState(m_display, XkbUseCoreKbd, &state) == Success) {
    int32_t actual = keyState.pollActiveGroup();

    ASSERT_EQ(state.group, actual);
  } else {
    FAIL() << "XkbGetState() returned error " << errno;
  }
#else
  SUCCEED() << "Xkb extension not installed";
#endif
}
#endif
