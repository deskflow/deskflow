#ifndef ICLIENT_H
#define ICLIENT_H

#include "IInterface.h"
#include "ClipboardTypes.h"
#include "KeyTypes.h"
#include "MouseTypes.h"
#include "CString.h"

class IClient : public IInterface {
public:
	// manipulators

	// open client
	virtual void		open() = 0;

	// service client
	virtual void		run() = 0;

	// close client
	virtual void		close() = 0;

	// enter the screen.  the cursor should be warped to xAbs,yAbs.
	// the client should record seqNum for future reporting of
	// clipboard changes.  mask is the expected toggle button state.
	// screenSaver is true if the screen is being entered because
	// the screen saver is starting.
	virtual void		enter(SInt32 xAbs, SInt32 yAbs,
							UInt32 seqNum, KeyModifierMask mask,
							bool screenSaver) = 0;

	// leave the screen.  returns false if the user may not leave the
	// client's screen.
	virtual bool		leave() = 0;

	// update the client's clipboard.  this implies that the client's
	// clipboard is now up to date.  if the client's clipboard was
	// already known to be up to date then this can do nothing.
	virtual void		setClipboard(ClipboardID, const CString&) = 0;

	// grab the client's clipboard.  since this is called when another
	// client takes ownership of the clipboard it implies that the
	// client's clipboard is dirty.
	virtual void		grabClipboard(ClipboardID) = 0;

	// called to set the client's clipboard as dirty or clean
	virtual void		setClipboardDirty(ClipboardID, bool dirty) = 0;

	// handle input events
	virtual void		keyDown(KeyID, KeyModifierMask) = 0;
	virtual void		keyRepeat(KeyID, KeyModifierMask, SInt32 count) = 0;
	virtual void		keyUp(KeyID, KeyModifierMask) = 0;
	virtual void		mouseDown(ButtonID) = 0;
	virtual void		mouseUp(ButtonID) = 0;
	virtual void		mouseMove(SInt32 xAbs, SInt32 yAbs) = 0;
	virtual void		mouseWheel(SInt32 delta) = 0;
	virtual void		screenSaver(bool activate) = 0;

	// accessors

	// get the client's identifier
	virtual CString		getName() const = 0;

	// get the screen's shape
	virtual void		getShape(SInt32& x, SInt32& y,
							SInt32& width, SInt32& height) const = 0;

	// get the center pixel
	virtual void		getCenter(SInt32& x, SInt32& y) const = 0;

	// get the size of jump zone
	virtual SInt32		getJumpZoneSize() const = 0;
};

#endif
