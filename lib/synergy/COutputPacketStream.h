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

#ifndef COUTPUTPACKETSTREAM_H
#define COUTPUTPACKETSTREAM_H

#include "COutputStreamFilter.h"

//! Packetizing output stream filter
/*!
Filters an output stream to create packets that include message
boundaries.  Each write() is considered a single packet.
*/
class COutputPacketStream : public COutputStreamFilter {
public:
	COutputPacketStream(IOutputStream*, bool adoptStream = true);
	~COutputPacketStream();

	// IOutputStream overrides
	virtual void		close();
	virtual UInt32		write(const void*, UInt32 count);
	virtual void		flush();
};

#endif
