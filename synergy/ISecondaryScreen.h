#ifndef ISECONDARYSCREEN_H
#define ISECONDARYSCREEN_H

#include "IInterface.h"
#include "BasicTypes.h"
#include "ClipboardTypes.h"
#include "KeyTypes.h"
#include "MouseTypes.h"

class CClient;
class IClipboard;

class ISecondaryScreen : public IInterface {
public:
	// manipulators

	// enter the screen's message loop.  this returns when it detects
	// the application should terminate or when stop() is called.
	// the screen must be open()'d before run() and must not be
	// close()'d until run() returns.
	virtual void		run() = 0;

	// cause run() to return
	virtual void		stop() = 0;

	// initialize the screen, hide the cursor, and disable the screen
	// saver.  start reporting certain events to the client (clipboard
	// stolen and screen size changed).
	virtual void		open(CClient*) = 0;

	// close the screen.  should restore the screen saver.
	virtual void		close() = 0;

	// called when the user navigates to the secondary screen.  warp
	// the cursor to the given coordinates and unhide it.  prepare to
	// simulate input events.
	virtual void		enter(SInt32 xAbsolute, SInt32 yAbsolute) = 0;

	// called when the user navigates off the secondary screen.  clean
	// up input event simulation and hide the cursor.
	virtual void		leave() = 0;

	// keyboard input simulation
	virtual void		keyDown(KeyID, KeyModifierMask) = 0;
	virtual void		keyRepeat(KeyID, KeyModifierMask, SInt32 count) = 0;
	virtual void		keyUp(KeyID, KeyModifierMask) = 0;

	// mouse input simulation
	virtual void		mouseDown(ButtonID) = 0;
	virtual void		mouseUp(ButtonID) = 0;
	virtual void		mouseMove(SInt32 xAbsolute, SInt32 yAbsolute) = 0;
	virtual void		mouseWheel(SInt32 delta) = 0;

	// set the screen's clipboard contents.  this is usually called
	// soon after an enter().
	virtual void		setClipboard(ClipboardID, const IClipboard*) = 0;

/*
	// show or hide the screen saver
	virtual void		screenSaver(bool show) = 0;
*/

	// take ownership of clipboard
	virtual void		grabClipboard(ClipboardID) = 0;

	// accessors

	// get the size of the screen
	virtual void		getSize(SInt32* width, SInt32* height) const = 0;

	// get the size of jump zone
	virtual SInt32		getJumpZoneSize() const = 0;

	// get the screen's clipboard contents
	virtual void		getClipboard(ClipboardID, IClipboard*) const = 0;
};

#endif
