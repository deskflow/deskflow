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
	virtual void		resetOptions();
	virtual void		setOptions(const COptionsList& options);
	virtual IScreen*	getScreen() const;

	// IScreenEventHandler overrides
	virtual void		onScreensaver(bool activated);
	virtual bool		onPreDispatch(const CEvent* event);
	virtual bool		onEvent(CEvent* event);
	virtual void		onOneShotTimerExpired(UInt32 id);

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
	virtual void		updateKeys(KeyState* sysKeyStates);
	virtual KeyModifierMask	getModifiers() const;

	virtual bool		isAutoRepeating(SysKeyID) const;
	virtual KeyModifierMask	getModifierKeyMask(SysKeyID) const;
	virtual bool		isModifierActive(SysKeyID) const;
	virtual SysKeyID	getToggleSysKey(KeyID keyID) const;
	virtual void		flush();
	virtual KeyModifierMask
						mapKey(Keystrokes&, SysKeyID& sysKeyID, KeyID,
							KeyModifierMask, KeyModifierMask, EKeyAction) const;
	virtual void		fakeKeyEvent(SysKeyID, bool press) const;
	virtual void		fakeMouseButton(ButtonID, bool press) const;
	virtual void		fakeMouseMove(SInt32 x, SInt32 y) const;
	virtual void		fakeMouseWheel(SInt32 delta) const;

private:
	typedef unsigned int ModifierIndex;
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
		KeyModifierMask	m_modifierMask;

		// whether keysym is sensitive to caps and num lock
		bool			m_numLockSensitive;
		bool			m_capsLockSensitive;
	};

	typedef std::vector<KeyCode> KeyCodes;
	typedef std::map<KeyCode, ModifierIndex> KeyCodeToModifierMap;
	typedef std::map<KeySym, KeyMapping> KeySymMap;
	typedef KeySymMap::const_iterator KeySymIndex;
	typedef std::vector<KeySym> KeySyms;
	typedef std::map<KeySym, KeySyms> KeySymsMap;

	unsigned int		mapButton(ButtonID button) const;

	bool				mapToKeystrokes(Keystrokes& keys,
							SysKeyID& keycode,
							KeyModifierMask& finalMask,
							KeySymIndex keyIndex,
							KeyModifierMask currentMask,
							EKeyAction action,
							bool isHalfDuplex) const;
	bool				adjustModifiers(Keystrokes& keys,
							Keystrokes& undo,
							KeyModifierMask& inOutMask,
							KeyModifierMask desiredMask) const;
	bool				adjustModifier(Keystrokes& keys,
							Keystrokes& undo,
							KeySym keysym,
							bool desireActive) const;
	KeyModifierMask		mapToModifierMask(ModifierIndex, KeySym) const;

	unsigned int		findBestKeyIndex(KeySymIndex keyIndex,
							KeyModifierMask currentMask) const;
	bool				isShiftInverted(KeySymIndex keyIndex,
							KeyModifierMask currentMask) const;

	void				doUpdateKeys(Display*);
	void				updateKeysymMap(Display* display);
	void				updateModifiers(Display* display);
	ModifierIndex		keySymToModifierIndex(KeySym) const;
	static bool			isToggleKeysym(KeySym);

	KeySym				keyIDToKeySym(KeyID id, KeyModifierMask mask) const;
	bool				adjustForNumLock(KeySym) const;
	bool				adjustForCapsLock(KeySym) const;

	bool				decomposeKeySym(KeySym keysym,
							KeySyms& decomposed) const;
	static const KeySymsMap&	getDecomposedKeySymTable();

private:
	CXWindowsScreen*	m_screen;
	Window				m_window;

	// logical to physical button mapping.  m_buttons[i] gives the
	// physical button for logical button i+1.
	std::vector<unsigned char>	m_buttons;

	// the modifiers that have keys bound to them
	KeyModifierMask		m_modifierMask;

	// set bits indicate modifiers that toggle (e.g. caps-lock)
	KeyModifierMask		m_toggleModifierMask;

	// keysym to keycode mapping
	KeySymMap			m_keysymMap;

	// modifier index to keycodes
	KeyCodes			m_modifierKeycodes[8];

	// modifier index to modifier mask
	KeyModifierMask		m_modifierIndexToMask[8];

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
