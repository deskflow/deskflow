#ifndef CTCPSOCKET_H
#define CTCPSOCKET_H

#include "IDataSocket.h"
#include "CNetwork.h"

class CMutex;
template <class T>
class CCondVar;
class CThread;
class CBufferedInputStream;
class CBufferedOutputStream;

class CTCPSocket : public IDataSocket {
public:
	CTCPSocket();
	CTCPSocket(CNetwork::Socket);
	~CTCPSocket();

	// manipulators

	// accessors

	// ISocket overrides
	virtual void		bind(const CNetworkAddress&);
	virtual void		close();

	// IDataSocket overrides
	virtual void		connect(const CNetworkAddress&);
	virtual IInputStream*	getInputStream();
	virtual IOutputStream*	getOutputStream();

private:
	void				init();
	void				ioThread(void*);
	void				ioCleanup();
	void				ioService();
	void				closeInput(void*);
	void				closeOutput(void*);

private:
	enum { kClosed = 0, kRead = 1, kWrite = 2, kReadWrite = 3 };

	CNetwork::Socket		m_fd;
	CBufferedInputStream*	m_input;
	CBufferedOutputStream*	m_output;

	CMutex*				m_mutex;
	CThread*			m_thread;
	UInt32				m_connected;
};

#endif
