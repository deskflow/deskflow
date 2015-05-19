/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2015 Synergy Si Inc.
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "synergy/FileChunk.h"

#include "synergy/protocol_types.h"

FileChunk::FileChunk(size_t size) :
	Chunk(size)
{
		m_dataSize = size - FILE_CHUNK_META_SIZE;
}

FileChunk*
FileChunk::start(const String& size)
{
	size_t sizeLength = size.size();
	FileChunk* start = new FileChunk(sizeLength + FILE_CHUNK_META_SIZE);
	char* chunk = start->m_chunk;
	chunk[0] = kDataStart;
	memcpy(&chunk[1], size.c_str(), sizeLength);
	chunk[sizeLength + 1] = '\0';

	return start;
}

FileChunk*
FileChunk::data(UInt8* data, size_t dataSize)
{
	FileChunk* chunk = new FileChunk(dataSize + FILE_CHUNK_META_SIZE);
	char* chunkData = chunk->m_chunk;
	chunkData[0] = kDataChunk;
	memcpy(&chunkData[1], data, dataSize);
	chunkData[dataSize + 1] = '\0';

	return chunk;
}

FileChunk*
FileChunk::end()
{
	FileChunk* end = new FileChunk(FILE_CHUNK_META_SIZE);
	char* chunk = end->m_chunk;
	chunk[0] = kDataEnd;
	chunk[1] = '\0';

	return end;
}
