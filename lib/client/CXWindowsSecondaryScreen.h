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
	virtual void		keyDown(KeyID, KeyModifierMask);
	virtual void		keyRepeat(KeyID, KeyModifierMask, SInt32 count);
	virtual void		keyUp(KeyID, KeyModifierMask);
	virtual void		mouseDown(ButtonID);
	virtual void		mouseUp(ButtonID);
	virtual void		mouseMove(SInt32 x, SInt32 y);
	virtual void		mouseWheel(SInt32 delta);
	virtual IScreen*	getScreen() const;

	// IScreenEventHandler overrides
	virtual void		onScreensaver(bool activated);
	virtual bool		onPreDispatch(const CEvent* event);
	virtual bool		onEvent(CEvent* event);
	virtual SInt32		getJumpZoneSize() const;

protected:
	// CSecondaryScreen overrides
	virtual void		onPreMainLoop();
	virtual void		onPreOpen();
	virtual void		onPostOpen();
	virtual void		onPreEnter();
	virtual void		onPreLeave();
	virtual void		createWindow();
	virtual void		destroyWindow();
	virtual void		showWindow();
	virtual void		hideWindow();
	virtual void		warpCursor(SInt32 x, SInt32 y);
	virtual void		updateKeys();
	virtual void		setToggleState(KeyModifierMask);
	virtual KeyModifierMask	getToggleState() const;

private:
	enum EKeyAction { kPress, kRelease, kRepeat };
	class KeyCodeMask {
	public:
		KeyCodeMask();
	public:
		KeyCode			m_keycode[4];
		// FIXME -- don't need masks
		unsigned int	m_keyMask[4];
		unsigned int	m_keyMaskMask[4];
	};
	class Keystroke {
	public:
		KeyCode			m_keycode;
		Bool			m_press;
		bool			m_repeat;
	};
	typedef std::vector<Keystroke> Keystrokes;
	typedef std::vector<KeyCode> KeyCodes;
	typedef std::map<KeySym, KeyCodeMask> KeyCodeMap;
	typedef KeyCodeMap::const_iterator KeyCodeIndex;
	typedef std::map<KeyCode, unsigned int> ModifierMap;

	unsigned int		mapButton(ButtonID button) const;

	unsigned int		mapKey(Keystrokes&, KeyCode&, KeyID,
							KeyModifierMask, EKeyAction) const;
/*
	bool				findKeyCode(KeyCode&, unsigned int&,
							KeyID id, unsigned int) const;
*/
	void				doKeystrokes(const Keystrokes&, SInt32 count);
	unsigned int		maskToX(KeyModifierMask) const;

	void				releaseKeys(Display*);
	void				updateKeycodeMap(Display* display);
	void				updateModifiers(Display* display);
	void				updateModifierMap(Display* display);
	unsigned int		indexToModifierMask(int index) const;
	void				toggleKey(Display*, KeySym, unsigned int mask);
	static bool			isToggleKeysym(KeySym);

	KeyCodeIndex		findKey(KeyID keysym, KeyModifierMask& mask) const;
	KeyCodeIndex		noKey() const;
	bool				adjustForNumLock(KeySym) const;
	bool				adjustForCapsLock(KeySym) const;

private:
	enum { kNONE, kSHIFT, kALTGR, kSHIFT_ALTGR };

	CXWindowsScreen*	m_screen;
	Window				m_window;

	// note toggle keys that toggles on up/down (false) or on
	// transition (true)
	bool				m_numLockHalfDuplex;
	bool				m_capsLockHalfDuplex;

	// set entries indicate keys that are pressed.  indexed by keycode.
	bool				m_keys[256];

	// logical to physical button mapping.  m_buttons[i] gives the
	// physical button for logical button i+1.
	unsigned char		m_buttons[5];

	// current active modifiers (X key masks)
	unsigned int		m_mask;

	// maps key IDs to X keycodes and the X modifier key mask needed
	// to generate the right keysym
	KeyCodeMap			m_keycodeMap;

	// the modifiers that have keys bound to them
	unsigned int		m_modifierMask;

	// set bits indicate modifiers that toggle (e.g. caps-lock)
	unsigned int		m_toggleModifierMask;

	// modifier masks
	unsigned int		m_altMask;
	unsigned int		m_metaMask;
	unsigned int		m_superMask;
	unsigned int		m_modeSwitchMask;
	unsigned int		m_numLockMask;
	unsigned int		m_capsLockMask;
	unsigned int		m_scrollLockMask;

	// map X modifier key indices to the key codes bound to them
	unsigned int		m_keysPerModifier;
	KeyCodes			m_modifierToKeycode;
	KeyCodes			m_modifierToKeycodes;

	// maps keycodes to modifier indices
	ModifierMap			m_keycodeToModifier;
};

#endif
