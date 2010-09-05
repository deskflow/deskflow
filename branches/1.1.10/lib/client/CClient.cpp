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

#include "CClient.h"
#include "CServerProxy.h"
#include "CScreen.h"
#include "CClipboard.h"
#include "CPacketStreamFilter.h"
#include "CProtocolUtil.h"
#include "ProtocolTypes.h"
#include "XSynergy.h"
#include "IDataSocket.h"
#include "ISocketFactory.h"
#include "IStreamFilterFactory.h"
#include "CLog.h"
#include "IEventQueue.h"
#include "TMethodEventJob.h"

//
// CClient
//

CEvent::Type			CClient::s_connectedEvent        = CEvent::kUnknown;
CEvent::Type			CClient::s_connectionFailedEvent = CEvent::kUnknown;
CEvent::Type			CClient::s_disconnectedEvent     = CEvent::kUnknown;

CClient::CClient(const CString& name, const CNetworkAddress& address,
				ISocketFactory* socketFactory,
				IStreamFilterFactory* streamFilterFactory,
				CScreen* screen) :
	m_name(name),
	m_serverAddress(address),
	m_socketFactory(socketFactory),
	m_streamFilterFactory(streamFilterFactory),
	m_screen(screen),
	m_stream(NULL),
	m_timer(NULL),
	m_server(NULL),
	
	m_active(false)
{
	assert(m_socketFactory != NULL);
	assert(m_screen        != NULL);

	// do nothing
}

CClient::~CClient()
{
	cleanupTimer();
	cleanupScreen();
	cleanupConnecting();
	cleanupConnection();
	delete m_socketFactory;
	delete m_streamFilterFactory;
}

void
CClient::connect()
{
	if (m_stream != NULL) {
		return;
	}

	try {
		IDataSocket* socket = m_socketFactory->create();

		// filter socket messages, including a packetizing filter
		m_stream = socket;
		if (m_streamFilterFactory != NULL) {
			m_stream = m_streamFilterFactory->create(m_stream, true);
		}
		m_stream = new CPacketStreamFilter(m_stream, true);

		// connect
		LOG((CLOG_DEBUG1 "connecting to server"));
		setupConnecting();
		setupTimer();
		socket->connect(m_serverAddress);
	}
	catch (XBase& e) {
		cleanupTimer();
		cleanupConnecting();
		delete m_stream;
		m_stream = NULL;
		LOG((CLOG_DEBUG1 "connection failed"));
		sendConnectionFailedEvent(e.what());
		return;
	}
}

void
CClient::disconnect(const char* msg)
{
	cleanupTimer();
	cleanupScreen();
	cleanupConnection();
	if (msg != NULL) {
		sendConnectionFailedEvent(msg);
	}
	else {
		sendEvent(getDisconnectedEvent(), NULL);
	}
}

void
CClient::handshakeComplete()
{
	m_ready = true;
	m_screen->enable();
	sendEvent(getConnectedEvent(), NULL);
}

bool
CClient::isConnected() const
{
	return (m_server != NULL);
}

bool
CClient::isConnecting() const
{
	return (m_timer != NULL);
}

CEvent::Type
CClient::getConnectedEvent()
{
	return CEvent::registerTypeOnce(s_connectedEvent,
							"CClient::connected");
}

CEvent::Type
CClient::getConnectionFailedEvent()
{
	return CEvent::registerTypeOnce(s_connectionFailedEvent,
							"CClient::failed");
}

CEvent::Type
CClient::getDisconnectedEvent()
{
	return CEvent::registerTypeOnce(s_disconnectedEvent,
							"CClient::disconnected");
}

void*
CClient::getEventTarget() const
{
	return m_screen->getEventTarget();
}

bool
CClient::getClipboard(ClipboardID id, IClipboard* clipboard) const
{
	return m_screen->getClipboard(id, clipboard);
}

void
CClient::getShape(SInt32& x, SInt32& y, SInt32& w, SInt32& h) const
{
	m_screen->getShape(x, y, w, h);
}

void
CClient::getCursorPos(SInt32& x, SInt32& y) const
{
	m_screen->getCursorPos(x, y);
}

void
CClient::enter(SInt32 xAbs, SInt32 yAbs, UInt32, KeyModifierMask mask, bool)
{
	m_active = true;
	m_screen->mouseMove(xAbs, yAbs);
	m_screen->enter(mask);
}

bool
CClient::leave()
{
	m_screen->leave();

	m_active = false;

	// send clipboards that we own and that have changed
	for (ClipboardID id = 0; id < kClipboardEnd; ++id) {
		if (m_ownClipboard[id]) {
			sendClipboard(id);
		}
	}

	return true;
}

void
CClient::setClipboard(ClipboardID id, const IClipboard* clipboard)
{
 	m_screen->setClipboard(id, clipboard);
}

void
CClient::grabClipboard(ClipboardID id)
{
	m_screen->grabClipboard(id);
	m_ownClipboard[id] = false;
}

void
CClient::setClipboardDirty(ClipboardID, bool)
{
	assert(0 && "shouldn't be called");
}

void
CClient::keyDown(KeyID id, KeyModifierMask mask, KeyButton button)
{
 	m_screen->keyDown(id, mask, button);
}

void
CClient::keyRepeat(KeyID id, KeyModifierMask mask,
				SInt32 count, KeyButton button)
{
 	m_screen->keyRepeat(id, mask, count, button);
}

void
CClient::keyUp(KeyID id, KeyModifierMask mask, KeyButton button)
{
 	m_screen->keyUp(id, mask, button);
}

void
CClient::mouseDown(ButtonID id)
{
 	m_screen->mouseDown(id);
}

void
CClient::mouseUp(ButtonID id)
{
 	m_screen->mouseUp(id);
}

void
CClient::mouseMove(SInt32 x, SInt32 y)
{
	m_screen->mouseMove(x, y);
}

void
CClient::mouseRelativeMove(SInt32 dx, SInt32 dy)
{
	m_screen->mouseRelativeMove(dx, dy);
}

void
CClient::mouseWheel(SInt32 delta)
{
	m_screen->mouseWheel(delta);
}

void
CClient::screensaver(bool activate)
{
 	m_screen->screensaver(activate);
}

void
CClient::resetOptions()
{
	m_screen->resetOptions();
}

void
CClient::setOptions(const COptionsList& options)
{
	m_screen->setOptions(options);
}

CString
CClient::getName() const
{
	return m_name;
}

void
CClient::sendClipboard(ClipboardID id)
{
	// note -- m_mutex must be locked on entry
	assert(m_screen != NULL);
	assert(m_server != NULL);

	// get clipboard data.  set the clipboard time to the last
	// clipboard time before getting the data from the screen
	// as the screen may detect an unchanged clipboard and
	// avoid copying the data.
	CClipboard clipboard;
	if (clipboard.open(m_timeClipboard[id])) {
		clipboard.close();
	}
	m_screen->getClipboard(id, &clipboard);

	// check time
	if (m_timeClipboard[id] == 0 ||
		clipboard.getTime() != m_timeClipboard[id]) {
		// save new time
		m_timeClipboard[id] = clipboard.getTime();

		// marshall the data
		CString data = clipboard.marshall();

		// save and send data if different
		if (data != m_dataClipboard[id]) {
			m_dataClipboard[id] = data;
			m_server->onClipboardChanged(id, &clipboard);
		}
	}
}

void
CClient::sendEvent(CEvent::Type type, void* data)
{
	EVENTQUEUE->addEvent(CEvent(type, getEventTarget(), data));
}

void
CClient::sendConnectionFailedEvent(const char* msg)
{
	CFailInfo* info = (CFailInfo*)malloc(sizeof(CFailInfo) + strlen(msg));
	info->m_retry   = true;
	strcpy(info->m_what, msg);
	sendEvent(getConnectionFailedEvent(), info);
}

void
CClient::setupConnecting()
{
	assert(m_stream != NULL);

	EVENTQUEUE->adoptHandler(IDataSocket::getConnectedEvent(),
							m_stream->getEventTarget(),
							new TMethodEventJob<CClient>(this,
								&CClient::handleConnected));
	EVENTQUEUE->adoptHandler(IDataSocket::getConnectionFailedEvent(),
							m_stream->getEventTarget(),
							new TMethodEventJob<CClient>(this,
								&CClient::handleConnectionFailed));
}

void
CClient::setupConnection()
{
	assert(m_stream != NULL);

	EVENTQUEUE->adoptHandler(ISocket::getDisconnectedEvent(),
							m_stream->getEventTarget(),
							new TMethodEventJob<CClient>(this,
								&CClient::handleDisconnected));
	EVENTQUEUE->adoptHandler(IStream::getInputReadyEvent(),
							m_stream->getEventTarget(),
							new TMethodEventJob<CClient>(this,
								&CClient::handleHello));
	EVENTQUEUE->adoptHandler(IStream::getOutputErrorEvent(),
							m_stream->getEventTarget(),
							new TMethodEventJob<CClient>(this,
								&CClient::handleOutputError));
	EVENTQUEUE->adoptHandler(IStream::getInputShutdownEvent(),
							m_stream->getEventTarget(),
							new TMethodEventJob<CClient>(this,
								&CClient::handleDisconnected));
	EVENTQUEUE->adoptHandler(IStream::getOutputShutdownEvent(),
							m_stream->getEventTarget(),
							new TMethodEventJob<CClient>(this,
								&CClient::handleDisconnected));
}

void
CClient::setupScreen()
{
	assert(m_server == NULL);

	m_ready  = false;
	m_server = new CServerProxy(this, m_stream);
	EVENTQUEUE->adoptHandler(IScreen::getShapeChangedEvent(),
							getEventTarget(),
							new TMethodEventJob<CClient>(this,
								&CClient::handleShapeChanged));
	EVENTQUEUE->adoptHandler(IScreen::getClipboardGrabbedEvent(),
							getEventTarget(),
							new TMethodEventJob<CClient>(this,
								&CClient::handleClipboardGrabbed));
}

void
CClient::setupTimer()
{
	assert(m_timer == NULL);

	m_timer = EVENTQUEUE->newOneShotTimer(15.0, NULL);
	EVENTQUEUE->adoptHandler(CEvent::kTimer, m_timer,
							new TMethodEventJob<CClient>(this,
								&CClient::handleConnectTimeout));
}

void
CClient::cleanupConnecting()
{
	if (m_stream != NULL) {
		EVENTQUEUE->removeHandler(IDataSocket::getConnectedEvent(),
							m_stream->getEventTarget());
		EVENTQUEUE->removeHandler(IDataSocket::getConnectionFailedEvent(),
							m_stream->getEventTarget());
	}
}

void
CClient::cleanupConnection()
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
		EVENTQUEUE->removeHandler(ISocket::getDisconnectedEvent(),
							m_stream->getEventTarget());
		delete m_stream;
		m_stream = NULL;
	}
}

void
CClient::cleanupScreen()
{
	if (m_server != NULL) {
		if (m_ready) {
			m_screen->disable();
			m_ready = false;
		}
		EVENTQUEUE->removeHandler(IScreen::getShapeChangedEvent(),
							getEventTarget());
		EVENTQUEUE->removeHandler(IScreen::getClipboardGrabbedEvent(),
							getEventTarget());
		delete m_server;
		m_server = NULL;
	}
}

void
CClient::cleanupTimer()
{
	if (m_timer != NULL) {
		EVENTQUEUE->removeHandler(CEvent::kTimer, m_timer);
		EVENTQUEUE->deleteTimer(m_timer);
		m_timer = NULL;
	}
}

void
CClient::handleConnected(const CEvent&, void*)
{
	LOG((CLOG_DEBUG1 "connected;  wait for hello"));
	cleanupConnecting();
	setupConnection();

	// reset clipboard state
	for (ClipboardID id = 0; id < kClipboardEnd; ++id) {
		m_ownClipboard[id]  = false;
		m_timeClipboard[id] = 0;
	}
}

void
CClient::handleConnectionFailed(const CEvent& event, void*)
{
	IDataSocket::CConnectionFailedInfo* info =
		reinterpret_cast<IDataSocket::CConnectionFailedInfo*>(event.getData());

	cleanupTimer();
	cleanupConnecting();
	delete m_stream;
	m_stream = NULL;
	LOG((CLOG_DEBUG1 "connection failed"));
	sendConnectionFailedEvent(info->m_what);
}

void
CClient::handleConnectTimeout(const CEvent&, void*)
{
	cleanupTimer();
	cleanupConnecting();
	delete m_stream;
	m_stream = NULL;
	LOG((CLOG_DEBUG1 "connection timed out"));
	sendConnectionFailedEvent("Timed out");
}

void
CClient::handleOutputError(const CEvent&, void*)
{
	cleanupTimer();
	cleanupScreen();
	cleanupConnection();
	LOG((CLOG_WARN "error sending to server"));
	sendEvent(getDisconnectedEvent(), NULL);
}

void
CClient::handleDisconnected(const CEvent&, void*)
{
	cleanupTimer();
	cleanupScreen();
	cleanupConnection();
	LOG((CLOG_DEBUG1 "disconnected"));
	sendEvent(getDisconnectedEvent(), NULL);
}

void
CClient::handleShapeChanged(const CEvent&, void*)
{
	LOG((CLOG_DEBUG "resolution changed"));
	m_server->onInfoChanged();
}

void
CClient::handleClipboardGrabbed(const CEvent& event, void*)
{
	const IScreen::CClipboardInfo* info =
		reinterpret_cast<const IScreen::CClipboardInfo*>(event.getData());

	// grab ownership
	m_server->onGrabClipboard(info->m_id);

	// we now own the clipboard and it has not been sent to the server
	m_ownClipboard[info->m_id]  = true;
	m_timeClipboard[info->m_id] = 0;

	// if we're not the active screen then send the clipboard now,
	// otherwise we'll wait until we leave.
	if (!m_active) {
		sendClipboard(info->m_id);
	}
}

void
CClient::handleHello(const CEvent&, void*)
{
	SInt16 major, minor;
	if (!CProtocolUtil::readf(m_stream, kMsgHello, &major, &minor)) {
		sendConnectionFailedEvent("Protocol error from server");
		cleanupTimer();
		cleanupConnection();
		return;
	}

	// check versions
	LOG((CLOG_DEBUG1 "got hello version %d.%d", major, minor));
	if (major < kProtocolMajorVersion ||
		(major == kProtocolMajorVersion && minor < kProtocolMinorVersion)) {
		sendConnectionFailedEvent(XIncompatibleClient(major, minor).what());
		cleanupTimer();
		cleanupConnection();
		return;
	}

	// say hello back
	LOG((CLOG_DEBUG1 "say hello version %d.%d", kProtocolMajorVersion, kProtocolMinorVersion));
	CProtocolUtil::writef(m_stream, kMsgHelloBack,
							kProtocolMajorVersion,
							kProtocolMinorVersion, &m_name);

	// now connected but waiting to complete handshake
	setupScreen();
	cleanupTimer();

	// make sure we process any remaining messages later.  we won't
	// receive another event for already pending messages so we fake
	// one.
	if (m_stream->isReady()) {
		EVENTQUEUE->addEvent(CEvent(IStream::getInputReadyEvent(),
							m_stream->getEventTarget()));
	}
}
