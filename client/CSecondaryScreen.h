#ifndef CSECONDARYSCREEN_H
#define CSECONDARYSCREEN_H

#include "ClipboardTypes.h"
#include "KeyTypes.h"
#include "MouseTypes.h"
#include "CMutex.h"

class IClipboard;
class IScreen;

// platform independent base class for secondary screen implementations.
// each platform will derive a class from CSecondaryScreen to handle
// platform dependent operations.
class CSecondaryScreen {
public:
	CSecondaryScreen();
	virtual ~CSecondaryScreen();

	// manipulators

	// enter the screen's message loop.  this returns when it detects
	// the application should terminate or when stop() is called.
	// run() may only be called between open() and close().
	void				run();

	// cause run() to return
	void				stop();

	// initialize the screen, hide the cursor, and disable the screen
	// saver.  start reporting events to the IScreenReceiver (which is
	// set through some other interface).
	void				open();

	// close the screen.  should restore the screen saver.  it should
	// also simulate key up events for any keys that have simulate key
	// down events without a matching key up.  without this the client
	// will leave its keyboard in the wrong logical state.
	void				close();

	// called when the user navigates to this secondary screen.  warps
	// the cursor to the given absoltue coordinates and unhide it.  prepare to
	// simulate input events.
	void				enter(SInt32 x, SInt32 y, KeyModifierMask mask);

	// called when the user navigates off the secondary screen.  clean
	// up input event simulation and hide the cursor.
	void				leave();

	// set the screen's clipboard contents.  this is usually called
	// soon after an enter().
	void				setClipboard(ClipboardID, const IClipboard*);

	// synergy should own the clipboard
	void				grabClipboard(ClipboardID);

	// activate or deactivate the screen saver
	void				screensaver(bool activate);

	// keyboard input event synthesis
	virtual void		keyDown(KeyID, KeyModifierMask) = 0;
	virtual void		keyRepeat(KeyID, KeyModifierMask, SInt32 count) = 0;
	virtual void		keyUp(KeyID, KeyModifierMask) = 0;

	// mouse input event synthesis
	virtual void		mouseDown(ButtonID) = 0;
	virtual void		mouseUp(ButtonID) = 0;
	virtual void		mouseMove(SInt32 xAbsolute, SInt32 yAbsolute) = 0;
	virtual void		mouseWheel(SInt32 delta) = 0;

	// accessors

	// returns true iff the screen is active (i.e. the user has entered
	// the screen)
	bool				isActive() const;

	// return the contents of the given clipboard
	void				getClipboard(ClipboardID, IClipboard*) const;

	// returns the size of the zone on the edges of the screen that
	// causes the cursor to jump to another screen.  default returns 0.
	virtual SInt32		getJumpZoneSize() const;

	// get the shape (position of upper-left corner and size) of the
	// screen
	virtual void		getShape(SInt32& x, SInt32& y,
							SInt32& width, SInt32& height) const;

	// get the position of the mouse on the screen
	virtual void		getCursorPos(SInt32& x, SInt32& y) const;

	// get the platform dependent screen object
	virtual IScreen*	getScreen() const = 0;

protected:
	// template method hooks.  these are called on entry/exit to the
	// named method.  override to do platform specific operations.
	// defaults do nothing.
	virtual void		onPreRun();
	virtual void		onPostRun();
	virtual void		onPreOpen();
	virtual void		onPostOpen();
	virtual void		onPreClose();
	virtual void		onPostClose();
	virtual void		onPreEnter();
	virtual void		onPostEnter();
	virtual void		onPreLeave();
	virtual void		onPostLeave();

	// create/destroy the window.  this window is generally used to
	// receive events and hide the cursor.
	virtual void		createWindow() = 0;
	virtual void		destroyWindow() = 0;

	// called when the user navigates off the secondary screen.  hide
	// the cursor.
	virtual void		showWindow() = 0;

	// called when the user navigates to this secondary screen.  show
	// the cursor and prepare to synthesize input events.
	virtual void		hideWindow() = 0;

	// warp the cursor to the given absolute coordinates
	virtual void		warpCursor(SInt32 x, SInt32 y) = 0;

	// check the current keyboard state.  normally a screen will save
	// the keyboard state in this method and use this shadow state
	// when synthesizing events.
	virtual void		updateKeys() = 0;

	// toggle modifiers that don't match the given state
	virtual void		setToggleState(KeyModifierMask) = 0;

private:
	CMutex				m_mutex;

	// m_active is true if this screen has been entered
	bool				m_active;
};

#endif
