#ifndef CUNIXTCPSOCKET_H
#define CUNIXTCPSOCKET_H

#include "CSocket.h"
#include "CSocketFactory.h"

class CUnixTCPSocket : public CSocket {
  public:
	CUnixTCPSocket();
	virtual ~CUnixTCPSocket();

	// ISocket overrides
	virtual void		connect(const CString& hostname, UInt16 port);
	virtual void		listen(const CString& hostname, UInt16 port);
	virtual ISocket*	accept();
	virtual SInt32		read(void* buffer, SInt32 numBytes);
	virtual void		write(const void* buffer, SInt32 numBytes);

  protected:
	// CSocket overrides
	virtual void		onJobChanged();

  private:
	CUnixTCPSocket(int);

	// disable Nagle algorithm
	void				setNoDelay();

	// callbacks for read/write events
	void				readCB();
	void				writeCB();

  private:
	enum EState { kNone, kConnecting, kConnected, kListening };
	int					m_fd;
	EState				m_state;
	bool				m_addedJobs;
};

class CUnixTCPSocketFactory : public CSocketFactory {
  public:
	CUnixTCPSocketFactory() { }
	virtual ~CUnixTCPSocketFactory() { }

	// CSocketFactory overrides
	virtual ISocket*	create() const
								{ return new CUnixTCPSocket; }
};

#endif
