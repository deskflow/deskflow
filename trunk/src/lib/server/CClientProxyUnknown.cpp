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

#include "CClientProxyUnknown.h"
#include "CClientProxy1_0.h"
#include "CClientProxy1_1.h"
#include "CClientProxy1_2.h"
#include "CClientProxy1_3.h"
#include "CClientProxy1_4.h"
#include "CClientProxy1_5.h"
#include "ProtocolTypes.h"
#include "CProtocolUtil.h"
#include "XSynergy.h"
#include "IStream.h"
#include "XIO.h"
#include "CLog.h"
#include "CString.h"
#include "IEventQueue.h"
#include "TMethodEventJob.h"
#include "CServer.h"

//
// CClientProxyUnknown
//

CClientProxyUnknown::CClientProxyUnknown(synergy::IStream* stream, double timeout, CServer* server, IEventQueue* events) :
	m_events(events),
	m_stream(stream),
	m_proxy(NULL),
	m_ready(false),
	m_server(server)
{
	assert(m_server != NULL);

	m_events->adoptHandler(CEvent::kTimer, this,
							new TMethodEventJob<CClientProxyUnknown>(this,
								&CClientProxyUnknown::handleTimeout, NULL));
	m_timer = m_events->newOneShotTimer(timeout, this);
	addStreamHandlers();

	LOG((CLOG_DEBUG1 "saying hello"));
	CProtocolUtil::writef(m_stream, kMsgHello,
							kProtocolMajorVersion,
							kProtocolMinorVersion);
}

CClientProxyUnknown::~CClientProxyUnknown()
{
	removeHandlers();
	removeTimer();
	delete m_stream;
	delete m_proxy;
}

CClientProxy*
CClientProxyUnknown::orphanClientProxy()
{
	if (m_ready) {
		removeHandlers();
		CClientProxy* proxy = m_proxy;
		m_proxy = NULL;
		return proxy;
	}
	else {
		return NULL;
	}
}

void
CClientProxyUnknown::sendSuccess()
{
	m_ready = true;
	removeTimer();
	m_events->addEvent(CEvent(m_events->forCClientProxyUnknown().success(), this));
}

void
CClientProxyUnknown::sendFailure()
{
	delete m_proxy;
	m_proxy = NULL;
	m_ready = false;
	removeHandlers();
	removeTimer();
	m_events->addEvent(CEvent(m_events->forCClientProxyUnknown().failure(), this));
}

void
CClientProxyUnknown::addStreamHandlers()
{
	assert(m_stream != NULL);

	m_events->adoptHandler(m_events->forIStream().inputReady(),
							m_stream->getEventTarget(),
							new TMethodEventJob<CClientProxyUnknown>(this,
								&CClientProxyUnknown::handleData));
	m_events->adoptHandler(m_events->forIStream().outputError(),
							m_stream->getEventTarget(),
							new TMethodEventJob<CClientProxyUnknown>(this,
								&CClientProxyUnknown::handleWriteError));
	m_events->adoptHandler(m_events->forIStream().inputShutdown(),
							m_stream->getEventTarget(),
							new TMethodEventJob<CClientProxyUnknown>(this,
								&CClientProxyUnknown::handleDisconnect));
	m_events->adoptHandler(m_events->forIStream().outputShutdown(),
							m_stream->getEventTarget(),
							new TMethodEventJob<CClientProxyUnknown>(this,
								&CClientProxyUnknown::handleWriteError));
}

void
CClientProxyUnknown::addProxyHandlers()
{
	assert(m_proxy != NULL);

	m_events->adoptHandler(m_events->forCClientProxy().ready(),
							m_proxy,
							new TMethodEventJob<CClientProxyUnknown>(this,
								&CClientProxyUnknown::handleReady));
	m_events->adoptHandler(m_events->forCClientProxy().disconnected(),
							m_proxy,
							new TMethodEventJob<CClientProxyUnknown>(this,
								&CClientProxyUnknown::handleDisconnect));
}

void
CClientProxyUnknown::removeHandlers()
{
	if (m_stream != NULL) {
		m_events->removeHandler(m_events->forIStream().inputReady(),
							m_stream->getEventTarget());
		m_events->removeHandler(m_events->forIStream().outputError(),
							m_stream->getEventTarget());
		m_events->removeHandler(m_events->forIStream().inputShutdown(),
							m_stream->getEventTarget());
		m_events->removeHandler(m_events->forIStream().outputShutdown(),
							m_stream->getEventTarget());
	}
	if (m_proxy != NULL) {
		m_events->removeHandler(m_events->forCClientProxy().ready(),
							m_proxy);
		m_events->removeHandler(m_events->forCClientProxy().disconnected(),
							m_proxy);
	}
}

void
CClientProxyUnknown::removeTimer()
{
	if (m_timer != NULL) {
		m_events->deleteTimer(m_timer);
		m_events->removeHandler(CEvent::kTimer, this);
		m_timer = NULL;
	}
}

void
CClientProxyUnknown::handleData(const CEvent&, void*)
{
	LOG((CLOG_DEBUG1 "parsing hello reply"));

	CString name("<unknown>");
	try {
		// limit the maximum length of the hello
		UInt32 n = m_stream->getSize();
		if (n > kMaxHelloLength) {
			LOG((CLOG_DEBUG1 "hello reply too long"));
			throw XBadClient();
		}

		// parse the reply to hello
		SInt16 major, minor;
		if (!CProtocolUtil::readf(m_stream, kMsgHelloBack,
									&major, &minor, &name)) {
			throw XBadClient();
		}

		// disallow invalid version numbers
		if (major <= 0 || minor < 0) {
			throw XIncompatibleClient(major, minor);
		}

		// remove stream event handlers.  the proxy we're about to create
		// may install its own handlers and we don't want to accidentally
		// remove those later.
		removeHandlers();

		// create client proxy for highest version supported by the client
		if (major == 1) {
			switch (minor) {
			case 0:
				m_proxy = new CClientProxy1_0(name, m_stream, m_events);
				break;

			case 1:
				m_proxy = new CClientProxy1_1(name, m_stream, m_events);
				break;

			case 2:
				m_proxy = new CClientProxy1_2(name, m_stream, m_events);
				break;

			case 3:
				m_proxy = new CClientProxy1_3(name, m_stream, m_events);
				break;

			case 4:
				m_proxy = new CClientProxy1_4(name, m_stream, m_server, m_events);
				break;

			case 5:
				m_proxy = new CClientProxy1_5(name, m_stream, m_server, m_events);
				break;
			}
		}

		// hangup (with error) if version isn't supported
		if (m_proxy == NULL) {
			throw XIncompatibleClient(major, minor);
		}

		// the proxy is created and now proxy now owns the stream
		LOG((CLOG_DEBUG1 "created proxy for client \"%s\" version %d.%d", name.c_str(), major, minor));
		m_stream = NULL;

		// wait until the proxy signals that it's ready or has disconnected
		addProxyHandlers();
		return;
	}
	catch (XIncompatibleClient& e) {
		// client is incompatible
		LOG((CLOG_WARN "client \"%s\" has incompatible version %d.%d)", name.c_str(), e.getMajor(), e.getMinor()));
		CProtocolUtil::writef(m_stream,
							kMsgEIncompatible,
							kProtocolMajorVersion, kProtocolMinorVersion);
	}
	catch (XBadClient&) {
		// client not behaving
		LOG((CLOG_WARN "protocol error from client \"%s\"", name.c_str()));
		CProtocolUtil::writef(m_stream, kMsgEBad);
	}
	catch (XBase& e) {
		// misc error
		LOG((CLOG_WARN "error communicating with client \"%s\": %s", name.c_str(), e.what()));
	}
	sendFailure();
}

void
CClientProxyUnknown::handleWriteError(const CEvent&, void*)
{
	LOG((CLOG_NOTE "error communicating with new client"));
	sendFailure();
}

void
CClientProxyUnknown::handleTimeout(const CEvent&, void*)
{
	LOG((CLOG_NOTE "new client is unresponsive"));
	sendFailure();
}

void
CClientProxyUnknown::handleDisconnect(const CEvent&, void*)
{
	LOG((CLOG_NOTE "new client disconnected"));
	sendFailure();
}

void
CClientProxyUnknown::handleReady(const CEvent&, void*)
{
	sendSuccess();
}
