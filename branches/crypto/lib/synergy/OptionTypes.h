/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2002 Chris Schoeneman
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef OPTIONTYPES_H
#define OPTIONTYPES_H

#include "BasicTypes.h"
#include "stdvector.h"

//! Option ID
/*!
Type to hold an option identifier.
*/
typedef UInt32			OptionID;

//! Option Value
/*!
Type to hold an option value.
*/
typedef SInt32			OptionValue;

// for now, options are just pairs of integers
typedef std::vector<UInt32> COptionsList;

// macro for packing 4 character strings into 4 byte integers
#define OPTION_CODE(_s) 											\
	(static_cast<UInt32>(static_cast<unsigned char>(_s[0]) << 24) |	\
	 static_cast<UInt32>(static_cast<unsigned char>(_s[1]) << 16) |	\
	 static_cast<UInt32>(static_cast<unsigned char>(_s[2]) <<  8) |	\
	 static_cast<UInt32>(static_cast<unsigned char>(_s[3])      ))

//! @name Option identifiers
//@{
static const OptionID	kOptionHalfDuplexCapsLock     = OPTION_CODE("HDCL");
static const OptionID	kOptionHalfDuplexNumLock      = OPTION_CODE("HDNL");
static const OptionID	kOptionHalfDuplexScrollLock   = OPTION_CODE("HDSL");
static const OptionID	kOptionModifierMapForShift    = OPTION_CODE("MMFS");
static const OptionID	kOptionModifierMapForControl  = OPTION_CODE("MMFC");
static const OptionID	kOptionModifierMapForAlt      = OPTION_CODE("MMFA");
static const OptionID	kOptionModifierMapForMeta     = OPTION_CODE("MMFM");
static const OptionID	kOptionModifierMapForSuper    = OPTION_CODE("MMFR");
static const OptionID	kOptionHeartbeat              = OPTION_CODE("HART");
static const OptionID	kOptionScreenSwitchCorners    = OPTION_CODE("SSCM");
static const OptionID	kOptionScreenSwitchCornerSize = OPTION_CODE("SSCS");
static const OptionID	kOptionScreenSwitchDelay      = OPTION_CODE("SSWT");
static const OptionID	kOptionScreenSwitchTwoTap     = OPTION_CODE("SSTT");
static const OptionID	kOptionScreenSaverSync        = OPTION_CODE("SSVR");
static const OptionID	kOptionXTestXineramaUnaware   = OPTION_CODE("XTXU");
static const OptionID	kOptionScreenPreserveFocus    = OPTION_CODE("SFOC");
static const OptionID	kOptionRelativeMouseMoves     = OPTION_CODE("MDLT");
static const OptionID	kOptionWin32KeepForeground    = OPTION_CODE("_KFW");
//@}

//! @name Screen switch corner enumeration
//@{
enum EScreenSwitchCorners {
	kNoCorner,
	kTopLeft,
	kTopRight,
	kBottomLeft,
	kBottomRight,
	kFirstCorner = kTopLeft,
	kLastCorner  = kBottomRight
};
//@}

//! @name Screen switch corner masks
//@{
enum EScreenSwitchCornerMasks {
	kNoCornerMask    = 0,
	kTopLeftMask     = 1 << (kTopLeft - kFirstCorner),
	kTopRightMask    = 1 << (kTopRight - kFirstCorner),
	kBottomLeftMask  = 1 << (kBottomLeft - kFirstCorner),
	kBottomRightMask = 1 << (kBottomRight - kFirstCorner),
	kAllCornersMask  = kTopLeftMask | kTopRightMask |
						kBottomLeftMask | kBottomRightMask
};
//@}

#undef OPTION_CODE

#endif
