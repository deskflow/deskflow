#ifndef CSOCKETINPUTSTREAM_H
#define CSOCKETINPUTSTREAM_H

#include "CSocketStreamBuffer.h"
#include "CCondVar.h"
#include "IInputStream.h"

class CMutex;
class IJob;

class CSocketInputStream : public IInputStream {
  public:
	CSocketInputStream(CMutex*, IJob* adoptedCloseCB);
	~CSocketInputStream();

	// manipulators

	// write() appends n bytes to the buffer
	void				write(const void*, UInt32 n);

	// causes read() to always return immediately.  if there is no
	// more data then it returns 0.  further writes are discarded.
	void				hangup();

	// accessors

	// IInputStream overrides
	// these all lock the mutex for their duration
	virtual void		close();
	virtual UInt32		read(void*, UInt32 count);
	virtual UInt32		getSize() const;

  private:
	CMutex*				m_mutex;
	CCondVar<bool>		m_empty;
	IJob*				m_closeCB;
	CSocketStreamBuffer	m_buffer;
	bool				m_closed;
	bool				m_hungup;
};

#endif
