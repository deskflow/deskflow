/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2011 Chris Schoeneman, Nick Bolton, Sorin Sbarnea
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "CMockKeyMap.h"
#include "CMockEventQueue.h"
#include "CXWindowsKeyState.h"

class CXWindowsKeyStateTests : public ::testing::Test
{
protected:
	virtual void
	SetUp()
	{
		m_display = XOpenDisplay(NULL);
	}

	virtual void
	TearDown()
	{
	}

	Display* m_display;
};

TEST_F(CXWindowsKeyStateTests, setActiveGroup_pollAndSet_groupIsZero)
{
	CMockKeyMap keyMap;
	CMockEventQueue eventQueue;
	CXWindowsKeyState keyState(
		m_display, true, (IEventQueue&)keyMap, (CKeyMap&)eventQueue);

	keyState.setActiveGroup(CXWindowsKeyState::kGroupPollAndSet);

	ASSERT_EQ(0, keyState.getGroup());
}

TEST_F(CXWindowsKeyStateTests, setActiveGroup_poll_groupIsZero)
{
	CMockKeyMap keyMap;
	CMockEventQueue eventQueue;
	CXWindowsKeyState keyState(
		m_display, true, (IEventQueue&)keyMap, (CKeyMap&)eventQueue);

	keyState.setActiveGroup(CXWindowsKeyState::kGroupPoll);

	ASSERT_EQ(-1, keyState.getGroup());
}

TEST_F(CXWindowsKeyStateTests, setActiveGroup_customGroup_groupWasSet)
{
	CMockKeyMap keyMap;
	CMockEventQueue eventQueue;
	CXWindowsKeyState keyState(
		m_display, true, (IEventQueue&)keyMap, (CKeyMap&)eventQueue);

	keyState.setActiveGroup(1);

	ASSERT_EQ(1, keyState.getGroup());
}

TEST_F(CXWindowsKeyStateTests, mapModifiersFromX_zeroState_zeroMask)
{
	CMockKeyMap keyMap;
	CMockEventQueue eventQueue;
	CXWindowsKeyState keyState(
		m_display, true, (IEventQueue&)keyMap, (CKeyMap&)eventQueue);

	int mask = keyState.mapModifiersFromX(0);

	ASSERT_EQ(0, mask);
}

TEST_F(CXWindowsKeyStateTests, mapModifiersToX_zeroMask_resultIsTrue)
{
	CMockKeyMap keyMap;
	CMockEventQueue eventQueue;
	CXWindowsKeyState keyState(
		m_display, true, (IEventQueue&)keyMap, (CKeyMap&)eventQueue);

	unsigned int modifiers = 0;
	bool result = keyState.mapModifiersToX(0, modifiers);

	ASSERT_TRUE(result);
}

TEST_F(CXWindowsKeyStateTests, fakeCtrlAltDel_default_returnsFalse)
{
	CMockKeyMap keyMap;
	CMockEventQueue eventQueue;
	CXWindowsKeyState keyState(
		m_display, true, (IEventQueue&)keyMap, (CKeyMap&)eventQueue);

	bool result = keyState.fakeCtrlAltDel();

	ASSERT_FALSE(result);
}
