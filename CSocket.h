#ifndef CSOCKET_H
#define CSOCKET_H

#include "ISocket.h"

class IJob;

class CSocket : public ISocket {
  public:
	CSocket();
	virtual ~CSocket();

	// ISocket overrides
	virtual void		setReadJob(IJob* adoptedJob);
	virtual void		setWriteJob(IJob* adoptedJob);
	virtual void		connect(const CString& hostname, UInt16 port) = 0;
	virtual void		listen(const CString& hostname, UInt16 port) = 0;
	virtual ISocket*	accept() = 0;
	virtual SInt32		read(void* buffer, SInt32 numBytes) = 0;
	virtual void		write(const void* buffer, SInt32 numBytes) = 0;

  protected:
	// called when the read or write job is changed.  default does nothing.
	virtual void		onJobChanged();

	// subclasses should call these at the appropriate time.  different
	// platforms will arrange to detect these situations differently.
	// does nothing if the respective job is NULL.
	void				runReadJob();
	void				runWriteJob();

	// return true iff the socket has a read job or a write job
	bool				hasReadJob() const;
	bool				hasWriteJob() const;

  private:
	IJob*				m_readJob;
	IJob*				m_writeJob;
};

#endif
