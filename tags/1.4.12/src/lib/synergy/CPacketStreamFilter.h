/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Bolton Software Ltd.
 * Copyright (C) 2004 Chris Schoeneman
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

#ifndef CPACKETSTREAMFILTER_H
#define CPACKETSTREAMFILTER_H

#include "CStreamFilter.h"
#include "CStreamBuffer.h"
#include "CMutex.h"

//! Packetizing stream filter 
/*!
Filters a stream to read and write packets.
*/
class CPacketStreamFilter : public CStreamFilter {
public:
	CPacketStreamFilter(synergy::IStream* stream, bool adoptStream = true);
	~CPacketStreamFilter();

	// IStream overrides
	virtual void		close();
	virtual UInt32		read(void* buffer, UInt32 n);
	virtual void		write(const void* buffer, UInt32 n);
	virtual void		shutdownInput();
	virtual bool		isReady() const;
	virtual UInt32		getSize() const;

protected:
	// CStreamFilter overrides
	virtual void		filterEvent(const CEvent&);

private:
	bool				isReadyNoLock() const;
	void				readPacketSize();
	bool				readMore();

private:
	CMutex				m_mutex;
	UInt32				m_size;
	CStreamBuffer		m_buffer;
	bool				m_inputShutdown;
};

#endif
