#ifndef ISECONDARYSCREEN_H
#define ISECONDARYSCREEN_H

#include "IInterface.h"
#include "BasicTypes.h"

class CClient;
//class IClipboard;

class ISecondaryScreen : public IInterface {
  public:
	// manipulators

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

	// warp the cursor to the given position
	virtual void		warpCursor(SInt32 xAbsolute, SInt32 yAbsolute) = 0;

	// keyboard input simulation
	virtual void		onKeyDown(KeyID, KeyModifierMask) = 0;
	virtual void		onKeyRepeat(KeyID, KeyModifierMask, SInt32 count) = 0;
	virtual void		onKeyUp(KeyID, KeyModifierMask) = 0;

	// mouse input simulation
	virtual void		onMouseDown(ButtonID) = 0;
	virtual void		onMouseUp(ButtonID) = 0;
	virtual void		onMouseMove(SInt32 xAbsolute, SInt32 yAbsolute) = 0;
	virtual void		onMouseWheel(SInt32 delta) = 0;

/*
	// set the screen's clipboard contents.  this is usually called
	// soon after an enter().
	virtual void		setClipboard(const IClipboard*) = 0;

	// show or hide the screen saver
	virtual void		onScreenSaver(bool show) = 0;

	// clipboard input
	virtual void		onClipboardChanged() = 0;
*/

	// accessors

	// get the size of the screen
	virtual void		getSize(SInt32* width, SInt32* height) const = 0;

	// get the size of jump zone
	virtual SInt32		getJumpZoneSize() const = 0;

/*
	// get the screen's clipboard contents
	virtual void		getClipboard(IClipboard*) const = 0;
*/
};

#endif
