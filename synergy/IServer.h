#ifndef ISERVER_H
#define ISERVER_H

#include "IInterface.h"
#include "ClipboardTypes.h"
#include "KeyTypes.h"
#include "MouseTypes.h"
#include "CString.h"

class CClientInfo;

class IServer : public IInterface {
public:
	// manipulators

	// notify of serious error.  this implies that the server should
	// shutdown.
	virtual void		onError() = 0;

	// notify of client info change
	virtual void		onInfoChanged(const CString& clientName,
							const CClientInfo&) = 0;

	// notify of clipboard grab.  returns true if the grab was honored,
	// false otherwise.
	virtual bool		onGrabClipboard(const CString& clientName,
							ClipboardID, UInt32 seqNum) = 0;

	// notify of new clipboard data
	virtual void		onClipboardChanged(ClipboardID,
							UInt32 seqNum, const CString& data) = 0;

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
