#ifndef IINPUTSTREAM_H
#define IINPUTSTREAM_H

#include "IInterface.h"
#include "BasicTypes.h"
#include "XIO.h"

class IInputStream : public IInterface {
public:
	// manipulators

	// close the stream
	virtual void		close() = 0;

	// read up to maxCount bytes into buffer, return number read.
	// blocks if no data is currently available.  if buffer is NULL
	// then the data is discarded.
	virtual UInt32		read(void* buffer, UInt32 maxCount) = 0;

	// accessors

	// get a conservative estimate of the available bytes to read
	// (i.e. a number not greater than the actual number of bytes).
	// some streams may not be able to determine this and will always
	// return zero.
	virtual UInt32		getSize() const = 0;
};

#endif
