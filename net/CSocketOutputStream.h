#ifndef CSOCKETOUTPUTSTREAM_H
#define CSOCKETOUTPUTSTREAM_H

#include "CSocketStreamBuffer.h"
#include "IOutputStream.h"

class CMutex;
class IJob;

class CSocketOutputStream : public IOutputStream {
  public:
	CSocketOutputStream(CMutex*, IJob* adoptedCloseCB);
	~CSocketOutputStream();

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
	UInt32				getSizeWithLock() const;

  private:
	CMutex*				m_mutex;
	IJob*				m_closeCB;
	CSocketStreamBuffer	m_buffer;
	bool				m_closed;
};

#endif
