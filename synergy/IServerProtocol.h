#ifndef ISERVERPROTOCOL_H
#define ISERVERPROTOCOL_H

#include "KeyTypes.h"
#include "MouseTypes.h"
#include "IInterface.h"
#include "XSynergy.h"
#include "XIO.h"

class IClipboard;

class IServerProtocol : public IInterface {
  public:
	// manipulators

	// process messages from the client and insert the appropriate
	// events into the server's event queue.  return when the client
	// disconnects.
	virtual void		run() = 0;

	// send client info query and process reply
	virtual void		queryInfo() = 0;

	// send various messages to client
	virtual void		sendClose() = 0;
	virtual void		sendEnter(SInt32 xAbs, SInt32 yAbs) = 0;
	virtual void		sendLeave() = 0;
	virtual void		sendClipboard(const CString&) = 0;
	virtual void		sendGrabClipboard() = 0;
	virtual void		sendQueryClipboard(UInt32 seqNum) = 0;
	virtual void		sendScreenSaver(bool on) = 0;
	virtual void		sendKeyDown(KeyID, KeyModifierMask) = 0;
	virtual void		sendKeyRepeat(KeyID, KeyModifierMask, SInt32 count) = 0;
	virtual void		sendKeyUp(KeyID, KeyModifierMask) = 0;
	virtual void		sendMouseDown(ButtonID) = 0;
	virtual void		sendMouseUp(ButtonID) = 0;
	virtual void		sendMouseMove(SInt32 xAbs, SInt32 yAbs) = 0;
	virtual void		sendMouseWheel(SInt32 delta) = 0;

	// accessors

  protected:
	// manipulators

	virtual void		recvInfo() = 0;
	virtual void		recvClipboard() = 0;
	virtual void		recvGrabClipboard() = 0;

	// accessors
};

#endif
