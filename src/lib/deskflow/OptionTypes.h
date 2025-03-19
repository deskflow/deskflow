/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "base/EventTypes.h"
#include "common/stdvector.h"

//! Option ID
/*!
Type to hold an option identifier.
*/
using OptionID = uint32_t;

//! Option Value
/*!
Type to hold an option value.
*/
using OptionValue = int32_t;

// for now, options are just pairs of integers
using OptionsList = std::vector<uint32_t>;

// macro for packing 4 character strings into 4 byte integers
#define OPTION_CODE(_s)                                                                                                \
  (static_cast<uint32_t>(static_cast<unsigned char>(_s[0]) << 24) |                                                    \
   static_cast<uint32_t>(static_cast<unsigned char>(_s[1]) << 16) |                                                    \
   static_cast<uint32_t>(static_cast<unsigned char>(_s[2]) << 8) |                                                     \
   static_cast<uint32_t>(static_cast<unsigned char>(_s[3])))

//! @name Option identifiers
//@{
static const OptionID kOptionHalfDuplexCapsLock = OPTION_CODE("HDCL");
static const OptionID kOptionHalfDuplexNumLock = OPTION_CODE("HDNL");
static const OptionID kOptionHalfDuplexScrollLock = OPTION_CODE("HDSL");
static const OptionID kOptionModifierMapForShift = OPTION_CODE("MMFS");
static const OptionID kOptionModifierMapForControl = OPTION_CODE("MMFC");
static const OptionID kOptionModifierMapForAlt = OPTION_CODE("MMFA");
static const OptionID kOptionModifierMapForAltGr = OPTION_CODE("MMFG");
static const OptionID kOptionModifierMapForMeta = OPTION_CODE("MMFM");
static const OptionID kOptionModifierMapForSuper = OPTION_CODE("MMFR");
static const OptionID kOptionHeartbeat = OPTION_CODE("HART");
static const OptionID kOptionProtocol = OPTION_CODE("PROT");
static const OptionID kOptionScreenSwitchCorners = OPTION_CODE("SSCM");
static const OptionID kOptionScreenSwitchCornerSize = OPTION_CODE("SSCS");
static const OptionID kOptionScreenSwitchDelay = OPTION_CODE("SSWT");
static const OptionID kOptionScreenSwitchTwoTap = OPTION_CODE("SSTT");
static const OptionID kOptionScreenSwitchNeedsShift = OPTION_CODE("SSNS");
static const OptionID kOptionScreenSwitchNeedsControl = OPTION_CODE("SSNC");
static const OptionID kOptionScreenSwitchNeedsAlt = OPTION_CODE("SSNA");
static const OptionID kOptionXTestXineramaUnaware = OPTION_CODE("XTXU");
static const OptionID kOptionScreenPreserveFocus = OPTION_CODE("SFOC");
static const OptionID kOptionRelativeMouseMoves = OPTION_CODE("MDLT");
static const OptionID kOptionWin32KeepForeground = OPTION_CODE("_KFW");
static const OptionID kOptionDisableLockToScreen = OPTION_CODE("DLTS");
static const OptionID kOptionClipboardSharing = OPTION_CODE("CLPS");
static const OptionID kOptionClipboardSharingSize = OPTION_CODE("CLSZ");
//@}

//! @name Screen switch corner enumeration
//@{
enum EScreenSwitchCorners
{
  kNoCorner,
  kTopLeft,
  kTopRight,
  kBottomLeft,
  kBottomRight,
  kFirstCorner = kTopLeft,
  kLastCorner = kBottomRight
};
//@}

//! @name Screen switch corner masks
//@{
enum EScreenSwitchCornerMasks
{
  kNoCornerMask = 0,
  kTopLeftMask = 1 << (kTopLeft - kFirstCorner),
  kTopRightMask = 1 << (kTopRight - kFirstCorner),
  kBottomLeftMask = 1 << (kBottomLeft - kFirstCorner),
  kBottomRightMask = 1 << (kBottomRight - kFirstCorner),
  kAllCornersMask = kTopLeftMask | kTopRightMask | kBottomLeftMask | kBottomRightMask
};
//@}

//! @name Network protocol
//@{
enum class ENetworkProtocol
{
  kSynergy,
  kBarrier
};
//@}

#undef OPTION_CODE
