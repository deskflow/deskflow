#include "CStreamBuffer.h"
#include <assert.h>

//
// CStreamBuffer
//

const UInt32			CStreamBuffer::kChunkSize = 4096;

CStreamBuffer::CStreamBuffer() : m_size(0)
{
	// do nothing
}

CStreamBuffer::~CStreamBuffer()
{
	// do nothing
}

const void*				CStreamBuffer::peek(UInt32 n)
{
	assert(n <= m_size);

	// reserve space in first chunk
	ChunkList::iterator head = m_chunks.begin();
	head->reserve(n);

	// consolidate chunks into the first chunk until it has n bytes
	ChunkList::iterator scan = head;
	++scan;
	while (head->size() < n && scan != m_chunks.end()) {
		head->insert(head->end(), scan->begin(), scan->end());
		scan = m_chunks.erase(scan);
	}

	return reinterpret_cast<const void*>(head->begin());
}

void					CStreamBuffer::pop(UInt32 n)
{
	// discard all chunks if n is greater than or equal to m_size
	if (n >= m_size) {
		m_size = 0;
		m_chunks.clear();
		return;
	}

	// update size
	m_size -= n;

	// discard chunks until more than n bytes would've been discarded
	ChunkList::iterator scan = m_chunks.begin();
	assert(scan != m_chunks.end());
	while (scan->size() <= n) {
		n   -= scan->size();
		scan = m_chunks.erase(scan);
		assert(scan != m_chunks.end());
	}
	assert(n > 0);

	// remove left over bytes from the head chunk
	scan->erase(scan->begin(), scan->begin() + n);
}

#include "CLog.h"
void					CStreamBuffer::write(
								const void* vdata, UInt32 n)
{
	assert(vdata != NULL);

	// ignore if no data, otherwise update size
	if (n == 0) {
		return;
	}
log((CLOG_DEBUG "### %p buffering %d bytes onto %d bytes", this, n, m_size));
	m_size += n;

	// cast data to bytes
	const UInt8* data = reinterpret_cast<const UInt8*>(vdata);
if (n > 1000)
log((CLOG_DEBUG "### %p src: %u %u %u %u %u %u %u %u", this,
data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]));

	// point to last chunk if it has space, otherwise append an empty chunk
	ChunkList::iterator scan = m_chunks.end();
	if (scan != m_chunks.begin()) {
log((CLOG_DEBUG "### %p at least one chunk", this));
		--scan;
		if (scan->size() >= kChunkSize)
{
log((CLOG_DEBUG "### %p last chunk full", this));
			++scan;
}
	}
	if (scan == m_chunks.end()) {
log((CLOG_DEBUG "### %p append chunk", this));
		scan = m_chunks.insert(scan, Chunk());
	}

	// append data in chunks
	while (n > 0) {
		// choose number of bytes for next chunk
		assert(scan->size() <= kChunkSize);
		UInt32 count = kChunkSize - scan->size();
		if (count > n)
			count = n;

		// transfer data
log((CLOG_DEBUG "### %p append %d bytes onto last chunk", this, count));
		scan->insert(scan->end(), data, data + count);
		n    -= count;
		data += count;

		// append another empty chunk if we're not done yet
		if (n > 0) {
			++scan;
			scan = m_chunks.insert(scan, Chunk());
log((CLOG_DEBUG "### %p append chunk2", this));
		}
	}
}

UInt32					CStreamBuffer::getSize() const
{
	return m_size;
}

