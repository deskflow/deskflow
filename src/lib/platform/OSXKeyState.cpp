/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Synergy Si Ltd.
 * Copyright (C) 2004 Chris Schoeneman
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

#include "platform/OSXKeyState.h"
#include "arch/Arch.h"
#include "base/Log.h"

#if defined(MAC_OS_X_VERSION_10_5)
#include <Carbon/Carbon.h>
#endif

// Note that some virtual keys codes appear more than once.  The
// first instance of a virtual key code maps to the KeyID that we
// want to generate for that code.  The others are for mapping
// different KeyIDs to a single key code.

#if defined(MAC_OS_X_VERSION_10_5)
static const UInt32 s_shiftVK    = kVK_Shift;
static const UInt32 s_controlVK  = kVK_Control;
static const UInt32 s_altVK      = kVK_Option;
static const UInt32 s_superVK    = kVK_Command;
static const UInt32 s_capsLockVK = kVK_CapsLock;
static const UInt32 s_numLockVK  = kVK_ANSI_KeypadClear; // 71
#else
// Hardcoded virtual key table on 10.4 and below.
static const UInt32 s_shiftVK    = 56;
static const UInt32 s_controlVK  = 59;
static const UInt32 s_altVK      = 58;
static const UInt32 s_superVK    = 55;
static const UInt32 s_capsLockVK = 57;
static const UInt32 s_numLockVK  = 71;
#endif

static const UInt32 s_osxNumLock = 1 << 16;

struct KeyEntry {
public:
	KeyID				m_keyID;
	UInt32				m_virtualKey;
};
static const KeyEntry	s_controlKeys[] = {
#if defined(MAC_OS_X_VERSION_10_5)
	// cursor keys.  if we don't do this we'll may still get these from
	// the keyboard resource but they may not correspond to the arrow
	// keys.
	{ kKeyLeft,		kVK_LeftArrow },
	{ kKeyRight,		kVK_RightArrow },
	{ kKeyUp,		kVK_UpArrow },
	{ kKeyDown,		kVK_DownArrow },
	{ kKeyHome,		kVK_Home },
	{ kKeyEnd,		kVK_End },
	{ kKeyPageUp,		kVK_PageUp },
	{ kKeyPageDown,		kVK_PageDown },
	{ kKeyInsert,		kVK_Help }, // Mac Keyboards have 'Help' on 'Insert'

	// function keys
	{ kKeyF1,		kVK_F1 },
	{ kKeyF2,		kVK_F2 },
	{ kKeyF3,		kVK_F3 },
	{ kKeyF4,		kVK_F4 },
	{ kKeyF5,		kVK_F5 },
	{ kKeyF6,		kVK_F6 },
	{ kKeyF7,		kVK_F7 },
	{ kKeyF8,		kVK_F8 },
	{ kKeyF9,		kVK_F9 },
	{ kKeyF10,		kVK_F10 },
	{ kKeyF11,		kVK_F11 },
	{ kKeyF12,		kVK_F12 },
	{ kKeyF13,		kVK_F13 },
	{ kKeyF14,		kVK_F14 },
	{ kKeyF15,		kVK_F15 },
	{ kKeyF16,		kVK_F16 },

	{ kKeyKP_0,		kVK_ANSI_Keypad0 },
	{ kKeyKP_1,		kVK_ANSI_Keypad1 },
	{ kKeyKP_2,		kVK_ANSI_Keypad2 },
	{ kKeyKP_3,		kVK_ANSI_Keypad3 },
	{ kKeyKP_4,		kVK_ANSI_Keypad4 },
	{ kKeyKP_5,		kVK_ANSI_Keypad5 },
	{ kKeyKP_6,		kVK_ANSI_Keypad6 },
	{ kKeyKP_7,		kVK_ANSI_Keypad7 },
	{ kKeyKP_8,		kVK_ANSI_Keypad8 },
	{ kKeyKP_9,		kVK_ANSI_Keypad9 },
	{ kKeyKP_Decimal,	kVK_ANSI_KeypadDecimal },
	{ kKeyKP_Equal,		kVK_ANSI_KeypadEquals },
	{ kKeyKP_Multiply,	kVK_ANSI_KeypadMultiply },
	{ kKeyKP_Add,		kVK_ANSI_KeypadPlus },
	{ kKeyKP_Divide,	kVK_ANSI_KeypadDivide },
	{ kKeyKP_Subtract,	kVK_ANSI_KeypadMinus },
	{ kKeyKP_Enter,		kVK_ANSI_KeypadEnter },
#else
  // Hardcoded virtual key table on 10.4 and below.
	// cursor keys.
	{ kKeyLeft,			123 },
	{ kKeyRight,		124 },
	{ kKeyUp,			126 },
	{ kKeyDown,			125 },
	{ kKeyHome,			115 },
	{ kKeyEnd,			119 },
	{ kKeyPageUp,		116 },
	{ kKeyPageDown,		121 },
	{ kKeyInsert,		114 },

	// function keys
	{ kKeyF1,			122 },
	{ kKeyF2,			120 },
	{ kKeyF3,			99 },
	{ kKeyF4,			118 },
	{ kKeyF5,			96 },
	{ kKeyF6,			97 },
	{ kKeyF7,			98 },
	{ kKeyF8,			100 },
	{ kKeyF9,			101 },
	{ kKeyF10,			109 },
	{ kKeyF11,			103 },
	{ kKeyF12,			111 },
	{ kKeyF13,			105 },
	{ kKeyF14,			107 },
	{ kKeyF15,			113 },
	{ kKeyF16,			106 },

	{ kKeyKP_0,			82 },
	{ kKeyKP_1,			83 },
	{ kKeyKP_2,			84 },
	{ kKeyKP_3,			85 },
	{ kKeyKP_4,			86 },
	{ kKeyKP_5,			87 },
	{ kKeyKP_6,			88 },
	{ kKeyKP_7,			89 },
	{ kKeyKP_8,			91 },
	{ kKeyKP_9,			92 },
	{ kKeyKP_Decimal,	65 },
	{ kKeyKP_Equal,		81 },
	{ kKeyKP_Multiply,	67 },
	{ kKeyKP_Add,		69 },
	{ kKeyKP_Divide,	75 },
	{ kKeyKP_Subtract,	78 },
	{ kKeyKP_Enter,		76 },
#endif

	// virtual key 110 is fn+enter and i have no idea what that's supposed
	// to map to.  also the enter key with numlock on is a modifier but i
	// don't know which.

	// modifier keys.  OS X doesn't seem to support right handed versions
	// of modifier keys so we map them to the left handed versions.
	{ kKeyShift_L,		s_shiftVK },
	{ kKeyShift_R,		s_shiftVK }, // 60
	{ kKeyControl_L,	s_controlVK },
	{ kKeyControl_R,	s_controlVK }, // 62
	{ kKeyAlt_L,		s_altVK },
	{ kKeyAlt_R,		s_altVK },
	{ kKeySuper_L,		s_superVK },
	{ kKeySuper_R,		s_superVK }, // 61
	{ kKeyMeta_L,		s_superVK },
	{ kKeyMeta_R,		s_superVK }, // 61

	// toggle modifiers
	{ kKeyNumLock,		s_numLockVK },
	{ kKeyCapsLock,		s_capsLockVK }
};


//
// OSXKeyState
//

OSXKeyState::OSXKeyState(IEventQueue* events) :
	KeyState(events)
{
	init();
}

OSXKeyState::OSXKeyState(IEventQueue* events, synergy::KeyMap& keyMap) :
	KeyState(events, keyMap)
{
	init();
}

OSXKeyState::~OSXKeyState()
{
}

void
OSXKeyState::init()
{
	m_deadKeyState = 0;
	m_shiftPressed = false;
	m_controlPressed = false;
	m_altPressed = false;
	m_superPressed = false;
	m_capsPressed = false;

	// build virtual key map
	for (size_t i = 0; i < sizeof(s_controlKeys) / sizeof(s_controlKeys[0]);
		++i) {
		
		m_virtualKeyMap[s_controlKeys[i].m_virtualKey] =
			s_controlKeys[i].m_keyID;
	}
}

KeyModifierMask
OSXKeyState::mapModifiersFromOSX(UInt32 mask) const
{
	KeyModifierMask outMask = 0;
	if ((mask & kCGEventFlagMaskShift) != 0) {
		outMask |= KeyModifierShift;
	}
	if ((mask & kCGEventFlagMaskControl) != 0) {
		outMask |= KeyModifierControl;
	}
	if ((mask & kCGEventFlagMaskAlternate) != 0) {
		outMask |= KeyModifierAlt;
	}
	if ((mask & kCGEventFlagMaskCommand) != 0) {
		outMask |= KeyModifierSuper;
	}
	if ((mask & kCGEventFlagMaskAlphaShift) != 0) {
		outMask |= KeyModifierCapsLock;
	}
	if ((mask & kCGEventFlagMaskNumericPad) != 0) {
		outMask |= KeyModifierNumLock;
	}

	LOG((CLOG_DEBUG1 "mask=%04x outMask=%04x", mask, outMask));
	return outMask;
}

KeyModifierMask
OSXKeyState::mapModifiersToCarbon(UInt32 mask) const
{
	KeyModifierMask outMask = 0;
	if ((mask & kCGEventFlagMaskShift) != 0) {
		outMask |= shiftKey;
	}
	if ((mask & kCGEventFlagMaskControl) != 0) {
		outMask |= controlKey;
	}
	if ((mask & kCGEventFlagMaskCommand) != 0) {
		outMask |= cmdKey;
	}
	if ((mask & kCGEventFlagMaskAlternate) != 0) {
		outMask |= optionKey;
	}
	if ((mask & kCGEventFlagMaskAlphaShift) != 0) {
		outMask |= alphaLock;
	}
	if ((mask & kCGEventFlagMaskNumericPad) != 0) {
		outMask |= s_osxNumLock;
	}
	
	return outMask;
}

KeyButton 
OSXKeyState::mapKeyFromEvent(KeyIDs& ids,
				KeyModifierMask* maskOut, CGEventRef event) const
{
	ids.clear();

	// map modifier key
	if (maskOut != NULL) {
		KeyModifierMask activeMask = getActiveModifiers();
		activeMask &= ~KeyModifierAltGr;
		*maskOut    = activeMask;
	}

	// get virtual key
	UInt32 vkCode = CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode);

	// handle up events
	UInt32 eventKind = CGEventGetType(event);
	if (eventKind == kCGEventKeyUp) {
		// the id isn't used.  we just need the same button we used on
		// the key press.  note that we don't use or reset the dead key
		// state;  up events should not affect the dead key state.
		ids.push_back(kKeyNone);
		return mapVirtualKeyToKeyButton(vkCode);
	}

	// check for special keys
	VirtualKeyMap::const_iterator i = m_virtualKeyMap.find(vkCode);
	if (i != m_virtualKeyMap.end()) {
		m_deadKeyState = 0;
		ids.push_back(i->second);
		return mapVirtualKeyToKeyButton(vkCode);
	}

	// get keyboard info

#if defined(MAC_OS_X_VERSION_10_5)
	TISInputSourceRef currentKeyboardLayout = TISCopyCurrentKeyboardLayoutInputSource(); 
#else
	KeyboardLayoutRef currentKeyboardLayout;
	OSStatus status = KLGetCurrentKeyboardLayout(&currentKeyboardLayout);
#endif
	if (currentKeyboardLayout == NULL) {
		return kKeyNone;
	}

	// get the event modifiers and remove the command and control
	// keys.  note if we used them though.
	// UCKeyTranslate expects old-style Carbon modifiers, so convert.
	UInt32 modifiers;
	modifiers = mapModifiersToCarbon(CGEventGetFlags(event));
	static const UInt32 s_commandModifiers =
		cmdKey | controlKey | rightControlKey;
	bool isCommand = ((modifiers & s_commandModifiers) != 0);
	modifiers &= ~s_commandModifiers;

	// if we've used a command key then we want the glyph produced without
	// the option key (i.e. the base glyph).
	//if (isCommand) {
		modifiers &= ~optionKey;
	//}

	// choose action
	UInt16 action;
	if(eventKind==kCGEventKeyDown) {
		action = kUCKeyActionDown;
	}
	else if(CGEventGetIntegerValueField(event, kCGKeyboardEventAutorepeat)==1) {
		action = kUCKeyActionAutoKey;
	}
	else {
		return 0;
	}

	// translate via uchr resource
#if defined(MAC_OS_X_VERSION_10_5)
	CFDataRef ref = (CFDataRef) TISGetInputSourceProperty(currentKeyboardLayout,
								kTISPropertyUnicodeKeyLayoutData);
	const UCKeyboardLayout* layout = (const UCKeyboardLayout*) CFDataGetBytePtr(ref);
	const bool layoutValid = (layout != NULL);
#else
	const void* resource;
	int err = KLGetKeyboardLayoutProperty(currentKeyboardLayout, kKLuchrData, &resource);
	const bool layoutValid = (err == noErr);
	const UCKeyboardLayout* layout = (const UCKeyboardLayout*)resource;
#endif

	if (layoutValid) {
		// translate key
		UniCharCount count;
		UniChar chars[2];
		LOG((CLOG_DEBUG2 "modifiers: %08x", modifiers & 0xffu));
		OSStatus status = UCKeyTranslate(layout,
							vkCode & 0xffu, action,
							(modifiers >> 8) & 0xffu,
							LMGetKbdType(), 0, &m_deadKeyState,
							sizeof(chars) / sizeof(chars[0]), &count, chars);

		// get the characters
		if (status == 0) {
			if (count != 0 || m_deadKeyState == 0) {
				m_deadKeyState = 0;
				for (UniCharCount i = 0; i < count; ++i) {
					ids.push_back(KeyResource::unicharToKeyID(chars[i]));
				}
				adjustAltGrModifier(ids, maskOut, isCommand);
				return mapVirtualKeyToKeyButton(vkCode);
			}
			return 0;
		}
	}

	return 0;
}

bool
OSXKeyState::fakeCtrlAltDel()
{
	// pass keys through unchanged
	return false;
}

CGEventFlags
OSXKeyState::getModifierStateAsOSXFlags()
{
    CGEventFlags modifiers = 0;
    
    if (m_shiftPressed) {
        modifiers |= kCGEventFlagMaskShift;
    }
    
    if (m_controlPressed) {
        modifiers |= kCGEventFlagMaskControl;
    }
    
    if (m_altPressed) {
        modifiers |= kCGEventFlagMaskAlternate;
    }
    
    if (m_superPressed) {
        modifiers |= kCGEventFlagMaskCommand;
    }
    
    if (m_capsPressed) {
        modifiers |= kCGEventFlagMaskAlphaShift;
    }
    
    return modifiers;
}

KeyModifierMask
OSXKeyState::pollActiveModifiers() const
{
	// falsely assumed that the mask returned by GetCurrentKeyModifiers()
	// was the same as a CGEventFlags (which is what mapModifiersFromOSX
	// expects). patch by Marc
	UInt32 mask = GetCurrentKeyModifiers();
	KeyModifierMask outMask = 0;

	if ((mask & shiftKey) != 0) {
		outMask |= KeyModifierShift;
	}
	if ((mask & controlKey) != 0) {
		outMask |= KeyModifierControl;
	}
	if ((mask & optionKey) != 0) {
		outMask |= KeyModifierAlt;
	}
	if ((mask & cmdKey) != 0) {
		outMask |= KeyModifierSuper;
	}
	if ((mask & alphaLock) != 0) {
		outMask |= KeyModifierCapsLock;
	}
	if ((mask & s_osxNumLock) != 0) {
		outMask |= KeyModifierNumLock;
	}

	LOG((CLOG_DEBUG1 "mask=%04x outMask=%04x", mask, outMask));
	return outMask;
}

SInt32
OSXKeyState::pollActiveGroup() const
{
	bool layoutValid = true;
#if defined(MAC_OS_X_VERSION_10_5)
	TISInputSourceRef keyboardLayout = TISCopyCurrentKeyboardLayoutInputSource();
#else
	KeyboardLayoutRef keyboardLayout;
	OSStatus status = KLGetCurrentKeyboardLayout(&keyboardLayout);
	layoutValid = (status == noErr);
#endif
	
	if (layoutValid) {
		GroupMap::const_iterator i = m_groupMap.find(keyboardLayout);
		if (i != m_groupMap.end()) {
			return i->second;
		}
	}
	return 0;
}

void
OSXKeyState::pollPressedKeys(KeyButtonSet& pressedKeys) const
{
	::KeyMap km;
	GetKeys(km);
	const UInt8* m = reinterpret_cast<const UInt8*>(km);
	for (UInt32 i = 0; i < 16; ++i) {
		for (UInt32 j = 0; j < 8; ++j) {
			if ((m[i] & (1u << j)) != 0) {
				pressedKeys.insert(mapVirtualKeyToKeyButton(8 * i + j));
			}
		}
	}
}

void
OSXKeyState::getKeyMap(synergy::KeyMap& keyMap)
{
	// update keyboard groups
	if (getGroups(m_groups)) {
		m_groupMap.clear();
		SInt32 numGroups = (SInt32)m_groups.size();
		for (SInt32 g = 0; g < numGroups; ++g) {
			m_groupMap[m_groups[g]] = g;
		}
	}

	UInt32 keyboardType = LMGetKbdType();
	for (SInt32 g = 0, n = (SInt32)m_groups.size(); g < n; ++g) {
		// add special keys
		getKeyMapForSpecialKeys(keyMap, g);

		const void* resource;
		bool layoutValid = false;
		
		// add regular keys
		// try uchr resource first
		#if defined(MAC_OS_X_VERSION_10_5)
		CFDataRef resourceRef = (CFDataRef)TISGetInputSourceProperty(
			m_groups[g], kTISPropertyUnicodeKeyLayoutData);
		layoutValid = resourceRef != NULL;
		if (layoutValid)
			resource = CFDataGetBytePtr(resourceRef);
		#else
		layoutValid = KLGetKeyboardLayoutProperty(
			m_groups[g], kKLuchrData, &resource);
		#endif

		if (layoutValid) {
			CUCHRKeyResource uchr(resource, keyboardType);
			if (uchr.isValid()) {
				LOG((CLOG_DEBUG1 "using uchr resource for group %d", g));
				getKeyMap(keyMap, g, uchr);
				continue;
			}
		}

		LOG((CLOG_DEBUG1 "no keyboard resource for group %d", g));
	}
}

void
OSXKeyState::fakeKey(const Keystroke& keystroke)
{
	switch (keystroke.m_type) {
	case Keystroke::kButton: {
		
		KeyButton button = keystroke.m_data.m_button.m_button;
		bool keyDown = keystroke.m_data.m_button.m_press;
		UInt32 client = keystroke.m_data.m_button.m_client;
		CGEventSourceRef source = 0;
		CGKeyCode virtualKey = mapKeyButtonToVirtualKey(button);
		
		LOG((CLOG_DEBUG1
			"  button=0x%04x virtualKey=0x%04x keyDown=%s client=0x%04x",
			button, virtualKey, keyDown ? "down" : "up", client));

		CGEventRef ref = CGEventCreateKeyboardEvent(
			source, virtualKey, keyDown);
		
		if (ref == NULL) {
			LOG((CLOG_CRIT "unable to create keyboard event for keystroke"));
			return;
		}

		// persist modifier state.
		if (virtualKey == s_shiftVK) {
			m_shiftPressed = keyDown;
		}
		
		if (virtualKey == s_controlVK) {
			m_controlPressed = keyDown;
		}
		
		if (virtualKey == s_altVK) {
			m_altPressed = keyDown;
		}
		
		if (virtualKey == s_superVK) {
			m_superPressed = keyDown;
		}
		
		if (virtualKey == s_capsLockVK) {
			m_capsPressed = keyDown;
		}

		// set the event flags for special keys
		// http://tinyurl.com/pxl742y
		CGEventFlags modifiers = 0;
		
		if (m_shiftPressed) {
			modifiers |= kCGEventFlagMaskShift;
		}
		
		if (m_controlPressed) {
			modifiers |= kCGEventFlagMaskControl;
		}
		
		if (m_altPressed) {
			modifiers |= kCGEventFlagMaskAlternate;
		}
		
		if (m_superPressed) {
			modifiers |= kCGEventFlagMaskCommand;
		}
		
		if (m_capsPressed) {
			modifiers |= kCGEventFlagMaskAlphaShift;
		}
		
		CGEventSetFlags(ref, modifiers);
		CGEventPost(kCGHIDEventTap, ref);
		CFRelease(ref);

		// add a delay if client data isn't zero
		// FIXME -- why?
		if (client != 0) {
			ARCH->sleep(0.01);
		}
		break;
	}

	case Keystroke::kGroup: {
		SInt32 group = keystroke.m_data.m_group.m_group;
		if (keystroke.m_data.m_group.m_absolute) {
			LOG((CLOG_DEBUG1 "  group %d", group));
			setGroup(group);
		}
		else {
			LOG((CLOG_DEBUG1 "  group %+d", group));
			setGroup(getEffectiveGroup(pollActiveGroup(), group));
		}
		break;
	}
	}
}

void
OSXKeyState::getKeyMapForSpecialKeys(synergy::KeyMap& keyMap, SInt32 group) const
{
	// special keys are insensitive to modifers and none are dead keys
	synergy::KeyMap::KeyItem item;
	for (size_t i = 0; i < sizeof(s_controlKeys) /
								sizeof(s_controlKeys[0]); ++i) {
		const KeyEntry& entry = s_controlKeys[i];
		item.m_id        = entry.m_keyID;
		item.m_group     = group;
		item.m_button    = mapVirtualKeyToKeyButton(entry.m_virtualKey);
		item.m_required  = 0;
		item.m_sensitive = 0;
		item.m_dead      = false;
		item.m_client    = 0;
		synergy::KeyMap::initModifierKey(item);
		keyMap.addKeyEntry(item);

		if (item.m_lock) {
			// all locking keys are half duplex on OS X
			keyMap.addHalfDuplexButton(item.m_button);
		}
	}

	// note:  we don't special case the number pad keys.  querying the
	// mac keyboard returns the non-keypad version of those keys but
	// a KeyState always provides a mapping from keypad keys to
	// non-keypad keys so we'll be able to generate the characters
	// anyway.
}

bool
OSXKeyState::getKeyMap(synergy::KeyMap& keyMap,
				SInt32 group, const KeyResource& r) const
{
	if (!r.isValid()) {
		return false;
	}

	// space for all possible modifier combinations
	std::vector<bool> modifiers(r.getNumModifierCombinations());

	// make space for the keys that any single button can synthesize
	std::vector<std::pair<KeyID, bool> > buttonKeys(r.getNumTables());

	// iterate over each button
	synergy::KeyMap::KeyItem item;
	for (UInt32 i = 0; i < r.getNumButtons(); ++i) {
		item.m_button = mapVirtualKeyToKeyButton(i);

		// the KeyIDs we've already handled
		std::set<KeyID> keys;

		// convert the entry in each table for this button to a KeyID
		for (UInt32 j = 0; j < r.getNumTables(); ++j) {
			buttonKeys[j].first  = r.getKey(j, i);
			buttonKeys[j].second = synergy::KeyMap::isDeadKey(buttonKeys[j].first);
		}

		// iterate over each character table
		for (UInt32 j = 0; j < r.getNumTables(); ++j) {
			// get the KeyID for the button/table
			KeyID id = buttonKeys[j].first;
			if (id == kKeyNone) {
				continue;
			}

			// if we've already handled the KeyID in the table then
			// move on to the next table
			if (keys.count(id) > 0) {
				continue;
			}
			keys.insert(id);

			// prepare item.  the client state is 1 for dead keys.
			item.m_id     = id;
			item.m_group  = group;
			item.m_dead   = buttonKeys[j].second;
			item.m_client = buttonKeys[j].second ? 1 : 0;
			synergy::KeyMap::initModifierKey(item);
			if (item.m_lock) {
				// all locking keys are half duplex on OS X
				keyMap.addHalfDuplexButton(i);
			}

			// collect the tables that map to the same KeyID.  we know it
			// can't be any earlier tables because of the check above.
			std::set<UInt8> tables;
			tables.insert(static_cast<UInt8>(j));
			for (UInt32 k = j + 1; k < r.getNumTables(); ++k) {
				if (buttonKeys[k].first == id) {
					tables.insert(static_cast<UInt8>(k));
				}
			}

			// collect the modifier combinations that map to any of the
			// tables we just collected
			for (UInt32 k = 0; k < r.getNumModifierCombinations(); ++k) {
				modifiers[k] = (tables.count(r.getTableForModifier(k)) > 0);
			}

			// figure out which modifiers the key is sensitive to.  the
			// key is insensitive to a modifier if for every modifier mask
			// with the modifier bit unset in the modifiers we also find
			// the same mask with the bit set.
			//
			// we ignore a few modifiers that we know aren't important
			// for generating characters.  in fact, we want to ignore any
			// characters generated by the control key.  we don't map
			// those and instead expect the control modifier plus a key.
			UInt32 sensitive = 0;
			for (UInt32 k = 0; (1u << k) <
								r.getNumModifierCombinations(); ++k) {
				UInt32 bit = (1u << k);
				if ((bit << 8) == cmdKey ||
					(bit << 8) == controlKey ||
					(bit << 8) == rightControlKey) {
					continue;
				}
				for (UInt32 m = 0; m < r.getNumModifierCombinations(); ++m) {
					if (modifiers[m] != modifiers[m ^ bit]) {
						sensitive |= bit;
						break;
					}
				}
			}

			// find each required modifier mask.  the key can be synthesized
			// using any of the masks.
			std::set<UInt32> required;
			for (UInt32 k = 0; k < r.getNumModifierCombinations(); ++k) {
				if ((k & sensitive) == k && modifiers[k & sensitive]) {
					required.insert(k);
				}
			}

			// now add a key entry for each key/required modifier pair.
			item.m_sensitive = mapModifiersFromOSX(sensitive << 8);
			for (std::set<UInt32>::iterator k = required.begin();
											k != required.end(); ++k) {
				item.m_required = mapModifiersFromOSX(*k << 8);
				keyMap.addKeyEntry(item);
			}
		}
	}

	return true;
}

bool
OSXKeyState::mapSynergyHotKeyToMac(KeyID key, KeyModifierMask mask,
				UInt32 &macVirtualKey, UInt32 &macModifierMask) const
{
	// look up button for key
	KeyButton button = getButton(key, pollActiveGroup());
	if (button == 0 && key != kKeyNone) {
		return false;
	}
	macVirtualKey = mapKeyButtonToVirtualKey(button);
	
	// calculate modifier mask
	macModifierMask = 0;
	if ((mask & KeyModifierShift) != 0) {
		macModifierMask |= shiftKey;
	}
	if ((mask & KeyModifierControl) != 0) {
		macModifierMask |= controlKey;
	}
	if ((mask & KeyModifierAlt) != 0) {
		macModifierMask |= cmdKey;
	}
	if ((mask & KeyModifierSuper) != 0) {
		macModifierMask |= optionKey;
	}
	if ((mask & KeyModifierCapsLock) != 0) {
		macModifierMask |= alphaLock;
	}
	if ((mask & KeyModifierNumLock) != 0) {
		macModifierMask |= s_osxNumLock;
	}
	
	return true;
}
						
void
OSXKeyState::handleModifierKeys(void* target,
				KeyModifierMask oldMask, KeyModifierMask newMask)
{
	// compute changed modifiers
	KeyModifierMask changed = (oldMask ^ newMask);

	// synthesize changed modifier keys
	if ((changed & KeyModifierShift) != 0) {
		handleModifierKey(target, s_shiftVK, kKeyShift_L,
							(newMask & KeyModifierShift) != 0, newMask);
	}
	if ((changed & KeyModifierControl) != 0) {
		handleModifierKey(target, s_controlVK, kKeyControl_L,
							(newMask & KeyModifierControl) != 0, newMask);
	}
	if ((changed & KeyModifierAlt) != 0) {
		handleModifierKey(target, s_altVK, kKeyAlt_L,
							(newMask & KeyModifierAlt) != 0, newMask);
	}
	if ((changed & KeyModifierSuper) != 0) {
		handleModifierKey(target, s_superVK, kKeySuper_L,
							(newMask & KeyModifierSuper) != 0, newMask);
	}
	if ((changed & KeyModifierCapsLock) != 0) {
		handleModifierKey(target, s_capsLockVK, kKeyCapsLock,
							(newMask & KeyModifierCapsLock) != 0, newMask);
	}
	if ((changed & KeyModifierNumLock) != 0) {
		handleModifierKey(target, s_numLockVK, kKeyNumLock,
							(newMask & KeyModifierNumLock) != 0, newMask);
	}
}

void
OSXKeyState::handleModifierKey(void* target,
				UInt32 virtualKey, KeyID id,
				bool down, KeyModifierMask newMask)
{
	KeyButton button = mapVirtualKeyToKeyButton(virtualKey);
	onKey(button, down, newMask);
	sendKeyEvent(target, down, false, id, newMask, 0, button);
}

bool
OSXKeyState::getGroups(GroupList& groups) const
{
	CFIndex n;
	bool gotLayouts = false;

#if defined(MAC_OS_X_VERSION_10_5)
	// get number of layouts
	CFStringRef keys[] = { kTISPropertyInputSourceCategory };
	CFStringRef values[] = { kTISCategoryKeyboardInputSource };
	CFDictionaryRef dict = CFDictionaryCreate(NULL, (const void **)keys, (const void **)values, 1, NULL, NULL);
	CFArrayRef kbds = TISCreateInputSourceList(dict, false);
	n = CFArrayGetCount(kbds);
	gotLayouts = (n != 0);
#else
	OSStatus status = KLGetKeyboardLayoutCount(&n);
	gotLayouts = (status == noErr);
#endif

	if (!gotLayouts) {
		LOG((CLOG_DEBUG1 "can't get keyboard layouts"));
		return false;
	}

	// get each layout
	groups.clear();
	for (CFIndex i = 0; i < n; ++i) {
		bool addToGroups = true;
#if defined(MAC_OS_X_VERSION_10_5)
		TISInputSourceRef keyboardLayout = 
			(TISInputSourceRef)CFArrayGetValueAtIndex(kbds, i);
#else
		KeyboardLayoutRef keyboardLayout;
		status = KLGetKeyboardLayoutAtIndex(i, &keyboardLayout);
		addToGroups == (status == noErr);
#endif
		if (addToGroups)
    		groups.push_back(keyboardLayout);
	}
	return true;
}

void
OSXKeyState::setGroup(SInt32 group)
{
#if defined(MAC_OS_X_VERSION_10_5)
	TISSetInputMethodKeyboardLayoutOverride(m_groups[group]);
#else
	KLSetCurrentKeyboardLayout(m_groups[group]);
#endif
}

void
OSXKeyState::checkKeyboardLayout()
{
	// XXX -- should call this when notified that groups have changed.
	// if no notification for that then we should poll.
	GroupList groups;
	if (getGroups(groups) && groups != m_groups) {
		updateKeyMap();
		updateKeyState();
	}
}

void
OSXKeyState::adjustAltGrModifier(const KeyIDs& ids,
				KeyModifierMask* mask, bool isCommand) const
{
	if (!isCommand) {
		for (KeyIDs::const_iterator i = ids.begin(); i != ids.end(); ++i) {
			KeyID id = *i;
			if (id != kKeyNone &&
				((id < 0xe000u || id > 0xefffu) ||
				(id >= kKeyKP_Equal && id <= kKeyKP_9))) {
				*mask |= KeyModifierAltGr;
				return;
			}
		}
	}
}

KeyButton
OSXKeyState::mapVirtualKeyToKeyButton(UInt32 keyCode)
{
	// 'A' maps to 0 so shift every id
	return static_cast<KeyButton>(keyCode + KeyButtonOffset);
}

UInt32
OSXKeyState::mapKeyButtonToVirtualKey(KeyButton keyButton)
{
	return static_cast<UInt32>(keyButton - KeyButtonOffset);
}


//
// OSXKeyState::KeyResource
//

KeyID
OSXKeyState::KeyResource::getKeyID(UInt8 c)
{
	if (c == 0) {
		return kKeyNone;
	}
	else if (c >= 32 && c < 127) {
		// ASCII
		return static_cast<KeyID>(c);
	}
	else {
		// handle special keys
		switch (c) {
		case 0x01:
			return kKeyHome;

		case 0x02:
			return kKeyKP_Enter;

		case 0x03:
			return kKeyKP_Enter;

		case 0x04:
			return kKeyEnd;

		case 0x05:
			return kKeyHelp;

		case 0x08:
			return kKeyBackSpace;

		case 0x09:
			return kKeyTab;

		case 0x0b:
			return kKeyPageUp;

		case 0x0c:
			return kKeyPageDown;

		case 0x0d:
			return kKeyReturn;

		case 0x10:
			// OS X maps all the function keys (F1, etc) to this one key.
			// we can't determine the right key here so we have to do it
			// some other way.
			return kKeyNone;

		case 0x1b:
			return kKeyEscape;

		case 0x1c:
			return kKeyLeft;

		case 0x1d:
			return kKeyRight;

		case 0x1e:
			return kKeyUp;

		case 0x1f:
			return kKeyDown;

		case 0x7f:
			return kKeyDelete;

		case 0x06:
		case 0x07:
		case 0x0a:
		case 0x0e:
		case 0x0f:
		case 0x11:
		case 0x12:
		case 0x13:
		case 0x14:
		case 0x15:
		case 0x16:
		case 0x17:
		case 0x18:
		case 0x19:
		case 0x1a:
			// discard other control characters
			return kKeyNone;

		default:
			// not special or unknown
			break;
		}

		// create string with character
		char str[2];
		str[0] = static_cast<char>(c);
		str[1] = 0;

#if defined(MAC_OS_X_VERSION_10_5)
		// get current keyboard script
		TISInputSourceRef isref = TISCopyCurrentKeyboardInputSource();
		CFArrayRef langs = (CFArrayRef) TISGetInputSourceProperty(isref, kTISPropertyInputSourceLanguages);
		CFStringEncoding encoding = CFStringConvertIANACharSetNameToEncoding((CFStringRef)CFArrayGetValueAtIndex(langs, 0));
#else
		CFStringEncoding encoding = GetScriptManagerVariable(smKeyScript);
#endif
		// convert to unicode
		CFStringRef cfString =
			CFStringCreateWithCStringNoCopy(
				kCFAllocatorDefault, str, encoding, kCFAllocatorNull);

		// sometimes CFStringCreate...() returns NULL (e.g. Apple Korean
		// encoding with char value 214).  if it did then make no key,
		// otherwise CFStringCreateMutableCopy() will crash.
		if (cfString == NULL) {
			return kKeyNone; 
		}

		// convert to precomposed
		CFMutableStringRef mcfString =
			CFStringCreateMutableCopy(kCFAllocatorDefault, 0, cfString);
		CFRelease(cfString);
		CFStringNormalize(mcfString, kCFStringNormalizationFormC);

		// check result
		int unicodeLength = CFStringGetLength(mcfString);
		if (unicodeLength == 0) {
			CFRelease(mcfString);
			return kKeyNone;
		}
		if (unicodeLength > 1) {
			// FIXME -- more than one character, we should handle this
			CFRelease(mcfString);
			return kKeyNone;
		}

		// get unicode character
		UniChar uc = CFStringGetCharacterAtIndex(mcfString, 0);
		CFRelease(mcfString);

		// convert to KeyID
		return static_cast<KeyID>(uc);
	}
}

KeyID
OSXKeyState::KeyResource::unicharToKeyID(UniChar c)
{
	switch (c) {
	case 3:
		return kKeyKP_Enter;

	case 8:
		return kKeyBackSpace;

	case 9:
		return kKeyTab;

	case 13:
		return kKeyReturn;

	case 27:
		return kKeyEscape;

	case 127:
		return kKeyDelete;

	default:
		if (c < 32) {
			return kKeyNone;
		}
		return static_cast<KeyID>(c);
	}
}


//
// OSXKeyState::CUCHRKeyResource
//

OSXKeyState::CUCHRKeyResource::CUCHRKeyResource(const void* resource,
				UInt32 keyboardType) :
	m_m(NULL),
	m_cti(NULL),
	m_sdi(NULL),
	m_sri(NULL),
	m_st(NULL)
{
	m_resource = reinterpret_cast<const UCKeyboardLayout*>(resource);
	if (m_resource == NULL) {
		return;
	}

	// find the keyboard info for the current keyboard type
	const UCKeyboardTypeHeader* th = NULL;
	const UCKeyboardLayout* r = m_resource;
	for (ItemCount i = 0; i < r->keyboardTypeCount; ++i) {
		if (keyboardType >= r->keyboardTypeList[i].keyboardTypeFirst &&
			keyboardType <= r->keyboardTypeList[i].keyboardTypeLast) {
			th = r->keyboardTypeList + i;
			break;
		}
		if (r->keyboardTypeList[i].keyboardTypeFirst == 0) {
			// found the default.  use it unless we find a match.
			th = r->keyboardTypeList + i;
		}
	}
	if (th == NULL) {
		// cannot find a suitable keyboard type
		return;
	}

	// get tables for keyboard type
	const UInt8* base = reinterpret_cast<const UInt8*>(m_resource);
	m_m   = reinterpret_cast<const UCKeyModifiersToTableNum*>(base +
								th->keyModifiersToTableNumOffset);
	m_cti = reinterpret_cast<const UCKeyToCharTableIndex*>(base +
								th->keyToCharTableIndexOffset);
	m_sdi = reinterpret_cast<const UCKeySequenceDataIndex*>(base +
								th->keySequenceDataIndexOffset);
	if (th->keyStateRecordsIndexOffset != 0) {
		m_sri = reinterpret_cast<const UCKeyStateRecordsIndex*>(base +
								th->keyStateRecordsIndexOffset);
	}
	if (th->keyStateTerminatorsOffset != 0) {
		m_st = reinterpret_cast<const UCKeyStateTerminators*>(base +
								th->keyStateTerminatorsOffset);
	}

	// find the space key, but only if it can combine with dead keys.
	// a dead key followed by a space yields the non-dead version of
	// the dead key.
	m_spaceOutput = 0xffffu;
	UInt32 table  = getTableForModifier(0);
	for (UInt32 button = 0, n = getNumButtons(); button < n; ++button) {
		KeyID id = getKey(table, button);
		if (id == 0x20) {
			UCKeyOutput c =
				reinterpret_cast<const UCKeyOutput*>(base +
								m_cti->keyToCharTableOffsets[table])[button];
			if ((c & kUCKeyOutputTestForIndexMask) ==
								kUCKeyOutputStateIndexMask) {
				m_spaceOutput = (c & kUCKeyOutputGetIndexMask);
				break;
			}
		}
	}
}

bool
OSXKeyState::CUCHRKeyResource::isValid() const
{
	return (m_m != NULL);
}

UInt32
OSXKeyState::CUCHRKeyResource::getNumModifierCombinations() const
{
	// only 32 (not 256) because the righthanded modifier bits are ignored
	return 32;
}

UInt32
OSXKeyState::CUCHRKeyResource::getNumTables() const
{
	return m_cti->keyToCharTableCount;
}

UInt32
OSXKeyState::CUCHRKeyResource::getNumButtons() const
{
	return m_cti->keyToCharTableSize;
}

UInt32
OSXKeyState::CUCHRKeyResource::getTableForModifier(UInt32 mask) const
{
	if (mask >= m_m->modifiersCount) {
		return m_m->defaultTableNum;
	}
	else {
		return m_m->tableNum[mask];
	}
}

KeyID
OSXKeyState::CUCHRKeyResource::getKey(UInt32 table, UInt32 button) const
{
	assert(table < getNumTables());
	assert(button < getNumButtons());

	const UInt8* base   = reinterpret_cast<const UInt8*>(m_resource);
	const UCKeyOutput* cPtr = reinterpret_cast<const UCKeyOutput*>(base +
								m_cti->keyToCharTableOffsets[table]);

  const UCKeyOutput c = cPtr[button];

	KeySequence keys;
	switch (c & kUCKeyOutputTestForIndexMask) {
	case kUCKeyOutputStateIndexMask:
		if (!getDeadKey(keys, c & kUCKeyOutputGetIndexMask)) {
			return kKeyNone;
		}
		break;

	case kUCKeyOutputSequenceIndexMask:
	default:
		if (!addSequence(keys, c)) {
			return kKeyNone;
		}
		break;
	}

	// XXX -- no support for multiple characters
	if (keys.size() != 1) {
		return kKeyNone;
	}

	return keys.front();
}

bool
OSXKeyState::CUCHRKeyResource::getDeadKey(
				KeySequence& keys, UInt16 index) const
{
	if (m_sri == NULL || index >= m_sri->keyStateRecordCount) {
		// XXX -- should we be using some other fallback?
		return false;
	}

	UInt16 state = 0;
	if (!getKeyRecord(keys, index, state)) {
		return false;
	}
	if (state == 0) {
		// not a dead key
		return true;
	}

	// no dead keys if we couldn't find the space key
	if (m_spaceOutput == 0xffffu) {
		return false;
	}

	// the dead key should not have put anything in the key list
	if (!keys.empty()) {
		return false;
	}

	// get the character generated by pressing the space key after the
	// dead key.  if we're still in a compose state afterwards then we're
	// confused so we bail.
	if (!getKeyRecord(keys, m_spaceOutput, state) || state != 0) {
		return false;
	}

	// convert keys to their dead counterparts
	for (KeySequence::iterator i = keys.begin(); i != keys.end(); ++i) {
		*i = synergy::KeyMap::getDeadKey(*i);
	}

	return true;
}

bool
OSXKeyState::CUCHRKeyResource::getKeyRecord(
				KeySequence& keys, UInt16 index, UInt16& state) const
{
	const UInt8* base = reinterpret_cast<const UInt8*>(m_resource);
	const UCKeyStateRecord* sr =
		reinterpret_cast<const UCKeyStateRecord*>(base +
								m_sri->keyStateRecordOffsets[index]);
	const UCKeyStateEntryTerminal* kset =
		reinterpret_cast<const UCKeyStateEntryTerminal*>(sr->stateEntryData);

	UInt16 nextState = 0;
	bool found       = false;
	if (state == 0) {
		found     = true;
		nextState = sr->stateZeroNextState;
		if (!addSequence(keys, sr->stateZeroCharData)) {
			return false;
		}
	}
	else {
		// we have a next entry
		switch (sr->stateEntryFormat) {
		case kUCKeyStateEntryTerminalFormat:
			for (UInt16 j = 0; j < sr->stateEntryCount; ++j) {
				if (kset[j].curState == state) {
					if (!addSequence(keys, kset[j].charData)) {
						return false;
					}
					nextState = 0;
					found     = true;
					break;
				}
			}
			break;

		case kUCKeyStateEntryRangeFormat:
			// XXX -- not supported yet
			break;

		default:
			// XXX -- unknown format
			return false;
		}
	}
	if (!found) {
		// use a terminator
		if (m_st != NULL && state < m_st->keyStateTerminatorCount) {
			if (!addSequence(keys, m_st->keyStateTerminators[state - 1])) {
				return false;
			}
		}
		nextState = sr->stateZeroNextState;
		if (!addSequence(keys, sr->stateZeroCharData)) {
			return false;
		}
	}

	// next
	state = nextState;

	return true;
}

bool
OSXKeyState::CUCHRKeyResource::addSequence(
				KeySequence& keys, UCKeyCharSeq c) const
{
	if ((c & kUCKeyOutputTestForIndexMask) == kUCKeyOutputSequenceIndexMask) {
		UInt16 index = (c & kUCKeyOutputGetIndexMask);
		if (index < m_sdi->charSequenceCount &&
			m_sdi->charSequenceOffsets[index] !=
				m_sdi->charSequenceOffsets[index + 1]) {
			// XXX -- sequences not supported yet
			return false;
		}
	}

	if (c != 0xfffe && c != 0xffff) {
		KeyID id = unicharToKeyID(c);
		if (id != kKeyNone) {
			keys.push_back(id);
		}
	}

	return true;
}
