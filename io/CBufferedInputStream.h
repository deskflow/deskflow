#ifndef CBUFFEREDINPUTSTREAM_H
#define CBUFFEREDINPUTSTREAM_H

#include "CStreamBuffer.h"
#include "CCondVar.h"
#include "IInputStream.h"

class CMutex;
class IJob;

class CBufferedInputStream : public IInputStream {
public:
	CBufferedInputStream(CMutex*, IJob* adoptedCloseCB);
	~CBufferedInputStream();

	// the caller is expected to lock the mutex before calling
	// methods unless otherwise noted.

	// manipulators

	// write() appends n bytes to the buffer
	void				write(const void*, UInt32 n);

	// causes read() to always return immediately.  if there is no
	// more data then it returns 0.  further writes are discarded.
	void				hangup();

	// same as read() but caller must lock the mutex
	UInt32				readNoLock(void*, UInt32 count);

	// accessors

	// same as getSize() but caller must lock the mutex
	UInt32				getSizeNoLock() const;

	// IInputStream overrides
	// these all lock the mutex for their duration
	virtual void		close();
	virtual UInt32		read(void*, UInt32 count);
	virtual UInt32		getSize() const;

private:
	CMutex*				m_mutex;
	CCondVar<bool>		m_empty;
	IJob*				m_closeCB;
	CStreamBuffer		m_buffer;
	bool				m_closed;
	bool				m_hungup;
};

#endif
