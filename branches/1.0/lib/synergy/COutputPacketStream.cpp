/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2002 Chris Schoeneman
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "COutputPacketStream.h"

//
// COuputPacketStream
//

COutputPacketStream::COutputPacketStream(IOutputStream* stream, bool adopt) :
	COutputStreamFilter(stream, adopt)
{
	// do nothing
}

COutputPacketStream::~COutputPacketStream()
{
	// do nothing
}

void
COutputPacketStream::close()
{
	getStream()->close();
}

UInt32
COutputPacketStream::write(const void* buffer, UInt32 count)
{
	// write the length of the payload
	UInt8 length[4];
	length[0] = (UInt8)((count >> 24) & 0xff);
	length[1] = (UInt8)((count >> 16) & 0xff);
	length[2] = (UInt8)((count >>  8) & 0xff);
	length[3] = (UInt8)( count        & 0xff);
	UInt32 count2        = sizeof(length);
	const UInt8* cbuffer = length;
	while (count2 > 0) {
		UInt32 n = getStream()->write(cbuffer, count2);
		cbuffer += n;
		count2  -= n;
	}

	// write the payload
	count2  = count;
	cbuffer = reinterpret_cast<const UInt8*>(buffer);
	while (count2 > 0) {
		UInt32 n = getStream()->write(cbuffer, count2);
		cbuffer += n;
		count2  -= n;
	}

	return count;
}

void
COutputPacketStream::flush()
{
	getStream()->flush();
}

