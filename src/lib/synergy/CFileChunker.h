/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2013 Bolton Software Ltd.
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
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

#include "CString.h"

class IEventQueue;

class CFileChunker {
public:
	//! FileChunk data
	class CFileChunk {
	public:
		CFileChunk(size_t chunkSize) : m_dataSize(chunkSize - 2)
		{
			m_chunk = new char[chunkSize]; 
		}

		~CFileChunk() { delete[] m_chunk; }

	public:
		const size_t	m_dataSize;
		char*			m_chunk;
	};

	static void			sendFileChunks(char* filename, IEventQueue* events, void* eventTarget);
	static CString		intToString(size_t i);

private:
	static const size_t m_chunkSize;
};
