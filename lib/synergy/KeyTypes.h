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
Type to hold a key identifier.  The encoding is UTF-32, using
U+E000 through U+EFFF for the various control keys (e.g. arrow
keys, function keys, modifier keys, etc).
*/
typedef UInt32			KeyID;

//! Modifier key ID
/*!
Type to hold a bitmask of key modifiers (e.g. shift keys).
*/
typedef UInt32			KeyModifierMask;

//! @name Modifier key identifiers
//@{
static const KeyModifierMask	KeyModifierShift      = 0x0001;
static const KeyModifierMask	KeyModifierControl    = 0x0002;
static const KeyModifierMask	KeyModifierAlt        = 0x0004;
static const KeyModifierMask	KeyModifierMeta       = 0x0008;
static const KeyModifierMask	KeyModifierCapsLock   = 0x1000;
static const KeyModifierMask	KeyModifierNumLock    = 0x2000;
static const KeyModifierMask	KeyModifierScrollLock = 0x4000;
//@}

//! @name Key identifiers
//@{
// all identifiers except kKeyNone are equal to the corresponding
// X11 keysym - 0x1000.

// no key
static const KeyID		kKeyNone		= 0x0000;

// TTY functions
static const KeyID		kKeyBackSpace	= 0xEF08;	/* back space, back char */
static const KeyID		kKeyTab			= 0xEF09;
static const KeyID		kKeyLinefeed	= 0xEF0A;	/* Linefeed, LF */
static const KeyID		kKeyClear		= 0xEF0B;
static const KeyID		kKeyReturn		= 0xEF0D;	/* Return, enter */
static const KeyID		kKeyPause		= 0xEF13;	/* Pause, hold */
static const KeyID		kKeyScrollLock	= 0xEF14;
static const KeyID		kKeySysReq		= 0xEF15;
static const KeyID		kKeyEscape		= 0xEF1B;
static const KeyID		kKeyDelete		= 0xEFFF;	/* Delete, rubout */

// multi-key character composition
static const KeyID		kKeyMultiKey	= 0xEF20;	/* Multi-key character compose */

// cursor control
static const KeyID		kKeyHome		= 0xEF50;
static const KeyID		kKeyLeft		= 0xEF51;	/* Move left, left arrow */
static const KeyID		kKeyUp			= 0xEF52;	/* Move up, up arrow */
static const KeyID		kKeyRight		= 0xEF53;	/* Move right, right arrow */
static const KeyID		kKeyDown		= 0xEF54;	/* Move down, down arrow */
static const KeyID		kKeyPageUp		= 0xEF55;
static const KeyID		kKeyPageDown	= 0xEF56;
static const KeyID		kKeyEnd			= 0xEF57;	/* EOL */
static const KeyID		kKeyBegin		= 0xEF58;	/* BOL */

// misc functions
static const KeyID		kKeySelect		= 0xEF60;	/* Select, mark */
static const KeyID		kKeyPrint		= 0xEF61;
static const KeyID		kKeyExecute		= 0xEF62;	/* Execute, run, do */
static const KeyID		kKeyInsert		= 0xEF63;	/* Insert, insert here */
static const KeyID		kKeyUndo		= 0xEF65;	/* Undo, oops */
static const KeyID		kKeyRedo		= 0xEF66;	/* redo, again */
static const KeyID		kKeyMenu		= 0xEF67;
static const KeyID		kKeyFind		= 0xEF68;	/* Find, search */
static const KeyID		kKeyCancel		= 0xEF69;	/* Cancel, stop, abort, exit */
static const KeyID		kKeyHelp		= 0xEF6A;	/* Help */
static const KeyID		kKeyBreak		= 0xEF6B;
static const KeyID		kKeyModeSwitch	= 0xEF7E;	/* Character set switch */
static const KeyID		kKeyNumLock		= 0xEF7F;

// keypad
static const KeyID		kKeyKP_Space	= 0xEF80;	/* space */
static const KeyID		kKeyKP_Tab		= 0xEF89;
static const KeyID		kKeyKP_Enter	= 0xEF8D;	/* enter */
static const KeyID		kKeyKP_F1		= 0xEF91;	/* PF1, KP_A, ... */
static const KeyID		kKeyKP_F2		= 0xEF92;
static const KeyID		kKeyKP_F3		= 0xEF93;
static const KeyID		kKeyKP_F4		= 0xEF94;
static const KeyID		kKeyKP_Home		= 0xEF95;
static const KeyID		kKeyKP_Left		= 0xEF96;
static const KeyID		kKeyKP_Up		= 0xEF97;
static const KeyID		kKeyKP_Right	= 0xEF98;
static const KeyID		kKeyKP_Down		= 0xEF99;
static const KeyID		kKeyKP_Prior	= 0xEF9A;
static const KeyID		kKeyKP_PageUp	= 0xEF9A;
static const KeyID		kKeyKP_Next		= 0xEF9B;
static const KeyID		kKeyKP_PageDown	= 0xEF9B;
static const KeyID		kKeyKP_End		= 0xEF9C;
static const KeyID		kKeyKP_Begin	= 0xEF9D;
static const KeyID		kKeyKP_Insert	= 0xEF9E;
static const KeyID		kKeyKP_Delete	= 0xEF9F;
static const KeyID		kKeyKP_Equal	= 0xEFBD;	/* equals */
static const KeyID		kKeyKP_Multiply	= 0xEFAA;
static const KeyID		kKeyKP_Add		= 0xEFAB;
static const KeyID		kKeyKP_Separator= 0xEFAC;	/* separator, often comma */
static const KeyID		kKeyKP_Subtract	= 0xEFAD;
static const KeyID		kKeyKP_Decimal	= 0xEFAE;
static const KeyID		kKeyKP_Divide	= 0xEFAF;
static const KeyID		kKeyKP_0		= 0xEFB0; 
static const KeyID		kKeyKP_1		= 0xEFB1;
static const KeyID		kKeyKP_2		= 0xEFB2;
static const KeyID		kKeyKP_3		= 0xEFB3;
static const KeyID		kKeyKP_4		= 0xEFB4;
static const KeyID		kKeyKP_5		= 0xEFB5;
static const KeyID		kKeyKP_6		= 0xEFB6;
static const KeyID		kKeyKP_7		= 0xEFB7;
static const KeyID		kKeyKP_8		= 0xEFB8;
static const KeyID		kKeyKP_9		= 0xEFB9;

// function keys
static const KeyID		kKeyF1			= 0xEFBE;
static const KeyID		kKeyF2			= 0xEFBF;
static const KeyID		kKeyF3			= 0xEFC0;
static const KeyID		kKeyF4			= 0xEFC1;
static const KeyID		kKeyF5			= 0xEFC2;
static const KeyID		kKeyF6			= 0xEFC3;
static const KeyID		kKeyF7			= 0xEFC4;
static const KeyID		kKeyF8			= 0xEFC5;
static const KeyID		kKeyF9			= 0xEFC6;
static const KeyID		kKeyF10			= 0xEFC7;
static const KeyID		kKeyF11			= 0xEFC8;
static const KeyID		kKeyF12			= 0xEFC9;
static const KeyID		kKeyF13			= 0xEFCA;
static const KeyID		kKeyF14			= 0xEFCB;
static const KeyID		kKeyF15			= 0xEFCC;
static const KeyID		kKeyF16			= 0xEFCD;
static const KeyID		kKeyF17			= 0xEFCE;
static const KeyID		kKeyF18			= 0xEFCF;
static const KeyID		kKeyF19			= 0xEFD0;
static const KeyID		kKeyF20			= 0xEFD1;
static const KeyID		kKeyF21			= 0xEFD2;
static const KeyID		kKeyF22			= 0xEFD3;
static const KeyID		kKeyF23			= 0xEFD4;
static const KeyID		kKeyF24			= 0xEFD5;
static const KeyID		kKeyF25			= 0xEFD6;
static const KeyID		kKeyF26			= 0xEFD7;
static const KeyID		kKeyF27			= 0xEFD8;
static const KeyID		kKeyF28			= 0xEFD9;
static const KeyID		kKeyF29			= 0xEFDA;
static const KeyID		kKeyF30			= 0xEFDB;
static const KeyID		kKeyF31			= 0xEFDC;
static const KeyID		kKeyF32			= 0xEFDD;
static const KeyID		kKeyF33			= 0xEFDE;
static const KeyID		kKeyF34			= 0xEFDF;
static const KeyID		kKeyF35			= 0xEFE0;

// modifiers
static const KeyID		kKeyShift_L		= 0xEFE1;	/* Left shift */
static const KeyID		kKeyShift_R		= 0xEFE2;	/* Right shift */
static const KeyID		kKeyControl_L	= 0xEFE3;	/* Left control */
static const KeyID		kKeyControl_R	= 0xEFE4;	/* Right control */
static const KeyID		kKeyCapsLock	= 0xEFE5;	/* Caps lock */
static const KeyID		kKeyShiftLock	= 0xEFE6;	/* Shift lock */
static const KeyID		kKeyMeta_L		= 0xEFE7;	/* Left meta */
static const KeyID		kKeyMeta_R		= 0xEFE8;	/* Right meta */
static const KeyID		kKeyAlt_L		= 0xEFE9;	/* Left alt */
static const KeyID		kKeyAlt_R		= 0xEFEA;	/* Right alt */
static const KeyID		kKeySuper_L		= 0xEFEB;	/* Left super */
static const KeyID		kKeySuper_R		= 0xEFEC;	/* Right super */
static const KeyID		kKeyHyper_L		= 0xEFED;	/* Left hyper */
static const KeyID		kKeyHyper_R		= 0xEFEE;	/* Right hyper */

// more function and modifier keys
static const KeyID		kKeyLeftTab		= 0xEE20;
//@}

#endif
