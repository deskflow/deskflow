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

#ifndef CSCREEN_H
#define CSCREEN_H

#include "IKeyState.h"
#include "IScreen.h"
#include "ClipboardTypes.h"
#include "MouseTypes.h"
#include "OptionTypes.h"
#include "stdmap.h"

class IClipboard;
class IPlatformScreen;

//! Platform independent screen
/*!
This is a platform independent screen.  It can work as either a
primary or secondary screen.
*/
class CScreen : public IScreen, public IKeyState {
public:
	CScreen(IPlatformScreen* platformScreen);
	virtual ~CScreen();

	//! @name manipulators
	//@{

	//! Activate screen
	/*!
	Activate the screen, preparing it to report system and user events.
	For a secondary screen it also means disabling the screen saver if
	synchronizing it and preparing to synthesize events.
	*/
	void				enable();

	//! Deactivate screen
	/*!
	Undoes the operations in activate() and events are no longer
	reported.  It also releases keys that are logically pressed.
	*/
	void				disable();

	//! Enter screen
	/*!
	Called when the user navigates to this screen.
	*/
	void				enter();

	//! Leave screen
	/*!
	Called when the user navigates off this screen.
	*/
	bool				leave();

	//! Update configuration
	/*!
	This is called when the configuration has changed.  \c activeSides
	is a bitmask of EDirectionMask indicating which sides of the
	primary screen are linked to clients.
	*/
	void				reconfigure(UInt32 activeSides);

	//! Warp cursor
	/*!
	Warps the cursor to the absolute coordinates \c x,y.  Also
	discards input events up to and including the warp before
	returning.
	*/
	void				warpCursor(SInt32 x, SInt32 y);

	//! Set clipboard
	/*!
	Sets the system's clipboard contents.  This is usually called
	soon after an enter().
	*/
	void				setClipboard(ClipboardID, const IClipboard*);

	//! Grab clipboard
	/*!
	Grabs (i.e. take ownership of) the system clipboard.
	*/
	void				grabClipboard(ClipboardID);

	//! Activate/deactivate screen saver
	/*!
	Forcibly activates the screen saver if \c activate is true otherwise
	forcibly deactivates it.
	*/
	void				screensaver(bool activate);

	//! Notify of key press
	/*!
	Synthesize key events to generate a press of key \c id.  If possible
	match the given modifier mask.  The KeyButton identifies the physical
	key on the server that generated this key down.  The client must
	ensure that a key up or key repeat that uses the same KeyButton will
	synthesize an up or repeat for the same client key synthesized by
	keyDown().
	*/
	void				keyDown(KeyID id, KeyModifierMask, KeyButton);

	//! Notify of key repeat
	/*!
	Synthesize key events to generate a press and release of key \c id
	\c count times.  If possible match the given modifier mask.
	*/
	void				keyRepeat(KeyID id, KeyModifierMask,
							SInt32 count, KeyButton);

	//! Notify of key release
	/*!
	Synthesize key events to generate a release of key \c id.  If possible
	match the given modifier mask.
	*/
	void				keyUp(KeyID id, KeyModifierMask, KeyButton);

	//! Notify of mouse press
	/*!
	Synthesize mouse events to generate a press of mouse button \c id.
	*/
	void				mouseDown(ButtonID id);

	//! Notify of mouse release
	/*!
	Synthesize mouse events to generate a release of mouse button \c id.
	*/
	void				mouseUp(ButtonID id);

	//! Notify of mouse motion
	/*!
	Synthesize mouse events to generate mouse motion to the absolute
	screen position \c xAbs,yAbs.
	*/
	void				mouseMove(SInt32 xAbs, SInt32 yAbs);

	//! Notify of mouse wheel motion
	/*!
	Synthesize mouse events to generate mouse wheel motion of \c delta.
	\c delta is positive for motion away from the user and negative for
	motion towards the user.  Each wheel click should generate a delta
	of +/-120.
	*/
	void				mouseWheel(SInt32 delta);

	//! Notify of options changes
	/*!
	Resets all options to their default values.
	*/
	void				resetOptions();

	//! Notify of options changes
	/*!
	Set options to given values.  Ignores unknown options and doesn't
	modify options that aren't given in \c options.
	*/
	void				setOptions(const COptionsList& options);

	//! Set clipboard sequence number
	/*!
	Sets the sequence number to use in subsequent clipboard events.
	*/
	void				setSequenceNumber(UInt32);

	//@}
	//! @name accessors
	//@{

	//! Test if cursor on screen
	/*!
	Returns true iff the cursor is on the screen.
	*/
	bool				isOnScreen() const;

	//! Get screen lock state
	/*!
	Returns true if there's any reason that the user should not be
	allowed to leave the screen (usually because a button or key is
	pressed).  If this method returns true it logs a message as to
	why at the CLOG_DEBUG level.
	*/
	bool				isLockedToScreen() const;

	//! Get jump zone size
	/*!
	Return the jump zone size, the size of the regions on the edges of
	the screen that cause the cursor to jump to another screen.
	*/
	SInt32				getJumpZoneSize() const;

	//! Get cursor center position
	/*!
	Return the cursor center position which is where we park the
	cursor to compute cursor motion deltas and should be far from
	the edges of the screen, typically the center.
	*/
	void				getCursorCenter(SInt32& x, SInt32& y) const;

	//@}

	// IScreen overrides
	virtual void*		getEventTarget() const;
	virtual bool		getClipboard(ClipboardID id, IClipboard*) const;
	virtual void		getShape(SInt32& x, SInt32& y,
							SInt32& width, SInt32& height) const;
	virtual void		getCursorPos(SInt32& x, SInt32& y) const;

	// IKeyState overrides
	virtual void		updateKeys();
	virtual void		releaseKeys();
	virtual void		setKeyDown(KeyButton key, bool);
	virtual void		setToggled(KeyModifierMask);
	virtual void		addModifier(KeyModifierMask, KeyButtons&);
	virtual void		setToggleState(KeyModifierMask);
	virtual KeyButton	isAnyKeyDown() const;
	virtual bool		isKeyDown(KeyButton) const;
	virtual bool		isToggle(KeyModifierMask) const;
	virtual bool		isHalfDuplex(KeyModifierMask) const;
	virtual bool		isModifierActive(KeyModifierMask) const;
	virtual KeyModifierMask
						getActiveModifiers() const;
	virtual bool		mapModifier(Keystrokes& keys, Keystrokes& undo,
							KeyModifierMask mask, bool desireActive) const;
	virtual KeyModifierMask
						getMaskForKey(KeyButton) const;

protected:
	void				enablePrimary();
	void				enableSecondary();
	void				disablePrimary();
	void				disableSecondary();

	void				enterPrimary();
	void				enterSecondary();
	void				leavePrimary();
	void				leaveSecondary();

private:
	// Get the modifier mask for the current key state
	KeyModifierMask		getModifierMask() const;

	// Send fake keystrokes
	void				doKeystrokes(const Keystrokes&, SInt32 count);

	// Send a fake key event
	void				fakeKeyEvent(KeyButton, bool press, bool repeat) const;

	// Update the shadow state for a key
	void				updateKeyState(KeyButton button,
							KeyButton key, bool press);

	// Toggle a modifier
	void				toggleKey(KeyModifierMask);

	// Test if a modifier is toggled
	bool				isKeyToggled(KeyButton) const;

private:
	typedef std::map<KeyButton, KeyButton>			ServerKeyMap;
	typedef std::map<KeyModifierMask, KeyButtons>	MaskToKeys;
	typedef std::map<KeyButton, KeyModifierMask>	KeyToMask;

	// our platform dependent screen
	IPlatformScreen*	m_screen;

	// true if screen is being used as a primary screen, false otherwise
	bool				m_isPrimary;

	// true if screen is enabled
	bool				m_enabled;

	// true if the cursor is on this screen
	bool				m_entered;

	// true if screen saver should be synchronized to server
	bool				m_screenSaverSync;

	// note toggle keys that toggles on up/down (false) or on
	// transition (true)
	bool				m_numLockHalfDuplex;
	bool				m_capsLockHalfDuplex;

	// keyboard state

	// map server key buttons to local system keys
	ServerKeyMap		m_serverKeyMap;

	// system key states as set by us or the user
	KeyState			m_keys[512];

	// system key states as set by us
	KeyState			m_fakeKeys[512];

	// modifier info
	MaskToKeys			m_maskToKeys;
	KeyToMask			m_keyToMask;

	// current active modifiers
	KeyModifierMask		m_mask;

	// the toggle key state when this screen was last entered
	KeyModifierMask		m_toggleKeys;
};

#endif
