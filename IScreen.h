#ifndef ISCREEN_H
#define ISCREEN_H

/*
 * IScreen -- interface for display screens
 *
 * a screen encapsulates input and output devices, typically a mouse
 * and keyboard for input and a graphical display for output.  one
 * screen is designated as the primary screen.  only input from the
 * primary screen's input devices is used.  other screens are secondary
 * screens and they simulate input from their input devices but ignore
 * any actual input.  a screen can be either a primary or a secondary
 * but not both at the same time.  most methods behave differently
 * depending on the screen type.
 */

#include "BasicTypes.h"
#include "KeyTypes.h"
#include "MouseTypes.h"
#include "CString.h"

class IClipboard;

class IScreen {
  public:
	IScreen() { }
	virtual ~IScreen() { }

	// manipulators

	// open/close screen.  these are where the client should do
	// initialization and cleanup of the system's screen.  if isPrimary
	// is true then this screen will be used (exclusively) as the
	// primary screen, otherwise it will be used (exclusively) as a
	// secondary screen.
	//
	// primary:
	//   open():  open the screen and begin reporting input events to
	// the event queue.  input events should be reported no matter
	// where on the screen they occur but the screen should not
	// interfere with the normal dispatching of events.  the screen
	// should detect when the screen saver is activated.  if it can't
	// do that it should disable the screen saver and start it itself
	// after the appropriate duration of no input.
	//
	// secondary:
	//   open():  open the screen, hide the cursor and disable the
	// screen saver.  then wait for an enterScreen() or close(),
	// reporting the following events: FIXME.
	virtual void		open(bool isPrimary) = 0;
	virtual void		close() = 0;

	// enter/leave screen
	//
	// primary:
	//   enterScreen():  the user has navigated back to the primary
	// screen.  warp the cursor to the given coordinates, unhide the
	// cursor and ungrab the input devices.  the screen must also
	// detect and report (enqueue) input events.  for the primary
	// screen, enterScreen() is only called after a leaveScreen().
	//   leaveScreen():  the user has navigated off the primary screen.
	// hide the cursor and grab exclusive access to the input devices.
	// input events must be reported.
	//
	// secondary:
	//   enterScreen():  the user has navigated to this secondary
	// screen.  warp the cursor to the given coordinates and show it.
	// prepare to simulate input events.
	//   leaveScreen():  the user has navigated off this secondary
	// screen.  clean up input event simulation.  hide the cursor.
	virtual void		enterScreen(SInt32 xAbsolute, SInt32 yAbsolute) = 0;
	virtual void		leaveScreen() = 0;

	// warp the cursor to the given position
	virtual void		warpCursor(SInt32 xAbsolute, SInt32 yAbsolute) = 0;

	//
	// clipboard operations
	//

	// set the screen's clipboard contents.  this is usually called
	// soon after an enterScreen().
	virtual void		setClipboard(const IClipboard*) = 0;

	//
	// screen saver operations
	//

	// show or hide the screen saver
	virtual void		onScreenSaver(bool show) = 0;

	//
	// input simulation
	//
	// these methods must simulate the appropriate input event.
	// these methods are only called on secondary screens.
	//

	// keyboard input
	virtual void		onKeyDown(KeyID, KeyModifierMask) = 0;
	virtual void		onKeyRepeat(KeyID, KeyModifierMask, SInt32 count) = 0;
	virtual void		onKeyUp(KeyID, KeyModifierMask) = 0;

	// mouse input
	virtual void		onMouseDown(ButtonID) = 0;
	virtual void		onMouseUp(ButtonID) = 0;
	virtual void		onMouseMove(SInt32 xAbsolute, SInt32 yAbsolute) = 0;
	virtual void		onMouseWheel(SInt32 delta) = 0;

	// clipboard input
	// FIXME -- do we need this?
	virtual void		onClipboardChanged() = 0;

	// accessors

	// get the screen's name.  all screens must have a name unique on
	// the server they connect to.  the hostname is usually an
	// appropriate name.
	virtual CString		getName() const = 0;

	// get the size of the screen
	virtual void		getSize(SInt32* width, SInt32* height) const = 0;

	// clipboard operations

	// get the screen's clipboard contents
	virtual void		getClipboard(IClipboard*) const = 0;
};

#endif
