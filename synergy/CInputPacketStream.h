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
	virtual UInt32		read(void*, UInt32 maxCount, double timeout);
	virtual UInt32		getSize() const;

private:
	enum EResult { kData, kHungup, kTimedout };

	UInt32				getSizeNoLock() const;
	EResult				waitForFullMessage(double timeout) const;
	EResult				getMoreMessage(double timeout) const;
	bool				hasFullMessage() const;

private:
	CMutex				m_mutex;
	mutable UInt32					m_size;
	mutable CBufferedInputStream	m_buffer;
};

#endif

