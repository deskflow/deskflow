#ifndef CSERVERPROTOCOL1_0_H
#define CSERVERPROTOCOL1_0_H

#include "CServerProtocol.h"

class CServerProtocol1_0 : public CServerProtocol {
  public:
	CServerProtocol1_0(CServer*, const CString&, IInputStream*, IOutputStream*);
	~CServerProtocol1_0();

	// manipulators

	// accessors

	// IServerProtocol overrides
	virtual void		run() throw(XIO,XBadClient);
	virtual void		queryInfo() throw(XIO,XBadClient);
	virtual void		sendClose() throw(XIO);
	virtual void		sendEnter(SInt32 xAbs, SInt32 yAbs) throw(XIO);
	virtual void		sendLeave() throw(XIO);
	virtual void		sendGrabClipboard() throw(XIO);
	virtual void		sendQueryClipboard() throw(XIO);
	virtual void		sendScreenSaver(bool on) throw(XIO);
	virtual void		sendKeyDown(KeyID, KeyModifierMask) throw(XIO);
	virtual void		sendKeyRepeat(KeyID, KeyModifierMask) throw(XIO);
	virtual void		sendKeyUp(KeyID, KeyModifierMask) throw(XIO);
	virtual void		sendMouseDown(ButtonID) throw(XIO);
	virtual void		sendMouseUp(ButtonID) throw(XIO);
	virtual void		sendMouseMove(SInt32 xAbs, SInt32 yAbs) throw(XIO);
	virtual void		sendMouseWheel(SInt32 delta) throw(XIO);

  protected:
	// IServerProtocol overrides
	virtual void		recvInfo() throw(XIO,XBadClient);
};

#endif
