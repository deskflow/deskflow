#ifndef KEYTYPES_H
#define KEYTYPES_H

#include "BasicTypes.h"

// type to hold a key identifier
typedef UInt32			KeyID;

// type to hold bitmask of key modifiers (e.g. shift keys)
typedef UInt32			KeyModifierMask;

// key codes
static const KeyID		kKeyNone = 0;

// modifier key bitmasks
static const KeyModifierMask	KeyModifierShift      = 0x0001;
static const KeyModifierMask	KeyModifierControl    = 0x0002;
static const KeyModifierMask	KeyModifierAlt        = 0x0004;
static const KeyModifierMask	KeyModifierMeta       = 0x0008;
static const KeyModifierMask	KeyModifierCapsLock   = 0x1000;
static const KeyModifierMask	KeyModifierNumLock    = 0x2000;
static const KeyModifierMask	KeyModifierScrollLock = 0x4000;

#endif
