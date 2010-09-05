/*
 * synergy -- mouse and keyboard sharing utility
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
 */

#include "CClientProxyUnknown.h"
#include "CClientProxy1_0.h"
#include "CClientProxy1_1.h"
#include "CClientProxy1_2.h"
#include "ProtocolTypes.h"
#include "CProtocolUtil.h"
#include "XSynergy.h"
#include "IStream.h"
#include "XIO.h"
#include "CLog.h"
#include "CString.h"
#include "IEventQueue.h"
#include "TMethodEventJob.h"

//
// CClientProxyUnknown
//

CEvent::Type			CClientProxyUnknown::s_successEvent = CEvent::kUnknown;
CEvent::Type			CClientProxyUnknown::s_failureEvent = CEvent::kUnknown;

CClientProxyUnknown::CClientProxyUnknown(IStream* stream, double timeout) :
	m_stream(stream),
	m_proxy(NULL),
	m_ready(false)
{
	EVENTQUEUE->adoptHandler(CEvent::kTimer, this,
							new TMethodEventJob<CClientProxyUnknown>(this,
								&CClientProxyUnknown::handleTimeout, NULL));
	m_timer = EVENTQUEUE->newOneShotTimer(timeout, this);
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

CEvent::Type
CClientProxyUnknown::getSuccessEvent()
{
	return CEvent::registerTypeOnce(s_successEvent,
							"CClientProxy::success");
}

CEvent::Type
CClientProxyUnknown::getFailureEvent()
{
	return CEvent::registerTypeOnce(s_failureEvent,
							"CClientProxy::failure");
}

void
CClientProxyUnknown::sendSuccess()
{
	m_ready = true;
	removeTimer();
	EVENTQUEUE->addEvent(CEvent(getSuccessEvent(), this));
}

void
CClientProxyUnknown::sendFailure()
{
	delete m_proxy;
	m_proxy = NULL;
	m_ready = false;
	removeHandlers();
	removeTimer();
	EVENTQUEUE->addEvent(CEvent(getFailureEvent(), this));
}

void
CClientProxyUnknown::addStreamHandlers()
{
	assert(m_stream != NULL);

	EVENTQUEUE->adoptHandler(IStream::getInputReadyEvent(),
							m_stream->getEventTarget(),
							new TMethodEventJob<CClientProxyUnknown>(this,
								&CClientProxyUnknown::handleData));
	EVENTQUEUE->adoptHandler(IStream::getOutputErrorEvent(),
							m_stream->getEventTarget(),
							new TMethodEventJob<CClientProxyUnknown>(this,
								&CClientProxyUnknown::handleWriteError));
	EVENTQUEUE->adoptHandler(IStream::getInputShutdownEvent(),
							m_stream->getEventTarget(),
							new TMethodEventJob<CClientProxyUnknown>(this,
								&CClientProxyUnknown::handleDisconnect));
	EVENTQUEUE->adoptHandler(IStream::getOutputShutdownEvent(),
							m_stream->getEventTarget(),
							new TMethodEventJob<CClientProxyUnknown>(this,
								&CClientProxyUnknown::handleWriteError));
}

void
CClientProxyUnknown::addProxyHandlers()
{
	assert(m_proxy != NULL);

	EVENTQUEUE->adoptHandler(CClientProxy::getReadyEvent(),
							m_proxy,
							new TMethodEventJob<CClientProxyUnknown>(this,
								&CClientProxyUnknown::handleReady));
	EVENTQUEUE->adoptHandler(CClientProxy::getDisconnectedEvent(),
							m_proxy,
							new TMethodEventJob<CClientProxyUnknown>(this,
								&CClientProxyUnknown::handleDisconnect));
}

void
CClientProxyUnknown::removeHandlers()
{
	if (m_stream != NULL) {
		EVENTQUEUE->removeHandler(IStream::getInputReadyEvent(),
							m_stream->getEventTarget());
		EVENTQUEUE->removeHandler(IStream::getOutputErrorEvent(),
							m_stream->getEventTarget());
		EVENTQUEUE->removeHandler(IStream::getInputShutdownEvent(),
							m_stream->getEventTarget());
		EVENTQUEUE->removeHandler(IStream::getOutputShutdownEvent(),
							m_stream->getEventTarget());
	}
	if (m_proxy != NULL) {
		EVENTQUEUE->removeHandler(CClientProxy::getReadyEvent(),
							m_proxy);
		EVENTQUEUE->removeHandler(CClientProxy::getDisconnectedEvent(),
							m_proxy);
	}
}

void
CClientProxyUnknown::removeTimer()
{
	if (m_timer != NULL) {
		EVENTQUEUE->deleteTimer(m_timer);
		EVENTQUEUE->removeHandler(CEvent::kTimer, this);
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
				m_proxy = new CClientProxy1_0(name, m_stream);
				break;

			case 1:
				m_proxy = new CClientProxy1_1(name, m_stream);
				break;

			case 2:
				m_proxy = new CClientProxy1_2(name, m_stream);
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
