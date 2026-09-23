/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <stdint.h>

//! Modifier key ID
/*!
Type to hold the id of a key modifier (e.g. a shift key).
*/
#if __APPLE__
typedef uint32_t KeyModifierID;
#else
using KeyModifierID = uint32_t;
#endif

//! @name Modifier key identifiers
//@{
static const KeyModifierID kKeyModifierIDNull = 0;
static const KeyModifierID kKeyModifierIDShift = 1;
static const KeyModifierID kKeyModifierIDControl = 2;
static const KeyModifierID kKeyModifierIDAlt = 3;
static const KeyModifierID kKeyModifierIDMeta = 4;
static const KeyModifierID kKeyModifierIDSuper = 5;
static const KeyModifierID kKeyModifierIDAltGr = 6;
static const KeyModifierID kKeyModifierIDLast = 7;
//@}
