#ifndef CBUFFEREDOUTPUTSTREAM_H
#define CBUFFEREDOUTPUTSTREAM_H

#include "CStreamBuffer.h"
#include "IOutputStream.h"
#include "CCondVar.h"

class CMutex;
class IJob;

class CBufferedOutputStream : public IOutputStream {
public:
	CBufferedOutputStream(CMutex*, IJob* adoptedCloseCB);
	~CBufferedOutputStream();

	// the caller is expected to lock the mutex before calling
	// methods unless otherwise noted.

	// manipulators

	// peek() returns a buffer of n bytes (which must be <= getSize()).
	// pop() discards the next n bytes.
	const void*			peek(UInt32 n);
	void				pop(UInt32 n);

	// accessors

	// return the number of bytes in the buffer
	UInt32				getSize() const;

	// IOutputStream overrides
	// these all lock the mutex for their duration
	virtual void		close();
	virtual UInt32		write(const void*, UInt32 count);
	virtual void		flush();

private:
	CMutex*				m_mutex;
	IJob*				m_closeCB;
	CCondVar<bool>		m_empty;
	CStreamBuffer		m_buffer;
	bool				m_closed;
};

#endif
