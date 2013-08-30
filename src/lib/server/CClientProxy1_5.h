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

#include "CClientProxy1_4.h"
#include "CStopwatch.h"

class CServer;
class IEventQueue;

//! Proxy for client implementing protocol version 1.5
class CClientProxy1_5 : public CClientProxy1_4 {
public:
	CClientProxy1_5(const CString& name, synergy::IStream* adoptedStream, CServer* server, IEventQueue* events);
	~CClientProxy1_5();

	virtual void		draggingInfoSending(UInt32 fileCount, const char* data, size_t dataSize);
	virtual void		fileChunkSending(UInt8 mark, char* data, size_t dataSize);
	virtual bool		parseMessage(const UInt8* code);
	void				fileChunkReceived();
	void				dragInfoReceived();

private:
	IEventQueue*		m_events;

	CStopwatch			m_stopwatch;
	double				m_elapsedTime;
	size_t				m_receivedDataSize;
	static const UInt16	m_intervalThreshold;
};
