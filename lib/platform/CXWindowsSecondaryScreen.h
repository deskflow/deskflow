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

#ifndef CXWINDOWSSECONDARYSCREEN_H
#define CXWINDOWSSECONDARYSCREEN_H

#include "CSecondaryScreen.h"
#include "IScreenEventHandler.h"
#include "stdbitset.h"
#include "stdmap.h"
#include "stdvector.h"
#if defined(X_DISPLAY_MISSING)
#	error X11 is required to build synergy
#else
#	include <X11/Xlib.h>
#endif

class CXWindowsScreen;
class IScreenReceiver;

//! X11 secondary screen implementation
class CXWindowsSecondaryScreen :
				public CSecondaryScreen, public IScreenEventHandler {
public:
	CXWindowsSecondaryScreen(IScreenReceiver*);
	virtual ~CXWindowsSecondaryScreen();

	// CSecondaryScreen overrides
	virtual void		keyDown(KeyID, KeyModifierMask, KeyButton);
	virtual void		keyRepeat(KeyID, KeyModifierMask,
							SInt32 count, KeyButton);
	virtual void		keyUp(KeyID, KeyModifierMask, KeyButton);
	virtual void		mouseDown(ButtonID);
	virtual void		mouseUp(ButtonID);
	virtual void		mouseMove(SInt32 x, SInt32 y);
	virtual void		mouseWheel(SInt32 delta);
	virtual void		resetOptions();
	virtual void		setOptions(const COptionsList& options);
	virtual IScreen*	getScreen() const;

	// IScreenEventHandler overrides
	virtual void		onScreensaver(bool activated);
	virtual bool		onPreDispatch(const CEvent* event);
	virtual bool		onEvent(CEvent* event);
	virtual void		onOneShotTimerExpired(UInt32 id);
	virtual SInt32		getJumpZoneSize() const;

protected:
	// CSecondaryScreen overrides
	virtual void		onPreMainLoop();
	virtual void		onPreOpen();
	virtual void		onPostOpen();
	virtual void		onPreClose();
	virtual void		onPreEnter();
	virtual void		onPostEnter();
	virtual void		onPreLeave();
	virtual void		createWindow();
	virtual void		destroyWindow();
	virtual void		showWindow(SInt32 x, SInt32 y);
	virtual void		hideWindow();
	virtual void		warpCursor(SInt32 x, SInt32 y);
	virtual void		updateKeys();
	virtual void		releaseKeys();
	virtual void		setToggleState(KeyModifierMask);
	virtual KeyModifierMask	getToggleState() const;

private:
	enum EKeyAction { kPress, kRelease, kRepeat };
	typedef unsigned int ModifierIndex;
	typedef unsigned int ModifierMask;
	class Keystroke {
	public:
		KeyCode			m_keycode;
		Bool			m_press;
		bool			m_repeat;
	};
	class KeyMapping {
	public:
		KeyMapping();

	public:
		// KeyCode to generate keysym and whether keycode[i] is
		// sensitive to shift and mode switch.
		KeyCode			m_keycode[4];
		bool			m_shiftSensitive[4];
		bool			m_modeSwitchSensitive[4];

		// the modifier mask of keysym or 0 if not a modifier
		ModifierMask	m_modifierMask;

		// whether keysym is sensitive to caps and num lock
		bool			m_numLockSensitive;
		bool			m_capsLockSensitive;
	};

	typedef std::vector<KeyCode> KeyCodes;
	typedef std::map<KeyCode, ModifierIndex> KeyCodeToModifierMap;
	typedef std::map<KeySym, KeyMapping> KeySymMap;
	typedef KeySymMap::const_iterator KeySymIndex;
	typedef std::vector<Keystroke> Keystrokes;
	typedef std::vector<KeySym> KeySyms;
	typedef std::map<KeySym, KeySyms> KeySymsMap;
	typedef std::map<KeyButton, KeyCode> ServerKeyMap;

	void				flush(Display*) const;

	unsigned int		mapButton(ButtonID button) const;

	ModifierMask		mapKey(Keystrokes&, KeyCode&, KeyID,
							KeyModifierMask, EKeyAction) const;
	ModifierMask		mapKeyRelease(Keystrokes&, KeyCode) const;
	bool				mapToKeystrokes(Keystrokes& keys,
							KeyCode& keycode,
							ModifierMask& finalMask,
							KeySymIndex keyIndex,
							ModifierMask currentMask,
							EKeyAction action) const;
	bool				adjustModifiers(Keystrokes& keys,
							Keystrokes& undo,
							ModifierMask& inOutMask,
							ModifierMask desiredMask) const;
	bool				adjustModifier(Keystrokes& keys,
							Keystrokes& undo,
							KeySym keysym,
							bool desireActive) const;
	void				doKeystrokes(const Keystrokes&, SInt32 count);
	ModifierMask		maskToX(KeyModifierMask) const;

	unsigned int		findBestKeyIndex(KeySymIndex keyIndex,
							ModifierMask currentMask) const;
	bool				isShiftInverted(KeySymIndex keyIndex,
							ModifierMask currentMask) const;
	ModifierMask		getModifierMask(KeySym) const;

	void				doUpdateKeys(Display*);
	void				doReleaseKeys(Display*);
	void				updateKeysymMap(Display* display);
	void				updateModifiers(Display* display);
	ModifierIndex		keySymToModifierIndex(KeySym) const;
	void				toggleKey(Display*, KeySym, ModifierMask mask);
	static bool			isToggleKeysym(KeySym);

	KeySym				keyIDToKeySym(KeyID id, ModifierMask mask) const;
	bool				adjustForNumLock(KeySym) const;
	bool				adjustForCapsLock(KeySym) const;

	bool				decomposeKeySym(KeySym keysym,
							KeySyms& decomposed) const;
	static const KeySymsMap&	getDecomposedKeySymTable();

private:
	CXWindowsScreen*	m_screen;
	Window				m_window;

	// note toggle keys that toggles on up/down (false) or on
	// transition (true)
	bool				m_numLockHalfDuplex;
	bool				m_capsLockHalfDuplex;

	// set entries indicate keys that are pressed (by us or by the user).
	// indexed by keycode.
	std::bitset<256>	m_keys;

	// set entries indicate keys that are synthetically pressed by us.
	// this is normally the same as m_keys.
	std::bitset<256>	m_fakeKeys;

	// logical to physical button mapping.  m_buttons[i] gives the
	// physical button for logical button i+1.
	std::vector<unsigned char>	m_buttons;

	// current active modifiers (X key masks)
	ModifierMask		m_mask;

	// the modifiers that have keys bound to them
	ModifierMask		m_modifierMask;

	// set bits indicate modifiers that toggle (e.g. caps-lock)
	ModifierMask		m_toggleModifierMask;

	// keysym to keycode mapping
	KeySymMap			m_keysymMap;

	// modifier index to keycodes
	KeyCodes			m_modifierKeycodes[8];

	// keycode to modifier index
	KeyCodeToModifierMap	m_keycodeToModifier;

	// modifier keysyms
	KeySym				m_shiftKeysym;
	KeySym				m_ctrlKeysym;
	KeySym				m_altKeysym;
	KeySym				m_metaKeysym;
	KeySym				m_superKeysym;
	KeySym				m_modeSwitchKeysym;
	KeySym				m_numLockKeysym;
	KeySym				m_capsLockKeysym;
	KeySym				m_scrollLockKeysym;

	// modifier masks
	ModifierMask		m_shiftMask;
	ModifierMask		m_ctrlMask;
	ModifierMask		m_altMask;
	ModifierMask		m_metaMask;
	ModifierMask		m_superMask;
	ModifierMask		m_modeSwitchMask;
	ModifierMask		m_numLockMask;
	ModifierMask		m_capsLockMask;
	ModifierMask		m_scrollLockMask;

	// map server key buttons to local keycodes
	ServerKeyMap		m_serverKeyMap;

	// the keyboard control state the last time this screen was entered
	XKeyboardState		m_keyControl;

	// stuff to workaround xtest being xinerama unaware.  attempting
	// to fake a mouse motion under xinerama may behave strangely,
	// especially if screen 0 is not at 0,0 or if faking a motion on
	// a screen other than screen 0.
	bool				m_xtestIsXineramaUnaware;
	bool				m_xinerama;

	// a table of keysym decompositions
	static KeySymsMap	s_decomposedKeySyms;
};

#endif
