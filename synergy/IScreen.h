#ifndef ISCREEN_H
#define ISCREEN_H

#include "IInterface.h"
#include "ClipboardTypes.h"

class IClipboard;

// the interface for platform dependent screen implementations.  each
// platform will derive a type from IScreen for interaction with the
// platform's screen that's common to primary and secondary screens.
class IScreen : public IInterface {
public:
	// manipulators

	// open the screen
	virtual void		open() = 0;

	// runs an event loop and returns when exitMainLoop() is called.
	// must be called between open() and close().
	virtual void		mainLoop() = 0;

	// force mainLoop() to return
	virtual void		exitMainLoop() = 0;

	// close the screen
	virtual void		close() = 0;

	// set the contents of the clipboard
	virtual bool		setClipboard(ClipboardID, const IClipboard*) = 0;

	// check clipboard ownership and notify IScreenReceiver (set through
	// some other interface) if any changed
	virtual void		checkClipboards() = 0;

	// open/close the screen saver.  if notify is true then this object
	// will call IScreenEventHandler's onScreenSaver() when the screensaver
	// activates or deactivates until close.  if notify is false then
	// the screen saver is disabled on open and restored on close.
	virtual void		openScreenSaver(bool notify) = 0;
	virtual void		closeScreenSaver() = 0;

	// activate or deactivate the screen saver
	virtual void		screensaver(bool activate) = 0;

	// FIXME -- need explanation
	virtual void		syncDesktop() = 0;

	// accessors

	// get the contents of the clipboard
	virtual bool		getClipboard(ClipboardID, IClipboard*) const = 0;

	// get the shape of the screen
	virtual void		getShape(SInt32& x, SInt32& y,
							SInt32& w, SInt32& h) const = 0;

	// get the current cursor coordinates
	virtual void		getCursorPos(SInt32& x, SInt32& y) const = 0;

	// get the cursor center position
	// FIXME -- need better explanation
	virtual void		getCursorCenter(SInt32& x, SInt32& y) const = 0;
};

#endif
