#ifndef KEYTYPES_H
#define KEYTYPES_H

// type to hold a key identifier
typedef UInt32			KeyID;

// type to hold bitmask of keys that have toggle states
typedef UInt16			KeyToggleMask;

// toggle key bitmasks
static const UInt32		KeyToggleShiftLock  = 0x0001;
static const UInt32		KeyToggleNumLock    = 0x0002;
static const UInt32		KeyToggleScrollLock = 0x0004;

// key codes
static const KeyID		kKeyNone = 0;

#endif
