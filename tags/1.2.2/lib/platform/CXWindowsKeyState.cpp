/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2003 Chris Schoeneman
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

#include "CXWindowsKeyState.h"
#include "CXWindowsUtil.h"
#include "CLog.h"
#include "CStringUtil.h"
#if X_DISPLAY_MISSING
#	error X11 is required to build synergy
#else
#	include <X11/X.h>
#	include <X11/Xutil.h>
#	define XK_MISCELLANY
#	define XK_XKB_KEYS
#	include <X11/keysymdef.h>
// these should be in XF86keysym.h but there are several versions of
// that file floating around and not all have all symbols and none of
// them provide any form of versioning so we just define 'em here.
#define XF86XK_Standby		0x1008FF10
#define XF86XK_AudioLowerVolume	0x1008FF11
#define XF86XK_AudioMute	0x1008FF12
#define XF86XK_AudioRaiseVolume	0x1008FF13
#define XF86XK_AudioPlay	0x1008FF14
#define XF86XK_AudioStop	0x1008FF15
#define XF86XK_AudioPrev	0x1008FF16
#define XF86XK_AudioNext	0x1008FF17
#define XF86XK_HomePage		0x1008FF18
#define XF86XK_Mail		0x1008FF19
#define XF86XK_Start		0x1008FF1A
#define XF86XK_Search		0x1008FF1B
#define XF86XK_AudioRecord	0x1008FF1C
#define XF86XK_Calculator	0x1008FF1D
#define XF86XK_Memo		0x1008FF1E
#define XF86XK_ToDoList		0x1008FF1F
#define XF86XK_Calendar		0x1008FF20
#define XF86XK_PowerDown	0x1008FF21
#define XF86XK_ContrastAdjust	0x1008FF22
#define XF86XK_RockerUp		0x1008FF23
#define XF86XK_RockerDown	0x1008FF24
#define XF86XK_RockerEnter	0x1008FF25
#define XF86XK_Back		0x1008FF26
#define XF86XK_Forward		0x1008FF27
#define XF86XK_Stop		0x1008FF28
#define XF86XK_Refresh		0x1008FF29
#define XF86XK_PowerOff		0x1008FF2A
#define XF86XK_WakeUp		0x1008FF2B
#define XF86XK_Eject            0x1008FF2C
#define XF86XK_ScreenSaver      0x1008FF2D
#define XF86XK_WWW              0x1008FF2E
#define XF86XK_Sleep            0x1008FF2F
#define XF86XK_Favorites	0x1008FF30
#define XF86XK_AudioPause	0x1008FF31
#define XF86XK_AudioMedia	0x1008FF32
#define XF86XK_MyComputer	0x1008FF33
#define XF86XK_VendorHome	0x1008FF34
#define XF86XK_LightBulb	0x1008FF35
#define XF86XK_Shop		0x1008FF36
#define XF86XK_History		0x1008FF37
#define XF86XK_OpenURL		0x1008FF38
#define XF86XK_AddFavorite	0x1008FF39
#define XF86XK_HotLinks		0x1008FF3A
#define XF86XK_BrightnessAdjust	0x1008FF3B
#define XF86XK_Finance		0x1008FF3C
#define XF86XK_Community	0x1008FF3D
#define XF86XK_Launch0		0x1008FF40
#define XF86XK_Launch1		0x1008FF41
#define XF86XK_Launch2		0x1008FF42
#define XF86XK_Launch3		0x1008FF43
#define XF86XK_Launch4		0x1008FF44
#define XF86XK_Launch5		0x1008FF45
#define XF86XK_Launch6		0x1008FF46
#define XF86XK_Launch7		0x1008FF47
#define XF86XK_Launch8		0x1008FF48
#define XF86XK_Launch9		0x1008FF49
#define XF86XK_LaunchA		0x1008FF4A
#define XF86XK_LaunchB		0x1008FF4B
#define XF86XK_LaunchC		0x1008FF4C
#define XF86XK_LaunchD		0x1008FF4D
#define XF86XK_LaunchE		0x1008FF4E
#define XF86XK_LaunchF		0x1008FF4F
#endif

// map special KeyID keys to KeySyms
#if HAVE_X11_XF86KEYSYM_H
static const KeySym		g_mapE000[] =
{
	/* 0x00 */ 0, XF86XK_Eject, 0, 0, 0, 0, 0, 0,
	/* 0x08 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x10 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x18 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x20 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x28 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x30 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x38 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x40 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x48 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x50 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x58 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x60 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x68 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x70 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x78 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x80 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x88 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x90 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x98 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0xa0 */ 0, 0, 0, 0,
	/* 0xa4 */ 0, 0,
	/* 0xa6 */ XF86XK_Back, XF86XK_Forward,
	/* 0xa8 */ XF86XK_Refresh, XF86XK_Stop,
	/* 0xaa */ XF86XK_Search, XF86XK_Favorites,
	/* 0xac */ XF86XK_HomePage, XF86XK_AudioMute,
	/* 0xae */ XF86XK_AudioLowerVolume, XF86XK_AudioRaiseVolume,
	/* 0xb0 */ XF86XK_AudioNext, XF86XK_AudioPrev,
	/* 0xb2 */ XF86XK_AudioStop, XF86XK_AudioPlay,
	/* 0xb4 */ XF86XK_Mail, XF86XK_AudioMedia,
	/* 0xb6 */ XF86XK_Launch0, XF86XK_Launch1,
	/* 0xb8 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0xc0 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0xc8 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0xd0 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0xd8 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0xe0 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0xe8 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0xf0 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0xf8 */ 0, 0, 0, 0, 0, 0, 0, 0
};
#endif

CXWindowsKeyState::CXWindowsKeyState(Display* display) :
	m_display(display)
{
	// do nothing
}

CXWindowsKeyState::~CXWindowsKeyState()
{
	// do nothing
}

KeyModifierMask
CXWindowsKeyState::mapModifiersFromX(unsigned int state) const
{
	KeyModifierMask mask = 0;
	if (state & ShiftMask)
		mask |= KeyModifierShift;
	if (state & LockMask)
		mask |= KeyModifierCapsLock;
	if (state & ControlMask)
		mask |= KeyModifierControl;
	if (state & m_altMask)
		mask |= KeyModifierAlt;
	if (state & m_metaMask)
		mask |= KeyModifierMeta;
	if (state & m_superMask)
		mask |= KeyModifierSuper;
	if (state & m_modeSwitchMask)
		mask |= KeyModifierModeSwitch;
	if (state & m_numLockMask)
		mask |= KeyModifierNumLock;
	if (state & m_scrollLockMask)
		mask |= KeyModifierScrollLock;
	return mask;
}

bool
CXWindowsKeyState::fakeCtrlAltDel()
{
	// pass keys through unchanged
	return false;
}

const char*
CXWindowsKeyState::getKeyName(KeyButton keycode) const
{
	KeySym keysym = XKeycodeToKeysym(m_display, keycode, 0);
	char* name    = XKeysymToString(keysym);
	if (name != NULL) {
		return name;
	}
	else {
		static char buffer[20];
		return strcpy(buffer,
					CStringUtil::print("keycode %d", keycode).c_str());
	}
}

void
CXWindowsKeyState::doUpdateKeys()
{
	// query which keys are pressed
	char keys[32];
	XQueryKeymap(m_display, keys);

	// save the auto-repeat mask
	XGetKeyboardControl(m_display, &m_keyControl);

	// query the pointer to get the keyboard state
	Window root = DefaultRootWindow(m_display), window;
	int xRoot, yRoot, xWindow, yWindow;
	unsigned int state;
	if (!XQueryPointer(m_display, root, &root, &window,
								&xRoot, &yRoot, &xWindow, &yWindow, &state)) {
		state = 0;
	}

	// update mappings
	updateKeysymMap();
	updateModifiers();

	// transfer to our state
	for (UInt32 i = 0, j = 0; i < 32; j += 8, ++i) {
		if ((keys[i] & 0x01) != 0)
			setKeyDown(j + 0, true);
		if ((keys[i] & 0x02) != 0)
			setKeyDown(j + 1, true);
		if ((keys[i] & 0x04) != 0)
			setKeyDown(j + 2, true);
		if ((keys[i] & 0x08) != 0)
			setKeyDown(j + 3, true);
		if ((keys[i] & 0x10) != 0)
			setKeyDown(j + 4, true);
		if ((keys[i] & 0x20) != 0)
			setKeyDown(j + 5, true);
		if ((keys[i] & 0x40) != 0)
			setKeyDown(j + 6, true);
		if ((keys[i] & 0x80) != 0)
			setKeyDown(j + 7, true);
	}

	// set toggle modifier states
	if ((state & LockMask) != 0)
		setToggled(KeyModifierCapsLock);
	if ((state & m_numLockMask) != 0)
		setToggled(KeyModifierNumLock);
	if ((state & m_scrollLockMask) != 0)
		setToggled(KeyModifierScrollLock);
}

void
CXWindowsKeyState::doFakeKeyEvent(KeyButton keycode, bool press, bool)
{
	XTestFakeKeyEvent(m_display, keycode, press ? True : False, CurrentTime);
	XFlush(m_display);
}

KeyButton
CXWindowsKeyState::mapKey(Keystrokes& keys, KeyID id,
				KeyModifierMask desiredMask, bool isAutoRepeat) const
{
	// the system translates key events into characters depending
	// on the modifier key state at the time of the event.  to
	// generate the right keysym we need to set the modifier key
	// states appropriately.
	//
	// desiredMask is the mask desired by the caller.  however, there
	// may not be a keycode mapping to generate the desired keysym
	// with that mask.  we override the bits in the mask that cannot
	// be accomodated.

	// convert KeyID to a KeySym
	KeySym keysym = keyIDToKeySym(id, desiredMask);
	if (keysym == NoSymbol) {
		// unknown key
		return 0;
	}

	// get the mapping for this keysym
	KeySymIndex keyIndex = m_keysymMap.find(keysym);
	if (keyIndex != m_keysymMap.end()) {
		// the keysym is mapped to some keycode.  create the keystrokes
		// for this keysym.
		return mapToKeystrokes(keys, keyIndex, isAutoRepeat, false);
	}

	// we can't find the keysym mapped to any keycode.  this doesn't
	// necessarily mean we can't generate the keysym, though.  if the
	// keysym can be created by combining keysyms then we may still
	// be okay.
	if (!isAutoRepeat) {
		KeyButton keycode = mapDecompositionToKeystrokes(keys, keysym, true);
		if (keycode != 0) {
			return keycode;
		}
		keycode = mapDecompositionToKeystrokes(keys, keysym, false);
		if (keycode != 0) {
			// no key is left synthetically down when using the compose key
			// so return 0 even though we succeeded.
			return 0;
		}
	}

	// if the keysym is caps lock sensitive then convert the case of
	// the keysym and try again.
	KeySym lKey, uKey;
	XConvertCase(keysym, &lKey, &uKey);
	if (lKey != uKey) {
		if (lKey == keysym) {
			keyIndex = m_keysymMap.find(uKey);
		}
		else {
			keyIndex = m_keysymMap.find(lKey);
		}
		if (keyIndex != m_keysymMap.end()) {
			// the keysym is mapped to some keycode.  create the keystrokes
			// for this keysym.
			return mapToKeystrokes(keys, keyIndex, isAutoRepeat, false);
		}
	}

	return 0;
}

void
CXWindowsKeyState::updateKeysymMap()
{
	// there are up to 4 keysyms per keycode
	static const unsigned int maxKeysyms = 4;

	// get the number of keycodes
	int minKeycode, maxKeycode;
	XDisplayKeycodes(m_display, &minKeycode, &maxKeycode);
	const int numKeycodes = maxKeycode - minKeycode + 1;

	// get the keyboard mapping for all keys
	int keysymsPerKeycode;
	KeySym* keysyms = XGetKeyboardMapping(m_display,
								minKeycode, numKeycodes,
								&keysymsPerKeycode);

	// we only understand up to maxKeysyms keysyms per keycodes
	unsigned int numKeysyms = keysymsPerKeycode;
	if (numKeysyms > maxKeysyms) {
		numKeysyms = maxKeysyms;
	}

	// determine shift and mode switch sensitivity.  a keysym is shift
	// or mode switch sensitive if its keycode is.  a keycode is mode
	// mode switch sensitive if it has keysyms for indices 2 or 3.
	// it's shift sensitive if the keysym for index 1 (if any) is
	// different from the keysym for index 0 and, if the keysym for
	// for index 3 (if any) is different from the keysym for index 2.
	// that is, if shift changes the generated keysym for the keycode.
	std::vector<bool> usesShift(numKeycodes);
	std::vector<bool> usesModeSwitch(numKeycodes);
	for (int i = 0; i < numKeycodes; ++i) {
		// check mode switch first
		if (numKeysyms > 2 &&
			keysyms[i * keysymsPerKeycode + 2] != NoSymbol ||
			keysyms[i * keysymsPerKeycode + 3] != NoSymbol) {
			usesModeSwitch[i] = true;
		}

		// check index 0 with index 1 keysyms
		if (keysyms[i * keysymsPerKeycode + 0] != NoSymbol &&
			keysyms[i * keysymsPerKeycode + 1] != NoSymbol &&
			keysyms[i * keysymsPerKeycode + 1] !=
				keysyms[i * keysymsPerKeycode + 0]) {
			usesShift[i] = true;
		}

		else if (numKeysyms >= 4 &&
			keysyms[i * keysymsPerKeycode + 2] != NoSymbol &&
			keysyms[i * keysymsPerKeycode + 3] != NoSymbol &&
			keysyms[i * keysymsPerKeycode + 3] !=
				keysyms[i * keysymsPerKeycode + 2]) {
			usesShift[i] = true;
		}
	}

	// get modifier map from server
	XModifierKeymap* modifiers   = XGetModifierMapping(m_display);
	unsigned int keysPerModifier = modifiers->max_keypermod;

	// clear state
	m_keysymMap.clear();
	m_modeSwitchKeysym = NoSymbol;
	m_altMask          = 0;
	m_metaMask         = 0;
	m_superMask        = 0;
	m_modeSwitchMask   = 0;
	m_numLockMask      = 0;
	m_scrollLockMask   = 0;

	// work around for my system, which reports this state bit when
	// mode switch is down, instead of the appropriate modifier bit.
	// should have no effect on other systems.  -crs 9/02.
	m_modeSwitchMask |= (1 << 13);

	// for each modifier keycode, get the index 0 keycode and add it to
	// the keysym map.  also collect all keycodes for each modifier.
	for (unsigned int i = 0; i < 8; ++i) {
		// no keycodes for this modifier yet
		KeyModifierMask mask = 0;
		KeyButtons modifierKeys;

		// add each keycode for modifier
		for (unsigned int j = 0; j < keysPerModifier; ++j) {
			// get keycode and ignore unset keycodes
			KeyCode keycode = modifiers->modifiermap[i * keysPerModifier + j];
			if (keycode == 0) {
				continue;
			}

			// get keysym and get/create key mapping
			int keycodeIndex = keycode - minKeycode;
			KeySym keysym    = keysyms[keycodeIndex *
											keysymsPerKeycode + 0];

			// prefer XK_ISO_Level3_Shift over XK_Mode_switch.  newer
			// versions of X use the former only.  we assume here that
			// if you have XK_ISO_Level3_Shift mapped then that's what
			// you want to use even if XK_Mode_switch is also mapped.
			if (j == 0 && keysym == XK_Mode_switch) {
				// sort modifiers->modifiermap for this modifier so
				// that a keycode mapped to XK_ISO_Level3_Shift appears
				// before those mapped to anything else.  this one keycode
				// is enough so long as it's first because mapModifier in
				// CKeyState uses the first keycode in modifierKeys to
				// generate the key event for this modifier.
				KeyCode* keycodes = modifiers->modifiermap +
										i * keysPerModifier + j;
				for (unsigned int k = j + 1; k < keysPerModifier; ++k) {
					KeySym keysym2 = keysyms[(keycodes[k] - minKeycode) *
										keysymsPerKeycode + 0];
					if (keysym2 == XK_ISO_Level3_Shift) {
						// found an XK_ISO_Level3_Shift.  swap it with
						// the keycode in index j.
						keycodes[j] = keycodes[k];
						keycodes[k] = keycode;

						// now use the new first keycode
						keycode      = keycodes[j];
						keycodeIndex = keycode - minKeycode;
						keysym       = keysym2;
						break;
					}
				}
			}

			// get modifier mask if we haven't yet.  this has the side
			// effect of setting the m_*Mask members.
			if (mask == 0) {
				mask = mapToModifierMask(i, keysym);
				if (mask == 0) {
					continue;
				}
			}

			// save keycode for modifier
			modifierKeys.push_back(keycode);

			// skip if we already have a keycode for this index
			KeyMapping& mapping = m_keysymMap[keysym];
			if (mapping.m_keycode[0] != 0) {
				continue;
			}

			// fill in keysym info
			mapping.m_keycode[0]             = keycode;
			mapping.m_shiftSensitive[0]      = usesShift[keycodeIndex];
			mapping.m_modeSwitchSensitive[0] = usesModeSwitch[keycodeIndex];
			mapping.m_modifierMask           = mask;
			mapping.m_capsLockSensitive      = false;
			mapping.m_numLockSensitive       = false;
		}

		// note this modifier
		if (mask != 0) {
			addModifier(mask, modifierKeys);
		}
	}

	// create a convenient NoSymbol entry (if it doesn't exist yet).
	// sometimes it's useful to handle NoSymbol like a normal keysym.
	// remove any entry for NoSymbol.  that keysym doesn't count.
	{
		KeyMapping& mapping = m_keysymMap[NoSymbol];
		for (unsigned int i = 0; i < numKeysyms; ++i) {
			mapping.m_keycode[i]             = 0;
			mapping.m_shiftSensitive[i]      = false;
			mapping.m_modeSwitchSensitive[i] = false;
		}
		mapping.m_modifierMask      = 0;
		mapping.m_capsLockSensitive = false;
		mapping.m_numLockSensitive  = false;
	}

	// add each keysym to the map, unless we've already inserted a key
	// for that keysym index.
	for (int i = 0; i < numKeycodes; ++i) {
		for (unsigned int j = 0; j < numKeysyms; ++j) {
			// lookup keysym
			const KeySym keysym = keysyms[i * keysymsPerKeycode + j];
			if (keysym == NoSymbol) {
				continue;
			}
			KeyMapping& mapping = m_keysymMap[keysym];

			// skip if we already have a keycode for this index
			if (mapping.m_keycode[j] != 0) {
				continue;
			}

			// fill in keysym info
			if (mapping.m_keycode[0] == 0) {
				mapping.m_modifierMask       = 0;
			}
			mapping.m_keycode[j]             = static_cast<KeyCode>(
												minKeycode + i);
			mapping.m_shiftSensitive[j]      = usesShift[i];
			mapping.m_modeSwitchSensitive[j] = usesModeSwitch[i];
			mapping.m_numLockSensitive       = isNumLockSensitive(keysym);
			mapping.m_capsLockSensitive      = isCapsLockSensitive(keysym);
		}
	}

	// clean up
	XFreeModifiermap(modifiers);
	XFree(keysyms);
}

KeyModifierMask
CXWindowsKeyState::mapToModifierMask(unsigned int i, KeySym keysym)
{
	// some modifier indices (0,1,2) are dedicated to particular uses,
	// the rest depend on the keysyms bound.
	switch (i) {
	case 0:
		return KeyModifierShift;

	case 1:
		return KeyModifierCapsLock;

	case 2:
		return KeyModifierControl;

	default:
		switch (keysym) {
		case XK_Shift_L:
		case XK_Shift_R:
			return KeyModifierShift;

		case XK_Control_L:
		case XK_Control_R:
			return KeyModifierControl;

		case XK_Alt_L:
		case XK_Alt_R:
			m_altMask = (1 << i);
			return KeyModifierAlt;

		case XK_Meta_L:
		case XK_Meta_R:
			m_metaMask = (1 << i);
			return KeyModifierMeta;

		case XK_Super_L:
		case XK_Super_R:
			m_superMask = (1 << i);
			return KeyModifierSuper;

		case XK_Mode_switch:
		case XK_ISO_Level3_Shift:
			m_modeSwitchMask = (1 << i);
			return KeyModifierModeSwitch;

		case XK_Caps_Lock:
			return KeyModifierCapsLock;

		case XK_Num_Lock:
			m_numLockMask = (1 << i);
			return KeyModifierNumLock;

		case XK_Scroll_Lock:
			m_scrollLockMask = (1 << i);
			return KeyModifierScrollLock;

		default:
			return 0;
		}
	}
}

void
CXWindowsKeyState::updateModifiers()
{
	struct CHandedModifiers {
		KeySym m_left;
		KeySym m_right;
	};
	static const CHandedModifiers s_handedModifiers[] = {
		{ XK_Shift_L,   XK_Shift_R   },
		{ XK_Control_L, XK_Control_R },
		{ XK_Meta_L,    XK_Meta_R    },
		{ XK_Alt_L,     XK_Alt_R     },
		{ XK_Super_L,   XK_Super_R   },
		{ XK_Hyper_L,   XK_Hyper_R   }
	};
	struct CModifierBitInfo {
	public:
		KeySym CXWindowsKeyState::*m_keysym;
		KeySym m_left;
		KeySym m_right;
	};
	static const CModifierBitInfo s_modifierBitTable[] = {
	{ &CXWindowsKeyState::m_modeSwitchKeysym, XK_Mode_switch, NoSymbol },
	{ &CXWindowsKeyState::m_modeSwitchKeysym, XK_ISO_Level3_Shift, NoSymbol },
	};

	// for any modifier with left/right versions that has one side but
	// not the other mapped, map the missing side to the existing side.
	// this will map, for example, Alt_R to Alt_L if Alt_L is mapped
	// but Alt_R isn't.  this is almost always what the user wants
	// since the user almost never cares about the difference between
	// Alt_L and Alt_R.
	for (size_t i = 0; i < sizeof(s_handedModifiers) /
							sizeof(s_handedModifiers[0]); ++i) {
		KeySymIndex lIndex = m_keysymMap.find(s_handedModifiers[i].m_left);
		KeySymIndex rIndex = m_keysymMap.find(s_handedModifiers[i].m_right);
		if (lIndex == m_keysymMap.end() && rIndex != m_keysymMap.end()) {
			m_keysymMap[s_handedModifiers[i].m_left]  = rIndex->second;
		}
		else if (lIndex != m_keysymMap.end() && rIndex == m_keysymMap.end()) {
			m_keysymMap[s_handedModifiers[i].m_right] = lIndex->second;
		}
	}

	// choose the keysym to use for some modifiers.  if a modifier has
	// both left and right versions then (arbitrarily) prefer the left.
	for (size_t i = 0; i < sizeof(s_modifierBitTable) /
							sizeof(s_modifierBitTable[0]); ++i) {
		const CModifierBitInfo& info = s_modifierBitTable[i];

		// find available keysym
		KeySymIndex keyIndex = m_keysymMap.find(info.m_left);
		if (keyIndex == m_keysymMap.end() && info.m_right != NoSymbol) {
			keyIndex = m_keysymMap.find(info.m_right);
		}

		// save modifier info
		if (keyIndex                        != m_keysymMap.end() &&
			keyIndex->second.m_modifierMask != 0) {
			this->*(info.m_keysym) = keyIndex->first;
		}
	}

	// if there's no mode switch key mapped then remove all keycodes
	// that depend on it and no keycode can be mode switch sensitive.
	if (m_modeSwitchKeysym == NoSymbol) {
		LOG((CLOG_DEBUG2 "no mode switch in keymap"));
		for (KeySymMap::iterator i = m_keysymMap.begin();
								i != m_keysymMap.end(); ) {
			i->second.m_keycode[2]             = 0;
			i->second.m_keycode[3]             = 0;
			i->second.m_modeSwitchSensitive[0] = false;
			i->second.m_modeSwitchSensitive[1] = false;
			i->second.m_modeSwitchSensitive[2] = false;
			i->second.m_modeSwitchSensitive[3] = false;

			// if this keysym no has no keycodes then remove it
			// except for the NoSymbol keysym mapping.
			if (i->second.m_keycode[0] == 0 && i->second.m_keycode[1] == 0) {
				m_keysymMap.erase(i++);
			}
			else {
				++i;
			}
		}
	}
}

KeySym
CXWindowsKeyState::keyIDToKeySym(KeyID id, KeyModifierMask mask) const
{
	// convert id to keysym
	KeySym keysym = NoSymbol;
	if ((id & 0xfffff000) == 0xe000) {
		// special character
		switch (id & 0x0000ff00) {
#if HAVE_X11_XF86KEYSYM_H
		case 0xe000:
			return g_mapE000[id & 0xff];
#endif

		case 0xee00:
			// ISO 9995 Function and Modifier Keys
			if (id == kKeyLeftTab) {
				keysym = XK_ISO_Left_Tab;
			}
			break;

		case 0xef00:
			// MISCELLANY
			keysym = static_cast<KeySym>(id - 0xef00 + 0xff00);
			break;
		}
	}
	else if ((id >= 0x0020 && id <= 0x007e) ||
			(id >= 0x00a0 && id <= 0x00ff)) {
		// Latin-1 maps directly
		return static_cast<KeySym>(id);
	}
	else {
		// lookup keysym in table
		return CXWindowsUtil::mapUCS4ToKeySym(id);
	}

	// fail if unknown key
	if (keysym == NoSymbol) {
		return keysym;
	}

	// if kKeyTab is requested with shift active then try XK_ISO_Left_Tab
	// instead.  if that doesn't work, we'll fall back to XK_Tab with
	// shift active.  this is to handle primary screens that don't map
	// XK_ISO_Left_Tab sending events to secondary screens that do.
	if (keysym == XK_Tab && (mask & KeyModifierShift) != 0) {
		keysym = XK_ISO_Left_Tab;
	}

	// some keysyms have emergency backups (particularly the numpad
	// keys since most laptops don't have a separate numpad and the
	// numpad overlaying the main keyboard may not have movement
	// key bindings).  figure out the emergency backup.
	KeySym backupKeysym;
	switch (keysym) {
	case XK_KP_Home:
		backupKeysym = XK_Home;
		break;

	case XK_KP_Left:
		backupKeysym = XK_Left;
		break;

	case XK_KP_Up:
		backupKeysym = XK_Up;
		break;

	case XK_KP_Right:
		backupKeysym = XK_Right;
		break;

	case XK_KP_Down:
		backupKeysym = XK_Down;
		break;

	case XK_KP_Prior:
		backupKeysym = XK_Prior;
		break;

	case XK_KP_Next:
		backupKeysym = XK_Next;
		break;

	case XK_KP_End:
		backupKeysym = XK_End;
		break;

	case XK_KP_Insert:
		backupKeysym = XK_Insert;
		break;

	case XK_KP_Delete:
		backupKeysym = XK_Delete;
		break;

	case XK_ISO_Left_Tab:
		backupKeysym = XK_Tab;
		break;

	default:
		backupKeysym = keysym;
		break;
	}

	// see if the keysym is assigned to any keycode.  if not and the
	// backup keysym is then use the backup keysym.
	if (backupKeysym != keysym &&
		m_keysymMap.find(keysym)       == m_keysymMap.end() &&
		m_keysymMap.find(backupKeysym) != m_keysymMap.end()) {
		keysym = backupKeysym;
	}

	return keysym;
}

KeyButton
CXWindowsKeyState::mapToKeystrokes(Keystrokes& keys,
				KeySymIndex keyIndex, bool isAutoRepeat,
				bool pressAndRelease) const
{
	// keyIndex must be valid
	assert(keyIndex != m_keysymMap.end());

	KeyModifierMask currentMask = getActiveModifiers();

	// get the keysym we're trying to generate and possible keycodes
	const KeySym keysym       = keyIndex->first;
	const KeyMapping& mapping = keyIndex->second;
	LOG((CLOG_DEBUG2 "keysym = 0x%08x", keysym));

	// get the best keycode index for the keysym and modifiers.  note
	// that (bestIndex & 1) == 0 if the keycode is a shift modifier
	// and (bestIndex & 2) == 0 if the keycode is a mode switch
	// modifier.  this is important later because we don't want
	// adjustModifiers() to adjust a modifier if that's the key we're
	// mapping.
	unsigned int bestIndex = findBestKeyIndex(keyIndex, currentMask);

	// get the keycode
	KeyButton keycode = mapping.m_keycode[bestIndex];

	// flip low bit of bestIndex if shift is inverted.  if there's a
	// keycode for this new index then use it.  otherwise use the old
	// keycode.  you'd think we should fail if there isn't a keycode
	// for the new index but some keymaps only include the upper case
	// keysyms (notably those on Sun Solaris) so to handle the missing
	// lower case keysyms we just use the old keycode.  note that
	// isShiftInverted() will always return false for a shift modifier.
	if (isShiftInverted(keyIndex, currentMask)) {
		LOG((CLOG_DEBUG2 "shift is inverted"));
		bestIndex ^= 1;
		if (mapping.m_keycode[bestIndex] != 0) {
			keycode = mapping.m_keycode[bestIndex];
		}
	}
	LOG((CLOG_DEBUG2 "bestIndex = %d, keycode = %d", bestIndex, keycode));

	// if this for auto-repeat and this key does not auto-repeat
	// then return 0.
	if (isAutoRepeat &&
		(m_keyControl.auto_repeats[keycode >> 3] &
							static_cast<char>(1 << (keycode & 7))) == 0) {
		LOG((CLOG_DEBUG2 "non-autorepeating"));
		return 0;
	}

	// compute desired mask.  the desired mask is the one that matches
	// bestIndex, except if the key being synthesized is a shift key
	// where we desire what we already have or if it's the mode switch
	// key where we only desire to adjust shift.  also, if the keycode
	// is not sensitive to shift then don't adjust it, otherwise
	// something like shift+home would become just home.  similiarly
	// for mode switch.
	KeyModifierMask desiredMask = currentMask;
	if (keyIndex->second.m_modifierMask != KeyModifierShift) {
		if (keyIndex->second.m_shiftSensitive[bestIndex]) {
			if ((bestIndex & 1) != 0) {
				desiredMask |= KeyModifierShift;
			}
			else {
				desiredMask &= ~KeyModifierShift;
			}
		}
		if (keyIndex->second.m_modifierMask != KeyModifierModeSwitch) {
			if (keyIndex->second.m_modeSwitchSensitive[bestIndex]) {
				if ((bestIndex & 2) != 0) {
					desiredMask |= KeyModifierModeSwitch;
				}
				else {
					desiredMask &= ~KeyModifierModeSwitch;
				}
			}
		}
	}

	// adjust the modifiers to match the desired modifiers
	Keystrokes undo;
	if (!adjustModifiers(keys, undo, desiredMask)) {
		LOG((CLOG_DEBUG2 "failed to adjust modifiers"));
		return 0;
	}

	// add the key event
	Keystroke keystroke;
	keystroke.m_key        = keycode;
	if (pressAndRelease) {
		keystroke.m_press  = true;
		keystroke.m_repeat = false;
		keys.push_back(keystroke);
		keystroke.m_press  = false;
		keys.push_back(keystroke);
	}
	else if (!isAutoRepeat) {
		keystroke.m_press  = true;
		keystroke.m_repeat = false;
		keys.push_back(keystroke);
	}
	else {
		keystroke.m_press  = false;
		keystroke.m_repeat = true;
		keys.push_back(keystroke);
		keystroke.m_press  = true;
		keys.push_back(keystroke);
	}

	// put undo keystrokes at end of keystrokes in reverse order
	while (!undo.empty()) {
		keys.push_back(undo.back());
		undo.pop_back();
	}

	return keycode;
}

KeyButton
CXWindowsKeyState::mapDecompositionToKeystrokes(
				Keystrokes& keys, KeySym keysym, bool usingDeadKeys) const
{
	// decompose the keysym
	CXWindowsUtil::KeySyms decomposed;
	if (usingDeadKeys) {
		if (!CXWindowsUtil::decomposeKeySymWithDeadKeys(keysym, decomposed)) {
			// no decomposition
			return 0;
		}
		LOG((CLOG_DEBUG2 "decomposed keysym 0x%08x into %d keysyms using dead keys", keysym, decomposed.size()));
	}
	else {
		if (!CXWindowsUtil::decomposeKeySymWithCompose(keysym, decomposed)) {
			// no decomposition
			return 0;
		}
		LOG((CLOG_DEBUG2 "decomposed keysym 0x%08x into %d keysyms using compose key", keysym, decomposed.size()));
	}
	size_t n = decomposed.size();
	if (n == 0) {
		// nothing in the decomposition
		return 0;
	}

	// map to keystrokes
	Keystrokes keystrokes;
	KeyButton keycode = 0;
	for (size_t i = 0; i < n; ++i) {
		// lookup the key
		keysym               = decomposed[i];
		KeySymIndex keyIndex = m_keysymMap.find(keysym);
		if (keyIndex == m_keysymMap.end()) {
			// missing a required keysym
			LOG((CLOG_DEBUG2 "can't map keysym %d: 0x%04x", i, keysym));
			return 0;
		}

		// the keysym is mapped to some keycode.  add press and
		// release unless this is the last key and usingDeadKeys.
		keycode = mapToKeystrokes(keystrokes, keyIndex,
						false, (i + 1 < n || !usingDeadKeys));
		if (keycode == 0) {
			return 0;
		}
	}

	// copy keystrokes
	keys.insert(keys.end(), keystrokes.begin(), keystrokes.end());

	return keycode;
}

unsigned int
CXWindowsKeyState::findBestKeyIndex(KeySymIndex keyIndex,
				KeyModifierMask /*currentMask*/) const
{
	// there are up to 4 keycodes per keysym to choose from.  the
	// best choice is the one that requires the fewest adjustments
	// to the modifier state.  for example, the letter A normally
	// requires shift + a.  if shift isn't already down we'd have
	// to synthesize a shift press before the a press.  however,
	// if A could also be created with some other keycode without
	// shift then we'd prefer that when shift wasn't down.
	//
	// if the action is an auto-repeat then we don't call this
	// method since we just need to synthesize a key repeat on the
	// same keycode that we pressed.
	// FIXME -- do this right
	for (unsigned int i = 0; i < 4; ++i) {
		if (keyIndex->second.m_keycode[i] != 0) {
			return i;
		}
	}

	assert(0 && "no keycode found for keysym");
	return 0;
}

bool
CXWindowsKeyState::isShiftInverted(KeySymIndex keyIndex,
				KeyModifierMask currentMask) const
{
	// each keycode has up to 4 keysym associated with it, one each for:
	// no modifiers, shift, mode switch, and shift and mode switch.  if
	// a keysym is modified by num lock and num lock is active then you
	// get the shifted keysym when shift is not down and the unshifted
	// keysym when it is.  that is, num lock inverts the sense of the
	// shift modifier when active.  similarly for caps lock.  this
	// method returns true iff the sense of shift should be inverted
	// for this key given a modifier state.
	if (keyIndex->second.m_numLockSensitive) {
		if ((currentMask & KeyModifierNumLock) != 0) {
			return true;
		}
	}

	// if a keysym is num lock sensitive it is never caps lock
	// sensitive, thus the else here.
	else if (keyIndex->second.m_capsLockSensitive) {
		if ((currentMask & KeyModifierCapsLock) != 0) {
			return true;
		}
	}

	return false;
}

bool
CXWindowsKeyState::adjustModifiers(Keystrokes& keys,
				Keystrokes& undo,
				KeyModifierMask desiredMask) const
{
	KeyModifierMask currentMask = getActiveModifiers();

	// get mode switch set correctly.  do this before shift because
	// mode switch may be sensitive to the shift modifier and will
	// set/reset it as necessary.
	bool forceShift     = false;
	bool wantShift      = ((desiredMask & KeyModifierShift) != 0);
	bool wantModeSwitch = ((desiredMask & KeyModifierModeSwitch) != 0);
	bool haveModeSwitch = ((currentMask & KeyModifierModeSwitch) != 0);
	if (wantModeSwitch != haveModeSwitch) {
		LOG((CLOG_DEBUG2 "fix mode switch"));

		// adjust shift if necessary (i.e. turn it off it's on and mode
		// shift is sensitive to the shift key)
		KeySymIndex modeSwitchIndex = m_keysymMap.find(m_modeSwitchKeysym);
		assert(modeSwitchIndex != m_keysymMap.end());
		if (modeSwitchIndex->second.m_shiftSensitive[0]) {
			bool haveShift = ((currentMask & KeyModifierShift) != 0);
			if (haveShift) {
				// add shift keystrokes
				LOG((CLOG_DEBUG2 "fix shift for mode switch"));
				if (!mapModifier(keys, undo, KeyModifierShift, false, true)) {
					return false;
				}

				// our local concept of shift has flipped
				currentMask ^= KeyModifierShift;

				// force shift to get turned on below if we had to turn
				// off here and shift is desired.  if we didn't force it
				// then mapModifier would think shift is already down
				// and ignore the request.
				forceShift   = wantShift;
			}
		}

		// add mode switch keystrokes
		if (!mapModifier(keys, undo, KeyModifierModeSwitch, wantModeSwitch)) {
			return false;
		}
		currentMask ^= KeyModifierModeSwitch;
	}

	// get shift set correctly
	bool haveShift = ((currentMask & KeyModifierShift) != 0);
	if (wantShift != haveShift) {
		// add shift keystrokes
		LOG((CLOG_DEBUG2 "fix shift"));
		if (!mapModifier(keys, undo, KeyModifierShift, wantShift, forceShift)) {
			return false;
		}
		currentMask ^= KeyModifierShift;
	}

	return true;
}

bool
CXWindowsKeyState::isNumLockSensitive(KeySym keysym) const
{
	return (IsKeypadKey(keysym) || IsPrivateKeypadKey(keysym));
}

bool
CXWindowsKeyState::isCapsLockSensitive(KeySym keysym) const
{
	KeySym lKey, uKey;
	XConvertCase(keysym, &lKey, &uKey);
	return (lKey != uKey);
}


//
// CXWindowsKeyState::KeyMapping
//

CXWindowsKeyState::KeyMapping::KeyMapping()
{
	m_keycode[0] = 0;
	m_keycode[1] = 0;
	m_keycode[2] = 0;
	m_keycode[3] = 0;
}
