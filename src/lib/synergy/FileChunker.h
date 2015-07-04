/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2013 Synergy Si Ltd.
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

#pragma once

#include "base/String.h"

class IEventQueue;

class FileChunker {
public:
	//! FileChunk data
	class FileChunk {
	public:
		FileChunk(size_t chunkSize) : m_dataSize(chunkSize - 2)
		{
			m_chunk = new char[chunkSize]; 
		}

		~FileChunk() { delete[] m_chunk; }

	public:
		const size_t	m_dataSize;
		char*			m_chunk;
	};

	static void			sendFileChunks(char* filename, IEventQueue* events, void* eventTarget);
	static String		intToString(size_t i);

private:
	static const size_t m_chunkSize;
};
