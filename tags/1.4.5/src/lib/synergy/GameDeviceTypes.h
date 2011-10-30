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

//! Game device ID
/*!
Type to hold a game device ID.
*/
typedef UInt8			GameDeviceID;

//! Game device button
/*!
Type to hold a game device button.
*/
typedef UInt16			GameDeviceButton;

//! @name Game device buttons
//@{
static const GameDeviceButton	kGameDeviceDpadUp        = 0x0001;
static const GameDeviceButton	kGameDeviceDpadDown      = 0x0002;
static const GameDeviceButton	kGameDeviceDpadLeft      = 0x0004;
static const GameDeviceButton	kGameDeviceDpadRight     = 0x0008;
static const GameDeviceButton	kGameDeviceButtonStart   = 0x0010;
static const GameDeviceButton	kGameDeviceButtonBack    = 0x0020;
static const GameDeviceButton	kGameDeviceThumb1        = 0x0040;
static const GameDeviceButton	kGameDeviceThumb2        = 0x0080;
static const GameDeviceButton	kGameDeviceShoulder1     = 0x0100;
static const GameDeviceButton	kGameDeviceShoulder2     = 0x0200;
static const GameDeviceButton	kGameDeviceButton1       = 0x1000;
static const GameDeviceButton	kGameDeviceButton2       = 0x2000;
static const GameDeviceButton	kGameDeviceButton3       = 0x4000;
static const GameDeviceButton	kGameDeviceButton4       = 0x8000;
//@}

static const UInt8      NumGameDeviceButtons  = 14;
