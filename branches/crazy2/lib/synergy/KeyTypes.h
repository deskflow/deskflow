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

#ifndef KEYTYPES_H
#define KEYTYPES_H

#include "BasicTypes.h"

//! Key ID
/*!
Type to hold a key symbol identifier.  The encoding is UTF-32, using
U+E000 through U+EFFF for the various control keys (e.g. arrow
keys, function keys, modifier keys, etc).
*/
typedef UInt32			KeyID;

//! Key Code
/*!
Type to hold a physical key identifier.  That is, it identifies a
physical key on the keyboard.  KeyButton 0 is reserved to be an
invalid key;  platforms that use 0 as a physical key identifier
will have to remap that value to some arbitrary unused id.
*/
typedef UInt16			KeyButton;

//! Modifier key mask
/*!
Type to hold a bitmask of key modifiers (e.g. shift keys).
*/
typedef UInt32			KeyModifierMask;

//! Modifier key ID
/*!
Type to hold the id of a key modifier (e.g. a shift key).
*/
typedef UInt32			KeyModifierID;

//! @name Modifier key masks
//@{
static const KeyModifierMask	KeyModifierShift      = 0x0001;
static const KeyModifierMask	KeyModifierControl    = 0x0002;
static const KeyModifierMask	KeyModifierAlt        = 0x0004;
static const KeyModifierMask	KeyModifierMeta       = 0x0008;
static const KeyModifierMask	KeyModifierSuper      = 0x0010;
static const KeyModifierMask	KeyModifierAltGr      = 0x0020;
static const KeyModifierMask	KeyModifierCapsLock   = 0x1000;
static const KeyModifierMask	KeyModifierNumLock    = 0x2000;
static const KeyModifierMask	KeyModifierScrollLock = 0x4000;
//@}

//! @name Modifier key bits
//@{
static const UInt32				kKeyModifierBitNone       = 16;
static const UInt32				kKeyModifierBitShift      = 0;
static const UInt32				kKeyModifierBitControl    = 1;
static const UInt32				kKeyModifierBitAlt        = 2;
static const UInt32				kKeyModifierBitMeta       = 3;
static const UInt32				kKeyModifierBitSuper      = 4;
static const UInt32				kKeyModifierBitAltGr      = 5;
static const UInt32				kKeyModifierBitCapsLock   = 12;
static const UInt32				kKeyModifierBitNumLock    = 13;
static const UInt32				kKeyModifierBitScrollLock = 14;
static const SInt32				kKeyModifierNumBits       = 16;
//@}

//! @name Modifier key identifiers
//@{
static const KeyModifierID		kKeyModifierIDNull     = 0;
static const KeyModifierID		kKeyModifierIDShift    = 1;
static const KeyModifierID		kKeyModifierIDControl  = 2;
static const KeyModifierID		kKeyModifierIDAlt      = 3;
static const KeyModifierID		kKeyModifierIDMeta     = 4;
static const KeyModifierID		kKeyModifierIDSuper    = 5;
static const KeyModifierID		kKeyModifierIDLast     = 6;
//@}

//! @name Key identifiers - now using USB HID codes
//@{

// no key
static const KeyID		kKeyNone		= 0x00;
static const KeyID		kKeyCapsLock	= 0x39;
static const KeyID		kKeyScrollLock	= 0x47;
static const KeyID		kKeyDelete		= 0x4C;
static const KeyID		kKeyNumLock		= 0x53;
static const KeyID		kKeyControl_L	= 0xE0;
static const KeyID		kKeyShift_L		= 0xE1;
static const KeyID		kKeyAlt_L		= 0xE2;
static const KeyID		kKeySuper_L		= 0xE3; // Left GUI / Super / Mac
static const KeyID		kKeyControl_R	= 0xE4;
static const KeyID		kKeyShift_R		= 0xE5;
static const KeyID		kKeyAlt_R		= 0xE6;
static const KeyID		kKeySuper_R		= 0xE7;

static const KeyID		kKeyMeta_L		= kKeySuper_L;
static const KeyID		kKeyMeta_R		= kKeySuper_R;

static const KeyID		kKeySetModifiers	= 0xFFFF;
static const KeyID		kKeyClearModifiers	= 0xFFFE;

//@}

struct KeyNameMapEntry {
public:
	const char*			m_name;
	KeyID			 	m_id;
};
struct KeyModifierNameMapEntry {
public:
	const char*			m_name;
	KeyModifierMask 	m_mask;
};

//! Key name to KeyID table
/*!
A table of key names to the corresponding KeyID.  The end
of the table is the first pair with a NULL m_name.
*/
extern const KeyNameMapEntry kKeyNameMap[];

//! Modifier key name to KeyModifierMask table
/*!
A table of modifier key names to the corresponding KeyModifierMask.
The end of the table is the first pair with a NULL m_name.
*/
extern const KeyModifierNameMapEntry kModifierNameMap[];

#endif
