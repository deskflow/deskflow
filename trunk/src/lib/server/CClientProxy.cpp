/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2002 Chris Schoeneman, Nick Bolton, Sorin Sbarnea
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

#include "CClientProxy.h"
#include "CProtocolUtil.h"
#include "IStream.h"
#include "CLog.h"

//
// CClientProxy
//

CEvent::Type			CClientProxy::s_readyEvent           = CEvent::kUnknown;
CEvent::Type			CClientProxy::s_disconnectedEvent    = CEvent::kUnknown;
CEvent::Type			CClientProxy::s_clipboardChangedEvent= CEvent::kUnknown;

CClientProxy::CClientProxy(const CString& name, IStream* stream) :
	CBaseClientProxy(name),
	m_stream(stream)
{
	// do nothing
}

CClientProxy::~CClientProxy()
{
	delete m_stream;
}

void
CClientProxy::close(const char* msg)
{
	LOG((CLOG_DEBUG1 "send close \"%s\" to \"%s\"", msg, getName().c_str()));
	CProtocolUtil::writef(getStream(), msg);

	// force the close to be sent before we return
	getStream()->flush();
}

IStream*
CClientProxy::getStream() const
{
	return m_stream;
}

CEvent::Type
CClientProxy::getReadyEvent()
{
	return CEvent::registerTypeOnce(s_readyEvent,
							"CClientProxy::ready");
}

CEvent::Type
CClientProxy::getDisconnectedEvent()
{
	return CEvent::registerTypeOnce(s_disconnectedEvent,
							"CClientProxy::disconnected");
}

CEvent::Type
CClientProxy::getClipboardChangedEvent()
{
	return CEvent::registerTypeOnce(s_clipboardChangedEvent,
							"CClientProxy::clipboardChanged");
}

void*
CClientProxy::getEventTarget() const
{
	return static_cast<IScreen*>(const_cast<CClientProxy*>(this));
}
