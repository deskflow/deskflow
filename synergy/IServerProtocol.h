#ifndef ISERVERPROTOCOL_H
#define ISERVERPROTOCOL_H

#include "KeyTypes.h"
#include "MouseTypes.h"
#include "IInterface.h"
#include "XSynergy.h"
#include "XIO.h"

class IServerProtocol : public IInterface {
  public:
	// manipulators

	// process messages from the client and insert the appropriate
	// events into the server's event queue.  return when the client
	// disconnects.
	virtual void		run() throw(XIO,XBadClient) = 0;

	// send client info query and process reply
	virtual void		queryInfo() throw(XIO,XBadClient) = 0;

	// send various messages to client
	virtual void		sendClose() throw(XIO) = 0;
	virtual void		sendEnter(SInt32 xAbs, SInt32 yAbs) throw(XIO) = 0;
	virtual void		sendLeave() throw(XIO) = 0;
	virtual void		sendGrabClipboard() throw(XIO) = 0;
	virtual void		sendQueryClipboard() throw(XIO) = 0;
	virtual void		sendScreenSaver(bool on) throw(XIO) = 0;
	virtual void		sendKeyDown(KeyID, KeyModifierMask) throw(XIO) = 0;
	virtual void		sendKeyRepeat(KeyID, KeyModifierMask) throw(XIO) = 0;
	virtual void		sendKeyUp(KeyID, KeyModifierMask) throw(XIO) = 0;
	virtual void		sendMouseDown(ButtonID) throw(XIO) = 0;
	virtual void		sendMouseUp(ButtonID) throw(XIO) = 0;
	virtual void		sendMouseMove(SInt32 xAbs, SInt32 yAbs) throw(XIO) = 0;
	virtual void		sendMouseWheel(SInt32 delta) throw(XIO) = 0;

	// accessors

  protected:
	// manipulators

	virtual void		recvInfo() throw(XIO,XBadClient) = 0;

	// accessors
};

#endif
