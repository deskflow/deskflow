#ifndef IPRIMARYRECEIVER_H
#define IPRIMARYRECEIVER_H

#include "IInterface.h"
#include "ClipboardTypes.h"
#include "KeyTypes.h"
#include "MouseTypes.h"
#include "CString.h"

class IPrimaryReceiver : public IInterface {
public:
	// manipulators

	// notify of serious error.  this implies that the primary screen
	// cannot continue to function.
	virtual void		onError() = 0;

	// notify of info change
	virtual void		onInfoChanged(SInt32 xScreen, SInt32 yScreen,
							SInt32 wScreen, SInt32 hScreen,
							SInt32 zoneSize,
							SInt32 xMouse, SInt32 yMouse) = 0;

	// notify of clipboard grab.  returns true if the grab was honored,
	// false otherwise.
	virtual bool		onGrabClipboard(ClipboardID) = 0;

	// notify of new clipboard data.  returns true if the clipboard data
	// was accepted, false if the change was rejected.
	virtual bool		onClipboardChanged(ClipboardID,
							const CString& data) = 0;

	// call to notify of events.  onMouseMovePrimary() returns
	// true iff the mouse enters a jump zone and jumps.
	virtual void		onKeyDown(KeyID, KeyModifierMask) = 0;
	virtual void		onKeyUp(KeyID, KeyModifierMask) = 0;
	virtual void		onKeyRepeat(KeyID, KeyModifierMask, SInt32 count) = 0;
	virtual void		onMouseDown(ButtonID) = 0;
	virtual void		onMouseUp(ButtonID) = 0;
	virtual bool		onMouseMovePrimary(SInt32 x, SInt32 y) = 0;
	virtual void		onMouseMoveSecondary(SInt32 dx, SInt32 dy) = 0;
	virtual void		onMouseWheel(SInt32 delta) = 0;
	virtual void		onScreenSaver(bool activated) = 0;
};

#endif
