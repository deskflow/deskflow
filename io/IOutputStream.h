#ifndef IOUTPUTSTREAM_H
#define IOUTPUTSTREAM_H

#include "IInterface.h"
#include "BasicTypes.h"
#include "XIO.h"

class IOutputStream : public IInterface {
  public:
	// manipulators

	// close the stream
	virtual void		close() = 0;

	// write count bytes to stream
	virtual UInt32		write(const void*, UInt32 count) = 0;

	// flush the stream
	virtual void		flush() = 0;

	// accessors
};

#endif
