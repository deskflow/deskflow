#ifndef CSERVERPROTOCOL_H
#define CSERVERPROTOCOL_H

#include "CString.h"
#include "IServerProtocol.h"

class CServer;
class IInputStream;
class IOutputStream;

class CServerProtocol : public IServerProtocol {
  public:
	CServerProtocol(CServer*, const CString& clientName,
								IInputStream*, IOutputStream*);
	~CServerProtocol();

	// manipulators

	// accessors

	virtual CServer*		getServer() const;
	virtual CString			getClient() const;
	virtual IInputStream*	getInputStream() const;
	virtual IOutputStream*	getOutputStream() const;

	static IServerProtocol*	create(SInt32 major, SInt32 minor,
								CServer*, const CString& clientName,
								IInputStream*, IOutputStream*);

	// IServerProtocol overrides
	virtual void		run() = 0;
	virtual void		queryInfo() = 0;
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

  protected:
	//IServerProtocol overrides
	virtual void		recvInfo() = 0;
	virtual void		recvClipboard() = 0;
	virtual void		recvGrabClipboard() = 0;

  private:
	CServer*			m_server;
	CString				m_client;
	IInputStream*		m_input;
	IOutputStream*		m_output;
};

#endif
