#ifndef CSTREAMBUFFER_H
#define CSTREAMBUFFER_H

#include "BasicTypes.h"
#include "stdlist.h"
#include "stdvector.h"

class CStreamBuffer {
public:
	CStreamBuffer();
	~CStreamBuffer();

	// manipulators

	// peek() returns a buffer of n bytes (which must be <= getSize()).
	// pop() discards the next n bytes.
	const void*			peek(UInt32 n);
	void				pop(UInt32 n);

	// write() appends n bytes to the buffer
	void				write(const void*, UInt32 n);

	// accessors

	// return the number of bytes in the buffer
	UInt32				getSize() const;

private:
	static const UInt32	kChunkSize;

	typedef std::vector<UInt8> Chunk;
	typedef std::list<Chunk> ChunkList;

	ChunkList			m_chunks;
	UInt32				m_size;
};

#endif
