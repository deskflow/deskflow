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

#ifndef CSECONDARYSCREEN_H
#define CSECONDARYSCREEN_H

#include "ClipboardTypes.h"
#include "KeyTypes.h"
#include "MouseTypes.h"
#include "OptionTypes.h"
#include "CMutex.h"
#include "stdmap.h"
#include "stdvector.h"

class IClipboard;
class IScreen;

//! Generic client-side screen
/*!
This is a platform independent base class for secondary screen
implementations.  A secondary screen is a client-side screen.
Each platform will derive a class from CSecondaryScreen to handle
platform dependent operations.
*/
class CSecondaryScreen {
public:
	CSecondaryScreen();
	virtual ~CSecondaryScreen();

	//! @name manipulators
	//@{

	//! Open screen
	/*!
	Opens the screen.  It also causes events to the reported to an
	IScreenReceiver (which is set through some other interface).
	Calls close() before returning (rethrowing) if it fails for any
	reason.
	*/
	void				open();

	//! Run event loop
	/*!
	Run the screen's event loop.  This returns when it detects
	the application should terminate or when exitMainLoop() is called.
	mainLoop() may only be called between open() and close().
	*/
	void				mainLoop();

	//! Exit event loop
	/*!
	Force mainLoop() to return.  This call can return before
	mainLoop() does (i.e. asynchronously).
	*/
	void				exitMainLoop();

	//! Prepare for remote control
	/*!
	Prepares the screen for remote control by the server.  In
	particular, it disables the screen saver.
	*/
	void				remoteControl();

	//! Release from remote control
	/*!
	Cleans up the screen from remote control by the server.  In
	particular, it enables the screen saver. It also synthesizes
	key up events for any keys that are logically down;  without
	this the client will leave its keyboard in the wrong logical
	state.
	*/
	void				localControl();

	//! Close screen
	/*!
	Closes the screen.
	*/
	void				close();

	//! Enter screen
	/*!
	Called when the user navigates to this secondary screen.  Warps
	the cursor to the absolute coordinates \c x,y and unhides
	it.  Also prepares to synthesize input events.
	*/
	void				enter(SInt32 x, SInt32 y, KeyModifierMask mask);

	//! Leave screen
	/*!
	Called when the user navigates off the secondary screen.  Cleans
	up input event synthesis and hides the cursor.
	*/
	void				leave();

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
	Reset all options to their default values.  Overrides should call
	the superclass's method.
	*/
	virtual void		resetOptions();

	//! Notify of options changes
	/*!
	Set options to given values.  Ignore unknown options and don't
	modify our options that aren't given in \c options.  Overrides
	should call the superclass's method.
	*/
	virtual void		setOptions(const COptionsList& options);

	//@}
	//! @name accessors
	//@{

	//! Test if active
	/*!
	Returns true iff the screen is active (i.e. the user has entered
	the screen).  Note this is the reverse of a primary screen.
	*/
	bool				isActive() const;

	//! Get clipboard
	/*!
	Saves the contents of the system clipboard indicated by \c id.
	*/
	void				getClipboard(ClipboardID id, IClipboard*) const;

	//! Get jump zone size
	/*!
	Return the jump zone size, the size of the regions on the edges of
	the screen that cause the cursor to jump to another screen.
	*/
	SInt32				getJumpZoneSize() const;

	//! Get screen shape
	/*!
	Return the position of the upper-left corner of the screen in \c x and
	\c y and the size of the screen in \c width and \c height.
	*/
	virtual void		getShape(SInt32& x, SInt32& y,
							SInt32& width, SInt32& height) const;

	//! Get cursor position
	/*!
	Return the current position of the cursor in \c x,y.
	*/
	virtual void		getCursorPos(SInt32& x, SInt32& y) const;

	//! Get screen
	/*!
	Return the platform dependent screen.
	*/
	virtual IScreen*	getScreen() const = 0;

	//@}

protected:
	typedef UInt8		KeyState;
	typedef UInt32		SysKeyID;
	enum EKeyState  { kDown = 0x01, kToggled = 0x80 };
	enum EKeyAction { kPress, kRelease, kRepeat };
	class Keystroke {
	public:
		SysKeyID		m_sysKeyID;
		bool			m_press;
		bool			m_repeat;
	};
	typedef std::vector<Keystroke> Keystrokes;
	typedef std::map<KeyButton, SysKeyID> ServerKeyMap;

	void				updateKeys();
	void				releaseKeys();
	void				doKeystrokes(const Keystrokes&, SInt32 count);
	bool				isKeyDown(SysKeyID) const;
	bool				isKeyToggled(SysKeyID) const;
	bool				isKeyHalfDuplex(KeyID) const;

	//! Pre-mainLoop() hook
	/*!
	Called on entry to mainLoop().  Override to perform platform specific
	operations.  Default does nothing.  May throw.
	*/
	virtual void		onPreMainLoop();

	//! Post-mainLoop() hook
	/*!
	Called on exit from mainLoop().  Override to perform platform specific
	operations.  Default does nothing.  May \b not throw.
	*/
	virtual void		onPostMainLoop();

	//! Pre-open() hook
	/*!
	Called on entry to open().  Override to perform platform specific
	operations.  Default does nothing.  May throw.
	*/
	virtual void		onPreOpen();

	//! Post-open() hook
	/*!
	Called on exit from open() iff the open was successful.  Default
	does nothing.  May throw.
	*/
	virtual void		onPostOpen();

	//! Pre-close() hook
	/*!
	Called on entry to close().  Override to perform platform specific
	operations.  Default does nothing.  May \b not throw.
	*/
	virtual void		onPreClose();

	//! Post-close() hook
	/*!
	Called on exit from close().  Override to perform platform specific
	operations.  Default does nothing.  May \b not throw.
	*/
	virtual void		onPostClose();

	//! Pre-enter() hook
	/*!
	Called on entry to enter() after desktop synchronization.  Override
	to perform platform specific operations.  Default does nothing.  May
	\b not throw.
	*/
	virtual void		onPreEnter();

	//! Post-enter() hook
	/*!
	Called on exit from enter().  Override to perform platform specific
	operations.  Default does nothing.  May \b not throw.
	*/
	virtual void		onPostEnter();

	//! Pre-leave() hook
	/*!
	Called on entry to leave() after desktop synchronization.  Override
	to perform platform specific operations.  Default does nothing.  May
	\b not throw.
	*/
	virtual void		onPreLeave();

	//! Post-leave() hook
	/*!
	Called on exit from leave().  Override to perform platform specific
	operations.  Default does nothing.  May \b not throw.
	*/
	virtual void		onPostLeave();

	//! Create window
	/*!
	Called to create the window.  This window is generally used to
	receive events and hide the cursor.
	*/
	virtual void		createWindow() = 0;

	//! Destroy window
	/*!
	Called to destroy the window created by createWindow().
	*/
	virtual void		destroyWindow() = 0;

	//! Show window
	/*!
	Called when the user navigates off this secondary screen.  It needn't
	actually show the window created by createWindow() but it must move
	the cursor to x,y, hide it, and clean up event synthesis.
	*/
	virtual void		showWindow(SInt32 x, SInt32 y) = 0;

	//! Hide window
	/*!
	Called when the user navigates to this secondary screen.  It should
	hide the window (if shown), show the cursor, and prepare to synthesize
	input events.
	*/
	virtual void		hideWindow() = 0;

	//! Synchronize key state
	/*!
	Save the current keyboard state.  Normally a screen will save
	the keyboard state in this method and use this shadow state,
	available through isKeyDown() and getKeyState(), when
	synthesizing events.
	*/
	virtual void		updateKeys(KeyState* sysKeyStates) = 0;

	//! Get modifier key state
	/*!
	Return the current keyboard modifier state. 
	*/
	virtual KeyModifierMask	getModifiers() const = 0;

	//! Synchronize toggle key state
	/*!
	Toggles modifiers that don't match the given state so that they do.
	*/
	void				setToggleState(KeyModifierMask);

	virtual SysKeyID	getUnhanded(SysKeyID) const;
	virtual SysKeyID	getOtherHanded(SysKeyID) const;
	virtual bool		isAutoRepeating(SysKeyID) const = 0;
	virtual KeyModifierMask	getModifierKeyMask(SysKeyID) const = 0;
	virtual bool		isModifierActive(SysKeyID) const = 0;
	virtual SysKeyID	getToggleSysKey(KeyID keyID) const = 0;
	virtual bool		synthesizeCtrlAltDel(EKeyAction);
	virtual void		sync() const;
	virtual void		flush();

	virtual KeyModifierMask
						mapKey(Keystrokes&, SysKeyID& sysKeyID, KeyID,
							KeyModifierMask currentMask,
							KeyModifierMask desiredMask, EKeyAction) const = 0;
	virtual void		fakeKeyEvent(SysKeyID, bool press) const = 0;
	virtual void		fakeMouseButton(ButtonID, bool press) const = 0;

	//! Warp cursor
	/*!
	Warp the cursor to the absolute coordinates \c x,y.
	*/
	virtual void		fakeMouseMove(SInt32 x, SInt32 y) const = 0;

	virtual void		fakeMouseWheel(SInt32 delta) const = 0;

private:
	void				toggleKey(KeyID, KeyModifierMask);

private:
	CMutex				m_mutex;

	// true if ready for remote control
	bool				m_remoteReady;

	// m_active is true if this screen has been entered
	bool				m_active;

	// true if screen saver should be synchronized to server
	bool				m_screenSaverSync;

	// map server key buttons to local system keys
	ServerKeyMap		m_serverKeyMap;

	// system key states as set by us or the user
	KeyState			m_keys[256];

	// system key states as set by us
	KeyState			m_fakeKeys[256];

	// current active modifiers
// XXX -- subclasses still have and use this
	KeyModifierMask		m_mask;

	// the toggle key state when this screen was last entered
	KeyModifierMask		m_toggleKeys;

	// note toggle keys that toggles on up/down (false) or on
	// transition (true)
	bool				m_numLockHalfDuplex;
	bool				m_capsLockHalfDuplex;
};

#endif
