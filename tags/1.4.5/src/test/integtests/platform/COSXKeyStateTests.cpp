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

#include "COSXKeyState.h"
#include "CMockKeyMap.h"
#include "CMockEventQueue.h"

CGKeyCode escKeyCode = 53;
CGKeyCode shiftKeyCode = 56;
CGKeyCode controlKeyCode = 59;

// TODO: make pollActiveModifiers tests work reliably. 
/*
TEST(COSXKeyStateTests, pollActiveModifiers_shiftKeyDownThenUp_masksAreCorrect)
{
	CMockKeyMap keyMap;
	CMockEventQueue eventQueue;
	COSXKeyState keyState((IEventQueue&)keyMap, (CKeyMap&)eventQueue);

	// fake shift key down (without using synergy). this is a bit weird;
	// looks like you need to create a shift down event *and* set the
	// shift modifier.
	CGEventRef shiftDown = CGEventCreateKeyboardEvent(NULL, shiftKeyCode, true);
	CGEventSetFlags(shiftDown, kCGEventFlagMaskShift);
	CGEventPost(kCGHIDEventTap, shiftDown);
	CFRelease(shiftDown);

	// function under test (1st call)
	KeyModifierMask downMask = keyState.pollActiveModifiers();

	// fake shift key up (without using synergy). also as weird as the
	// shift down; use a non-shift key down and reset the pressed modifiers.
	CGEventRef shiftUp = CGEventCreateKeyboardEvent(NULL, escKeyCode, true);
	CGEventSetFlags(shiftUp, 0);
	CGEventPost(kCGHIDEventTap, shiftUp);
	CFRelease(shiftUp);

	// function under test (2nd call)
	KeyModifierMask upMask = keyState.pollActiveModifiers();

	EXPECT_TRUE((downMask & KeyModifierShift) == KeyModifierShift)
		<< "shift key not in mask (" << downMask << ") - key was not pressed";

	EXPECT_TRUE((upMask & KeyModifierShift) == 0)
		<< "shift key still in mask (" << upMask << ") - make sure no keys are being held down";
}

TEST(COSXKeyStateTests, pollActiveModifiers_controlKeyDownThenUp_masksAreCorrect)
{
	CMockKeyMap keyMap;
	CMockEventQueue eventQueue;
	COSXKeyState keyState((IEventQueue&)keyMap, (CKeyMap&)eventQueue);

	// fake control key down (without using synergy). this is a bit weird;
	// looks like you need to create a shift down event *and* set the
	// shift modifier.
	CGEventRef controlDown = CGEventCreateKeyboardEvent(NULL, controlKeyCode, true);
	CGEventSetFlags(controlDown, kCGEventFlagMaskControl);
	CGEventPost(kCGHIDEventTap, controlDown);
	CFRelease(controlDown);

	// function under test (1st call)
	KeyModifierMask downMask = keyState.pollActiveModifiers();

	// fake control key up (without using synergy). also as weird as the
	// shift down; use a non-shift key down and reset the pressed modifiers.
	CGEventRef controlUp = CGEventCreateKeyboardEvent(NULL, escKeyCode, true);
	CGEventSetFlags(controlUp, 0);
	CGEventPost(kCGHIDEventTap, controlUp);
	CFRelease(controlUp);

	// function under test (2nd call)
	KeyModifierMask upMask = keyState.pollActiveModifiers();

	EXPECT_TRUE((downMask & KeyModifierControl) == KeyModifierControl)
		<< "control key not in mask (" << downMask << ") - key was not pressed";

	EXPECT_TRUE((upMask & KeyModifierControl) == 0)
		<< "control key still in mask (" << upMask << ") - make sure no keys are being held down";
}
*/
