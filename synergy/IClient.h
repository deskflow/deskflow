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

	// FIXME -- methods for setting clipboard.  on proxy side these
	// will set/reset the gotClipboard flag and send the clipboard
	// when not gotClipboard is false.  grabbing goes in here too.
	// basically, figure out semantics of these methods.  note that
	// ISecondaryScreen wants an IClipboard* passed to set not a
	// string;  will that be a problem?

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
	// FIXME -- if server uses IClient as interface to primary screen
	// (should this class be renamed?) then be careful of absolute/relative
	// coordinates here;  move on primary and move on secondary take
	// different values.  probably not relevant, though.
	virtual void		mouseMove(SInt32 xAbs, SInt32 yAbs) = 0;
	virtual void		mouseWheel(SInt32 delta) = 0;
	virtual void		screenSaver(bool activate) = 0;

	// accessors

	// get the client's identifier
	virtual CString		getName() const = 0;

	// get the screen's shape
	// FIXME -- may want center pixel too
	virtual void		getShape(SInt32& x, SInt32& y,
							SInt32& width, SInt32& height) const = 0;

	// get the center pixel
	virtual void		getCenter(SInt32& x, SInt32& y) const = 0;

	// get the size of jump zone
	virtual SInt32		getJumpZoneSize() const = 0;

	// what about getClipboard()?  don't really want proxy to ask for it;
	// it's a push data model.  screen info is cached in proxy so it's
	// different.  will need to keep onClipboardChanged() in CClient.
};

#endif
