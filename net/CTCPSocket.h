#ifndef CTCPSOCKET_H
#define CTCPSOCKET_H

#include "ISocket.h"
#include "XThread.h"

class CMutex;
template <class T>
class CCondVar;
class CThread;
class CBufferedInputStream;
class CBufferedOutputStream;

class CTCPSocket : public ISocket {
  public:
	CTCPSocket() throw(XSocket);
	CTCPSocket(int fd) throw();
	~CTCPSocket();

	// manipulators

	// accessors

	// ISocket overrides
	virtual void		bind(const CNetworkAddress&) throw(XSocket);
	virtual void		connect(const CNetworkAddress&) throw(XSocket);
	virtual void		close() throw(XIO);
	virtual IInputStream*	getInputStream() throw();
	virtual IOutputStream*	getOutputStream() throw();

  private:
	void				init() throw(XIO);
	void				service(void*) throw(XThread);
	void				closeInput(void*) throw();
	void				closeOutput(void*) throw();

  private:
	enum { kClosed = 0, kRead = 1, kWrite = 2, kReadWrite = 3 };

	int						m_fd;
	CBufferedInputStream*	m_input;
	CBufferedOutputStream*	m_output;

	CMutex*				m_mutex;
	CThread*			m_thread;
	UInt32				m_connected;
};

#endif
