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

#pragma once

#include "BasicTypes.h"

//! Gamepad button ID
/*!
Type to hold a gamepad button identifier.
*/
typedef UInt8			GamepadButtonID;

//! Gamepad button ID
/*!
Type to hold a gamepad analog (stick/trigger) identifier.
*/
typedef UInt8			GamepadAnalogID;

//! @name Gamepad button identifiers
//@{
static const GamepadButtonID	kGamepadDpadUp        = 0;
static const GamepadButtonID	kGamepadDpadDown      = 1;
static const GamepadButtonID	kGamepadDpadLeft      = 2;
static const GamepadButtonID	kGamepadDpadRight     = 3;
static const GamepadButtonID	kGamepadButtonStart   = 4;
static const GamepadButtonID	kGamepadButtonBack    = 5;
static const GamepadButtonID	kGamepadLeftThumb     = 6;
static const GamepadButtonID	kGamepadRightThumb    = 7;
static const GamepadButtonID	kGamepadLeftShoulder  = 8;
static const GamepadButtonID	kGamepadRightShoulder = 9;
static const GamepadButtonID	kGamepadButtonA       = 10;
static const GamepadButtonID	kGamepadButtonB       = 11;
static const GamepadButtonID	kGamepadButtonX       = 12;
static const GamepadButtonID	kGamepadButtonY       = 13;
//@}

//! @name Gamepad analog identifiers
//@{
static const GamepadAnalogID	kGamepadLeftStick     = 0;
static const GamepadAnalogID	kGamepadRightStick    = 1;
static const GamepadAnalogID	kGamepadLeftTrigger   = 2;
static const GamepadAnalogID	kGamepadRightTrigger  = 3;
//@}

static const UInt8      NumGamepadButtonIDs  = 14;
