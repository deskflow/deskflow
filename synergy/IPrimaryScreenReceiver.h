#ifndef IPRIMARYSCREENRECEIVER_H
#define IPRIMARYSCREENRECEIVER_H

#include "IInterface.h"
#include "KeyTypes.h"
#include "MouseTypes.h"

// the interface for receiving notification of events on the primary
// screen.  the server implements this interface to handle user input.
// platform dependent primary screen implementation will need to take
// an IPrimaryScreenReceiver* and notify it of events.
class IPrimaryScreenReceiver : public IInterface {
public:
	// called when the screensaver is activated or deactivated
	virtual void		onScreensaver(bool activated) = 0;

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
};

#endif
