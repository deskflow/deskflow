#ifndef COUTPUTPACKETSTREAM_H
#define COUTPUTPACKETSTREAM_H

#include "COutputStreamFilter.h"

class COutputPacketStream : public COutputStreamFilter {
  public:
	COutputPacketStream(IOutputStream*, bool adoptStream = true);
	~COutputPacketStream();

	// manipulators

	// accessors

	// IOutputStream overrides
	virtual void		close() throw(XIO);
	virtual UInt32		write(const void*, UInt32 count) throw(XIO);
	virtual void		flush() throw(XIO);
};

#endif
