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

#include "CClientProxy.h"
#include "IStream.h"

//
// CClientProxy
//

CClientProxy::CClientProxy(IServer* server,
				const CString& name, IStream* stream) :
	m_server(server),
	m_name(name),
	m_stream(stream)
{
	// do nothing
}

CClientProxy::~CClientProxy()
{
	delete m_stream;
}

IServer*
CClientProxy::getServer() const
{
	return m_server;
}

IStream*
CClientProxy::getStream() const
{
	return m_stream;
}

CString
CClientProxy::getName() const
{
	return m_name;
}

const CMutex*
CClientProxy::getMutex() const
{
	return &m_mutex;
}
