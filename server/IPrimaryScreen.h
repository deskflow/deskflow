#ifndef IPRIMARYSCREEN_H
#define IPRIMARYSCREEN_H

#include "IInterface.h"
#include "KeyTypes.h"
#include "ClipboardTypes.h"

class IClipboard;

class IPrimaryScreen : public IInterface {
public:
	// manipulators

	// enter the screen's message loop.  this returns when it detects
	// the application should terminate or when stop() is called.
	// the screen must be open()'d before run() and must not be
	// close()'d until run() returns.
	virtual void		run() = 0;

	// cause run() to return
	virtual void		stop() = 0;

	// initialize the screen and start reporting events to the receiver
	// (which is set through some interface of the derived class).
	// events should be reported no matter where on the screen they
	// occur but do not interfere with normal event dispatch.  the
	// screen saver engaging should be reported as an event.  if that
	// can't be detected then this object should disable the system's
	// screen saver timer and should start the screen saver after
	// idling for an appropriate time.
	//
	// open() must call receiver->onInfoChanged() to notify of the
	// primary screen's initial resolution and jump zone size.  it
	// must also call receiver->onClipboardChanged() for each
	// clipboard that the primary screen has.
	virtual void		open() = 0;

	// close the screen.  should restore the screen saver timer if it
	// was disabled.
	virtual void		close() = 0;

	// called when the user navigates back to the primary screen.
	// warp the cursor to the given coordinates, unhide it, and
	// ungrab the input devices.  every call to enter has a matching
	// call to leave() which preceeds it, however the screen should
	// assume an implicit call to enter() in the call to open().
	// if warpCursor is false then do not warp the mouse.
	//
	// enter() must not call any receiver methods except onError().
	virtual void		enter(SInt32 xAbsolute, SInt32 yAbsolute,
							bool forScreenSaver) = 0;

	// called when the user navigates off the primary screen.  hide the
	// cursor and grab exclusive access to the input devices.  return
	// true iff successful.
	//
	// leave() must not call any receiver methods except onError().
	virtual bool		leave() = 0;

	// called when the configuration has changed.  subclasses may need
	// to adjust things (like the jump zones) after the configuration
	// changes.
	virtual void		reconfigure() = 0;

	// warp the cursor to the given position
	virtual void		warpCursor(SInt32 xAbsolute, SInt32 yAbsolute) = 0;

	// set the screen's clipboard contents.  this is usually called
	// soon after an enter().
	//
	// setClipboard() must not call any receiver methods except onError().
	virtual void		setClipboard(ClipboardID, const IClipboard*) = 0;

	// synergy should own the clipboard
	virtual void		grabClipboard(ClipboardID) = 0;

	// accessors

	// return the contents of the given clipboard.
	//
	// getClipboard() must not call any receiver methods except onError().
	virtual void		getClipboard(ClipboardID, IClipboard*) const = 0;

	// get the primary screen's current toggle modifier key state.
	// the returned mask should have the corresponding bit set for
	// each toggle key that is active.
	virtual KeyModifierMask	getToggleMask() const = 0;

	// return true if any key or button is being pressed or if there's
	// any other reason that the user should not be allowed to switch
	// screens.
	virtual bool		isLockedToScreen() const = 0;
};

#endif
