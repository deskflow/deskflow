#ifndef COUTPUTPACKETSTREAM_H
#define COUTPUTPACKETSTREAM_H

#include "COutputStreamFilter.h"

//! Packetizing output stream filter
/*!
Filters an output stream to create packets that include message
boundaries.  Each write() is considered a single packet.
*/
class COutputPacketStream : public COutputStreamFilter {
public:
	COutputPacketStream(IOutputStream*, bool adoptStream = true);
	~COutputPacketStream();

	// IOutputStream overrides
	virtual void		close();
	virtual UInt32		write(const void*, UInt32 count);
	virtual void		flush();
};

#endif
