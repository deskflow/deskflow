#ifndef CMESSAGESOCKET_H
#define CMESSAGESOCKET_H

#include "CSocket.h"

class CMessageSocket : public CSocket {
  public:
	CMessageSocket(ISocket* adoptedSocket);
	virtual ~CMessageSocket();

	// ISocket overrides
	// connect(), listen(), and accept() may not be called.
	virtual void		setWriteJob(IJob* adoptedJob);
	virtual void		connect(const CString& hostname, UInt16 port);
	virtual void		listen(const CString& hostname, UInt16 port);
	virtual ISocket*	accept();
	virtual SInt32		read(void* buffer, SInt32 numBytes);
	virtual void		write(const void* buffer, SInt32 numBytes);

  private:
	SInt32				doRead();
	virtual void		readJobCB();
	virtual void		writeJobCB();

  private:
	ISocket*			m_socket;
	UInt8*				m_buffer;
	SInt32				m_size;
	SInt32				m_capacity;
	SInt32				m_msgSize;
};

#endif
