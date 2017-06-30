/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2002 Chris Schoeneman
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
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

#include "common/basic_types.h"

//! Key ID
/*!
Type to hold a key symbol identifier.  The encoding is UTF-32, using
U+E000 through U+EFFF for the various control keys (e.g. arrow
keys, function keys, modifier keys, etc).
*/
typedef UInt32 KeyID;

//! Key Code
/*!
Type to hold a physical key identifier.  That is, it identifies a
physical key on the keyboard.  KeyButton 0 is reserved to be an
invalid key;  platforms that use 0 as a physical key identifier
will have to remap that value to some arbitrary unused id.
*/
typedef UInt16 KeyButton;

//! Modifier key mask
/*!
Type to hold a bitmask of key modifiers (e.g. shift keys).
*/
typedef UInt32 KeyModifierMask;

//! Modifier key ID
/*!
Type to hold the id of a key modifier (e.g. a shift key).
*/
typedef UInt32 KeyModifierID;

//! @name Modifier key masks
//@{
static const KeyModifierMask KeyModifierShift      = 0x0001;
static const KeyModifierMask KeyModifierControl    = 0x0002;
static const KeyModifierMask KeyModifierAlt        = 0x0004;
static const KeyModifierMask KeyModifierMeta       = 0x0008;
static const KeyModifierMask KeyModifierSuper      = 0x0010;
static const KeyModifierMask KeyModifierAltGr      = 0x0020;
static const KeyModifierMask KeyModifierLevel5Lock = 0x0040;
static const KeyModifierMask KeyModifierCapsLock   = 0x1000;
static const KeyModifierMask KeyModifierNumLock    = 0x2000;
static const KeyModifierMask KeyModifierScrollLock = 0x4000;
//@}

//! @name Modifier key bits
//@{
static const UInt32 kKeyModifierBitNone       = 16;
static const UInt32 kKeyModifierBitShift      = 0;
static const UInt32 kKeyModifierBitControl    = 1;
static const UInt32 kKeyModifierBitAlt        = 2;
static const UInt32 kKeyModifierBitMeta       = 3;
static const UInt32 kKeyModifierBitSuper      = 4;
static const UInt32 kKeyModifierBitAltGr      = 5;
static const UInt32 kKeyModifierBitLevel5Lock = 6;
static const UInt32 kKeyModifierBitCapsLock   = 12;
static const UInt32 kKeyModifierBitNumLock    = 13;
static const UInt32 kKeyModifierBitScrollLock = 14;
static const SInt32 kKeyModifierNumBits       = 16;
//@}

//! @name Modifier key identifiers
//@{
static const KeyModifierID kKeyModifierIDNull    = 0;
static const KeyModifierID kKeyModifierIDShift   = 1;
static const KeyModifierID kKeyModifierIDControl = 2;
static const KeyModifierID kKeyModifierIDAlt     = 3;
static const KeyModifierID kKeyModifierIDMeta    = 4;
static const KeyModifierID kKeyModifierIDSuper   = 5;
static const KeyModifierID kKeyModifierIDAltGr   = 6;
static const KeyModifierID kKeyModifierIDLast    = 7;
//@}

//! @name Key identifiers
//@{
// all identifiers except kKeyNone and those in 0xE000 to 0xE0FF
// inclusive are equal to the corresponding X11 keysym - 0x1000.

// no key
static const KeyID kKeyNone = 0x0000;

// TTY functions
static const KeyID kKeyBackSpace        = 0xEF08; /* back space, back char */
static const KeyID kKeyTab              = 0xEF09;
static const KeyID kKeyLinefeed         = 0xEF0A; /* Linefeed, LF */
static const KeyID kKeyClear            = 0xEF0B;
static const KeyID kKeyReturn           = 0xEF0D; /* Return, enter */
static const KeyID kKeyPause            = 0xEF13; /* Pause, hold */
static const KeyID kKeyScrollLock       = 0xEF14;
static const KeyID kKeySysReq           = 0xEF15;
static const KeyID kKeyEscape           = 0xEF1B;
static const KeyID kKeyHenkan           = 0xEF23; /* Start/Stop Conversion */
static const KeyID kKeyKana             = 0xEF26; /* Kana */
static const KeyID kKeyHiraganaKatakana = 0xEF27; /* Hiragana/Katakana toggle */
static const KeyID kKeyZenkaku          = 0xEF2A; /* Zenkaku/Hankaku */
static const KeyID kKeyKanzi            = 0xEF2A; /* Kanzi */
static const KeyID kKeyHangul           = 0xEF31; /* Hangul */
static const KeyID kKeyHanja            = 0xEF34; /* Hanja */
static const KeyID kKeyDelete           = 0xEFFF; /* Delete, rubout */

// cursor control
static const KeyID kKeyHome     = 0xEF50;
static const KeyID kKeyLeft     = 0xEF51; /* Move left, left arrow */
static const KeyID kKeyUp       = 0xEF52; /* Move up, up arrow */
static const KeyID kKeyRight    = 0xEF53; /* Move right, right arrow */
static const KeyID kKeyDown     = 0xEF54; /* Move down, down arrow */
static const KeyID kKeyPageUp   = 0xEF55;
static const KeyID kKeyPageDown = 0xEF56;
static const KeyID kKeyEnd      = 0xEF57; /* EOL */
static const KeyID kKeyBegin    = 0xEF58; /* BOL */

// misc functions
static const KeyID kKeySelect  = 0xEF60; /* Select, mark */
static const KeyID kKeyPrint   = 0xEF61;
static const KeyID kKeyExecute = 0xEF62; /* Execute, run, do */
static const KeyID kKeyInsert  = 0xEF63; /* Insert, insert here */
static const KeyID kKeyUndo    = 0xEF65; /* Undo, oops */
static const KeyID kKeyRedo    = 0xEF66; /* redo, again */
static const KeyID kKeyMenu    = 0xEF67;
static const KeyID kKeyFind    = 0xEF68; /* Find, search */
static const KeyID kKeyCancel  = 0xEF69; /* Cancel, stop, abort, exit */
static const KeyID kKeyHelp    = 0xEF6A; /* Help */
static const KeyID kKeyBreak   = 0xEF6B;
static const KeyID kKeyAltGr   = 0xEF7E; /* Character set switch */
static const KeyID kKeyNumLock = 0xEF7F;

// keypad
static const KeyID kKeyKP_Space     = 0xEF80; /* space */
static const KeyID kKeyKP_Tab       = 0xEF89;
static const KeyID kKeyKP_Enter     = 0xEF8D; /* enter */
static const KeyID kKeyKP_F1        = 0xEF91; /* PF1, KP_A, ... */
static const KeyID kKeyKP_F2        = 0xEF92;
static const KeyID kKeyKP_F3        = 0xEF93;
static const KeyID kKeyKP_F4        = 0xEF94;
static const KeyID kKeyKP_Home      = 0xEF95;
static const KeyID kKeyKP_Left      = 0xEF96;
static const KeyID kKeyKP_Up        = 0xEF97;
static const KeyID kKeyKP_Right     = 0xEF98;
static const KeyID kKeyKP_Down      = 0xEF99;
static const KeyID kKeyKP_PageUp    = 0xEF9A;
static const KeyID kKeyKP_PageDown  = 0xEF9B;
static const KeyID kKeyKP_End       = 0xEF9C;
static const KeyID kKeyKP_Begin     = 0xEF9D;
static const KeyID kKeyKP_Insert    = 0xEF9E;
static const KeyID kKeyKP_Delete    = 0xEF9F;
static const KeyID kKeyKP_Equal     = 0xEFBD; /* equals */
static const KeyID kKeyKP_Multiply  = 0xEFAA;
static const KeyID kKeyKP_Add       = 0xEFAB;
static const KeyID kKeyKP_Separator = 0xEFAC; /* separator, often comma */
static const KeyID kKeyKP_Subtract  = 0xEFAD;
static const KeyID kKeyKP_Decimal   = 0xEFAE;
static const KeyID kKeyKP_Divide    = 0xEFAF;
static const KeyID kKeyKP_0         = 0xEFB0;
static const KeyID kKeyKP_1         = 0xEFB1;
static const KeyID kKeyKP_2         = 0xEFB2;
static const KeyID kKeyKP_3         = 0xEFB3;
static const KeyID kKeyKP_4         = 0xEFB4;
static const KeyID kKeyKP_5         = 0xEFB5;
static const KeyID kKeyKP_6         = 0xEFB6;
static const KeyID kKeyKP_7         = 0xEFB7;
static const KeyID kKeyKP_8         = 0xEFB8;
static const KeyID kKeyKP_9         = 0xEFB9;

// function keys
static const KeyID kKeyF1  = 0xEFBE;
static const KeyID kKeyF2  = 0xEFBF;
static const KeyID kKeyF3  = 0xEFC0;
static const KeyID kKeyF4  = 0xEFC1;
static const KeyID kKeyF5  = 0xEFC2;
static const KeyID kKeyF6  = 0xEFC3;
static const KeyID kKeyF7  = 0xEFC4;
static const KeyID kKeyF8  = 0xEFC5;
static const KeyID kKeyF9  = 0xEFC6;
static const KeyID kKeyF10 = 0xEFC7;
static const KeyID kKeyF11 = 0xEFC8;
static const KeyID kKeyF12 = 0xEFC9;
static const KeyID kKeyF13 = 0xEFCA;
static const KeyID kKeyF14 = 0xEFCB;
static const KeyID kKeyF15 = 0xEFCC;
static const KeyID kKeyF16 = 0xEFCD;
static const KeyID kKeyF17 = 0xEFCE;
static const KeyID kKeyF18 = 0xEFCF;
static const KeyID kKeyF19 = 0xEFD0;
static const KeyID kKeyF20 = 0xEFD1;
static const KeyID kKeyF21 = 0xEFD2;
static const KeyID kKeyF22 = 0xEFD3;
static const KeyID kKeyF23 = 0xEFD4;
static const KeyID kKeyF24 = 0xEFD5;
static const KeyID kKeyF25 = 0xEFD6;
static const KeyID kKeyF26 = 0xEFD7;
static const KeyID kKeyF27 = 0xEFD8;
static const KeyID kKeyF28 = 0xEFD9;
static const KeyID kKeyF29 = 0xEFDA;
static const KeyID kKeyF30 = 0xEFDB;
static const KeyID kKeyF31 = 0xEFDC;
static const KeyID kKeyF32 = 0xEFDD;
static const KeyID kKeyF33 = 0xEFDE;
static const KeyID kKeyF34 = 0xEFDF;
static const KeyID kKeyF35 = 0xEFE0;

// modifiers
static const KeyID kKeyShift_L   = 0xEFE1; /* Left shift */
static const KeyID kKeyShift_R   = 0xEFE2; /* Right shift */
static const KeyID kKeyControl_L = 0xEFE3; /* Left control */
static const KeyID kKeyControl_R = 0xEFE4; /* Right control */
static const KeyID kKeyCapsLock  = 0xEFE5; /* Caps lock */
static const KeyID kKeyShiftLock = 0xEFE6; /* Shift lock */
static const KeyID kKeyMeta_L    = 0xEFE7; /* Left meta */
static const KeyID kKeyMeta_R    = 0xEFE8; /* Right meta */
static const KeyID kKeyAlt_L     = 0xEFE9; /* Left alt */
static const KeyID kKeyAlt_R     = 0xEFEA; /* Right alt */
static const KeyID kKeySuper_L   = 0xEFEB; /* Left super */
static const KeyID kKeySuper_R   = 0xEFEC; /* Right super */
static const KeyID kKeyHyper_L   = 0xEFED; /* Left hyper */
static const KeyID kKeyHyper_R   = 0xEFEE; /* Right hyper */

// multi-key character composition
static const KeyID kKeyCompose         = 0xEF20;
static const KeyID kKeyDeadGrave       = 0x0300;
static const KeyID kKeyDeadAcute       = 0x0301;
static const KeyID kKeyDeadCircumflex  = 0x0302;
static const KeyID kKeyDeadTilde       = 0x0303;
static const KeyID kKeyDeadMacron      = 0x0304;
static const KeyID kKeyDeadBreve       = 0x0306;
static const KeyID kKeyDeadAbovedot    = 0x0307;
static const KeyID kKeyDeadDiaeresis   = 0x0308;
static const KeyID kKeyDeadAbovering   = 0x030a;
static const KeyID kKeyDeadDoubleacute = 0x030b;
static const KeyID kKeyDeadCaron       = 0x030c;
static const KeyID kKeyDeadCedilla     = 0x0327;
static const KeyID kKeyDeadOgonek      = 0x0328;

// more function and modifier keys
static const KeyID kKeyLeftTab = 0xEE20;

// update modifiers
static const KeyID kKeySetModifiers   = 0xEE06;
static const KeyID kKeyClearModifiers = 0xEE07;

// group change
static const KeyID kKeyNextGroup = 0xEE08;
static const KeyID kKeyPrevGroup = 0xEE0A;

// extended keys
static const KeyID kKeyEject          = 0xE001;
static const KeyID kKeySleep          = 0xE05F;
static const KeyID kKeyWWWBack        = 0xE0A6;
static const KeyID kKeyWWWForward     = 0xE0A7;
static const KeyID kKeyWWWRefresh     = 0xE0A8;
static const KeyID kKeyWWWStop        = 0xE0A9;
static const KeyID kKeyWWWSearch      = 0xE0AA;
static const KeyID kKeyWWWFavorites   = 0xE0AB;
static const KeyID kKeyWWWHome        = 0xE0AC;
static const KeyID kKeyAudioMute      = 0xE0AD;
static const KeyID kKeyAudioDown      = 0xE0AE;
static const KeyID kKeyAudioUp        = 0xE0AF;
static const KeyID kKeyAudioNext      = 0xE0B0;
static const KeyID kKeyAudioPrev      = 0xE0B1;
static const KeyID kKeyAudioStop      = 0xE0B2;
static const KeyID kKeyAudioPlay      = 0xE0B3;
static const KeyID kKeyAppMail        = 0xE0B4;
static const KeyID kKeyAppMedia       = 0xE0B5;
static const KeyID kKeyAppUser1       = 0xE0B6;
static const KeyID kKeyAppUser2       = 0xE0B7;
static const KeyID kKeyBrightnessDown = 0xE0B8;
static const KeyID kKeyBrightnessUp   = 0xE0B9;
static const KeyID kKeyMissionControl = 0xE0C0;
static const KeyID kKeyLaunchpad      = 0xE0C1;

//@}

struct KeyNameMapEntry {
    const char* m_name;
    KeyID m_id;
};
struct KeyModifierNameMapEntry {
    const char* m_name;
    KeyModifierMask m_mask;
};

//! Key name to KeyID table
/*!
A table of key names to the corresponding KeyID.  Only the keys listed
above plus non-alphanumeric ASCII characters are in the table.  The end
of the table is the first pair with a NULL m_name.
*/
extern const struct KeyNameMapEntry kKeyNameMap[];

//! Modifier key name to KeyModifierMask table
/*!
A table of modifier key names to the corresponding KeyModifierMask.
The end of the table is the first pair with a NULL m_name.
*/
extern const struct KeyModifierNameMapEntry kModifierNameMap[];
