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

	virtual CServer*		getServer() const throw();
	virtual CString			getClient() const throw();
	virtual IInputStream*	getInputStream() const throw();
	virtual IOutputStream*	getOutputStream() const throw();

	static IServerProtocol*	create(SInt32 major, SInt32 minor,
								CServer*, const CString& clientName,
								IInputStream*, IOutputStream*);

	// IServerProtocol overrides
	virtual void		run() throw(XIO,XBadClient) = 0;
	virtual void		queryInfo() throw(XIO,XBadClient) = 0;
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

  protected:
	//IServerProtocol overrides
	virtual void		recvInfo() throw(XIO,XBadClient) = 0;

  private:
	CServer*			m_server;
	CString				m_client;
	IInputStream*		m_input;
	IOutputStream*		m_output;
};

#endif
