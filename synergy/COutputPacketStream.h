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
	virtual void		close();
	virtual UInt32		write(const void*, UInt32 count);
	virtual void		flush();
};

#endif
