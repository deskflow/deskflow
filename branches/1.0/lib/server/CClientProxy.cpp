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
#include "IInputStream.h"
#include "IOutputStream.h"

//
// CClientProxy
//

CClientProxy::CClientProxy(IServer* server, const CString& name,
				IInputStream* input, IOutputStream* output) :
	m_server(server),
	m_name(name),
	m_input(input),
	m_output(output)
{
	// do nothing
}

CClientProxy::~CClientProxy()
{
	delete m_output;
	delete m_input;
}

IServer*
CClientProxy::getServer() const
{
	return m_server;
}

IInputStream*
CClientProxy::getInputStream() const
{
	return m_input;
}

IOutputStream*
CClientProxy::getOutputStream() const
{
	return m_output;
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
