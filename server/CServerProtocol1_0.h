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
	virtual void		run();
	virtual void		queryInfo();
	virtual void		sendClose();
	virtual void		sendEnter(SInt32 xAbs, SInt32 yAbs,
							UInt32 seqNum, KeyModifierMask mask);
	virtual void		sendLeave();
	virtual void		sendClipboard(ClipboardID, const CString&);
	virtual void		sendGrabClipboard(ClipboardID);
	virtual void		sendScreenSaver(bool on);
	virtual void		sendInfoAcknowledgment();
	virtual void		sendKeyDown(KeyID, KeyModifierMask);
	virtual void		sendKeyRepeat(KeyID, KeyModifierMask, SInt32 count);
	virtual void		sendKeyUp(KeyID, KeyModifierMask);
	virtual void		sendMouseDown(ButtonID);
	virtual void		sendMouseUp(ButtonID);
	virtual void		sendMouseMove(SInt32 xAbs, SInt32 yAbs);
	virtual void		sendMouseWheel(SInt32 delta);

protected:
	// IServerProtocol overrides
	virtual void		recvInfo();
	virtual void		recvClipboard();
	virtual void		recvGrabClipboard();
};

#endif
