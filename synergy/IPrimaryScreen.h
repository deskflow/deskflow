#ifndef IPRIMARYSCREEN_H
#define IPRIMARYSCREEN_H

#include "IInterface.h"
#include "BasicTypes.h"

class CServer;
class IClipboard;

class IPrimaryScreen : public IInterface {
  public:
	// manipulators

	// initialize the screen and start reporting events to the server.
	// events should be reported no matter where on the screen they
	// occur but do not interfere with normal event dispatch.  the
	// screen saver engaging should be reported as an event.  if that
	// can't be detected then this object should disable the system's
	// screen saver timer and should start the screen saver after
	// idling for an appropriate time.
	virtual void		open(CServer*) = 0;

	// close the screen.  should restore the screen saver timer if it
	// was disabled.
	virtual void		close() = 0;

	// called when the user navigates back to the primary screen.
	// warp the cursor to the given coordinates, unhide it, and
	// ungrab the input devices.  every call to method has a matching
	// call to leave() which preceeds it.
	virtual void		enter(SInt32 xAbsolute, SInt32 yAbsolute) = 0;

	// called when the user navigates off the primary screen.  hide
	// the cursor and grab exclusive access to the input devices.
	virtual void		leave() = 0;

	// warp the cursor to the given position
	virtual void		warpCursor(SInt32 xAbsolute, SInt32 yAbsolute) = 0;

	// set the screen's clipboard contents.  this is usually called
	// soon after an enter().
	virtual void		setClipboard(const IClipboard*) = 0;

/*
	// show or hide the screen saver
	virtual void		onScreenSaver(bool show) = 0;

	// clipboard input
	virtual void		onClipboardChanged() = 0;
*/

	// accessors

/*
	// get the screen's name.  all screens must have a name unique on
	// the server they connect to.  the hostname is usually an
	// appropriate name.
	virtual CString		getName() const = 0;
*/

	// get the size of the screen
	virtual void		getSize(SInt32* width, SInt32* height) const = 0;

	// get the size of jump zone
	virtual SInt32		getJumpZoneSize() const = 0;

	// get the screen's clipboard contents
	virtual void		getClipboard(IClipboard*) const = 0;
};

#endif
