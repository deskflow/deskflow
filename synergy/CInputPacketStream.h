#ifndef CINPUTPACKETSTREAM_H
#define CINPUTPACKETSTREAM_H

#include "CInputStreamFilter.h"
#include "CBufferedInputStream.h"
#include "CMutex.h"

class CInputPacketStream : public CInputStreamFilter {
public:
	CInputPacketStream(IInputStream*, bool adoptStream = true);
	~CInputPacketStream();

	// manipulators

	// accessors

	// IInputStream overrides
	virtual void		close();
	virtual UInt32		read(void*, UInt32 maxCount);
	virtual UInt32		getSize() const;

private:
	UInt32				getSizeNoLock() const;
	bool				waitForFullMessage() const;
	bool				getMoreMessage() const;
	bool				hasFullMessage() const;

private:
	CMutex				m_mutex;
	mutable UInt32					m_size;
	mutable CBufferedInputStream	m_buffer;
};

#endif

