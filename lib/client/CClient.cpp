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
#include "ISecondaryScreenFactory.h"
#include "CClipboard.h"
#include "CInputPacketStream.h"
#include "COutputPacketStream.h"
#include "CProtocolUtil.h"
#include "CSecondaryScreen.h"
#include "IServer.h"
#include "ProtocolTypes.h"
#include "XScreen.h"
#include "XSynergy.h"
#include "IDataSocket.h"
#include "ISocketFactory.h"
#include "XSocket.h"
#include "IStreamFilterFactory.h"
#include "CLock.h"
#include "CThread.h"
#include "CTimerThread.h"
#include "XThread.h"
#include "CLog.h"
#include "CStopwatch.h"
#include "TMethodJob.h"

//
// CClient
//

CClient::CClient(const CString& clientName) :
	m_name(clientName),
	m_screen(NULL),
	m_server(NULL),
	m_camp(false),
	m_screenFactory(NULL),
	m_socketFactory(NULL),
	m_streamFilterFactory(NULL),
	m_session(NULL),
	m_active(false),
	m_rejected(true)
{
	// do nothing
}

CClient::~CClient()
{
	delete m_screenFactory;
	delete m_socketFactory;
	delete m_streamFilterFactory;
}

void
CClient::camp(bool on)
{
	CLock lock(&m_mutex);
	m_camp = on;
}

void
CClient::setAddress(const CNetworkAddress& serverAddress)
{
	CLock lock(&m_mutex);
	m_serverAddress = serverAddress;
}

void
CClient::setScreenFactory(ISecondaryScreenFactory* adopted)
{
	CLock lock(&m_mutex);
	delete m_screenFactory;
	m_screenFactory = adopted;
}

void
CClient::setSocketFactory(ISocketFactory* adopted)
{
	CLock lock(&m_mutex);
	delete m_socketFactory;
	m_socketFactory = adopted;
}

void
CClient::setStreamFilterFactory(IStreamFilterFactory* adopted)
{
	CLock lock(&m_mutex);
	delete m_streamFilterFactory;
	m_streamFilterFactory = adopted;
}

void
CClient::exitMainLoop()
{
	m_screen->exitMainLoop();
}

bool
CClient::wasRejected() const
{
	return m_rejected;
}

void
CClient::onError()
{
	// close down session but don't wait too long
	deleteSession(3.0);
}

void
CClient::onInfoChanged(const CClientInfo& info)
{
	log((CLOG_DEBUG "resolution changed"));

	CLock lock(&m_mutex);
	if (m_server != NULL) {
		m_server->onInfoChanged(info);
	}
}

bool
CClient::onGrabClipboard(ClipboardID id)
{
	CLock lock(&m_mutex);
	if (m_server == NULL) {
		// m_server can be NULL if the screen calls this method
		// before we've gotten around to connecting to the server.
		// we simply ignore the clipboard change in that case.
		return false;
	}

	// grab ownership
	m_server->onGrabClipboard(id);

	// we now own the clipboard and it has not been sent to the server
	m_ownClipboard[id]  = true;
	m_timeClipboard[id] = 0;

	// if we're not the active screen then send the clipboard now,
	// otherwise we'll wait until we leave.
	if (!m_active) {
		sendClipboard(id);
	}

	return true;
}

void
CClient::onClipboardChanged(ClipboardID, const CString&)
{
	// ignore -- we'll check the clipboard when we leave
}

void
CClient::open()
{
	// open the screen
	try {
		log((CLOG_INFO "opening screen"));
		openSecondaryScreen();
	}
	catch (XScreenOpenFailure&) {
		// can't open screen
		log((CLOG_INFO "failed to open screen"));
		throw;
	}
}

void
CClient::mainLoop()
{
	{
		CLock lock(&m_mutex);

		// check preconditions
		assert(m_screen != NULL);
		assert(m_server == NULL);

		// connection starts as unsuccessful
		m_rejected = true;
	}

	try {
		log((CLOG_NOTE "starting client \"%s\"", m_name.c_str()));

		// start server interactions
		{
			CLock lock(&m_mutex);
			m_session = new CThread(new TMethodJob<CClient>(
								this, &CClient::runSession));
		}

		// handle events
		m_screen->mainLoop();

		// clean up
		deleteSession();
		log((CLOG_NOTE "stopping client \"%s\"", m_name.c_str()));
	}
	catch (XBase& e) {
		log((CLOG_ERR "client error: %s", e.what()));

		// clean up
		deleteSession();
		log((CLOG_NOTE "stopping client \"%s\"", m_name.c_str()));
		CLock lock(&m_mutex);
		m_rejected = false;
	}
	catch (XThread&) {
		// clean up
		deleteSession();
		log((CLOG_NOTE "stopping client \"%s\"", m_name.c_str()));
		throw;
	}
	catch (...) {
		log((CLOG_DEBUG "unknown client error"));

		// clean up
		deleteSession();
		log((CLOG_NOTE "stopping client \"%s\"", m_name.c_str()));
		throw;
	}
}

void
CClient::close()
{
	closeSecondaryScreen();
	log((CLOG_INFO "closed screen"));
}

void
CClient::enter(SInt32 xAbs, SInt32 yAbs, UInt32, KeyModifierMask mask, bool)
{
	{
		CLock lock(&m_mutex);
		m_active = true;
	}

	m_screen->enter(xAbs, yAbs, mask);
}

bool
CClient::leave()
{
	m_screen->leave();

	CLock lock(&m_mutex);
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
CClient::setClipboard(ClipboardID id, const CString& data)
{
 	// unmarshall
 	CClipboard clipboard;
 	clipboard.unmarshall(data, 0);
 
 	// set screen's clipboard
 	m_screen->setClipboard(id, &clipboard);
}

void
CClient::grabClipboard(ClipboardID id)
{
	// we no longer own the clipboard
	{
		CLock lock(&m_mutex);
		m_ownClipboard[id] = false;
	}

	m_screen->grabClipboard(id);
}

void
CClient::setClipboardDirty(ClipboardID, bool)
{
	assert(0 && "shouldn't be called");
}

void
CClient::keyDown(KeyID id, KeyModifierMask mask)
{
 	m_screen->keyDown(id, mask);
}

void
CClient::keyRepeat(KeyID id, KeyModifierMask mask, SInt32 count)
{
 	m_screen->keyRepeat(id, mask, count);
}

void
CClient::keyUp(KeyID id, KeyModifierMask mask)
{
 	m_screen->keyUp(id, mask);
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
CClient::mouseWheel(SInt32 delta)
{
	m_screen->mouseWheel(delta);
}

void
CClient::screensaver(bool activate)
{
 	m_screen->screensaver(activate);
}

CString
CClient::getName() const
{
	return m_name;
}

SInt32
CClient::getJumpZoneSize() const
{
	return m_screen->getJumpZoneSize();
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
CClient::getCursorCenter(SInt32&, SInt32&) const
{
	assert(0 && "shouldn't be called");
}

void
CClient::openSecondaryScreen()
{
	assert(m_screen == NULL);

	// not active
	m_active = false;

	// reset clipboard state
	for (ClipboardID id = 0; id < kClipboardEnd; ++id) {
		m_ownClipboard[id]  = false;
		m_timeClipboard[id] = 0;
	}

	// create screen
	log((CLOG_DEBUG1 "creating secondary screen"));
	if (m_screenFactory != NULL) {
		m_screen = m_screenFactory->create(this);
	}
	if (m_screen == NULL) {
		throw XScreenOpenFailure();
	}

	// open screen
	try {
		log((CLOG_DEBUG1 "opening secondary screen"));
		m_screen->open();
	}
	catch (...) {
		log((CLOG_DEBUG1 "destroying secondary screen"));
		delete m_screen;
		m_screen = NULL;
		throw;
	}
}

void
CClient::closeSecondaryScreen()
{
	// close the secondary screen
	try {
		if (m_screen != NULL) {
			log((CLOG_DEBUG1 "closing secondary screen"));
			m_screen->close();
		}
	}
	catch (...) {
		// ignore
	}

	// clean up
	log((CLOG_DEBUG1 "destroying secondary screen"));
	delete m_screen;
	m_screen = NULL;
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
			m_server->onClipboardChanged(id, data);
		}
	}
}

void
CClient::runSession(void*)
{
	try {
		log((CLOG_DEBUG "starting server proxy"));
		runServer();
		m_screen->exitMainLoop();
		log((CLOG_DEBUG "stopping server proxy"));
	}
	catch (...) {
		m_screen->exitMainLoop();
		log((CLOG_DEBUG "stopping server proxy"));
		throw;
	}
}

void
CClient::deleteSession(double timeout)
{
	// get session thread object
	CThread* thread;
	{
		CLock lock(&m_mutex);
		thread    = m_session;
		m_session = NULL;
	}

	// shut it down
	if (thread != NULL) {
		thread->cancel();
		thread->wait(timeout);
		delete thread;
	}
}

void
CClient::runServer()
{
	IDataSocket* socket = NULL;
	CServerProxy* proxy = NULL;
	try {
		for (;;) {
			try {
				// allow connect this much time to succeed
				CTimerThread timer(m_camp ? -1.0 : 30.0);

				// create socket and attempt to connect to server
				log((CLOG_DEBUG1 "connecting to server"));
				if (m_socketFactory != NULL) {
					socket = m_socketFactory->create();
				}
				assert(socket != NULL);
				socket->connect(m_serverAddress);
				log((CLOG_INFO "connected to server"));
				break;
			}
			catch (XSocketConnect& e) {
				log((CLOG_DEBUG1 "failed to connect to server: %s", e.getErrstr()));

				// failed to connect.  if not camping then rethrow.
				if (!m_camp) {
					throw;
				}

				// we're camping.  wait a bit before retrying
				CThread::sleep(5.0);
			}
		}

		// create proxy
		log((CLOG_DEBUG1 "negotiating with server"));
		proxy = handshakeServer(socket);
		CLock lock(&m_mutex);
		m_server = proxy;
	}
	catch (XThread&) {
		log((CLOG_ERR "connection timed out"));
		delete socket;
		throw;
	}
	catch (XBase& e) {
		log((CLOG_ERR "connection failed: %s", e.what()));
		log((CLOG_DEBUG "disconnecting from server"));
		delete socket;
		return;
	}
	catch (...) {
		log((CLOG_ERR "connection failed: <unknown error>"));
		log((CLOG_DEBUG "disconnecting from server"));
		delete socket;
		return;
	}

	try {
		// process messages
		bool rejected = true;
		if (proxy != NULL) {
			log((CLOG_DEBUG1 "communicating with server"));
			rejected = !proxy->mainLoop();
		}

		// clean up
		CLock lock(&m_mutex);
		m_rejected = rejected;
		m_server   = NULL;
		delete proxy;
		log((CLOG_DEBUG "disconnecting from server"));
		socket->close();
		delete socket;
	}
	catch (...) {
		CLock lock(&m_mutex);
		m_rejected = false;
		m_server   = NULL;
		delete proxy;
		log((CLOG_DEBUG "disconnecting from server"));
		socket->close();
		delete socket;
		throw;
	}
}

CServerProxy*
CClient::handshakeServer(IDataSocket* socket)
{
	// get the input and output streams
	IInputStream*  input  = socket->getInputStream();
	IOutputStream* output = socket->getOutputStream();
	bool own              = false;

	// attach filters
	if (m_streamFilterFactory != NULL) {
		input  = m_streamFilterFactory->createInput(input, own);
		output = m_streamFilterFactory->createOutput(output, own);
		own    = true;
	}

	// attach the packetizing filters
	input  = new CInputPacketStream(input, own);
	output = new COutputPacketStream(output, own);
	own    = true;

	CServerProxy* proxy = NULL;
	try {
		// give handshake some time
		CTimerThread timer(30.0);

		// wait for hello from server
		log((CLOG_DEBUG1 "wait for hello"));
		SInt16 major, minor;
		CProtocolUtil::readf(input, kMsgHello, &major, &minor);

		// check versions
		log((CLOG_DEBUG1 "got hello version %d.%d", major, minor));
		if (major < kProtocolMajorVersion ||
			(major == kProtocolMajorVersion && minor < kProtocolMinorVersion)) {
			throw XIncompatibleClient(major, minor);
		}

		// say hello back
		log((CLOG_DEBUG1 "say hello version %d.%d", kProtocolMajorVersion, kProtocolMinorVersion));
		CProtocolUtil::writef(output, kMsgHelloBack,
								kProtocolMajorVersion,
								kProtocolMinorVersion, &m_name);

		// create server proxy
		proxy = new CServerProxy(this, input, output);

		// negotiate
		// FIXME

		return proxy;
	}
	catch (XIncompatibleClient& e) {
		log((CLOG_ERR "server has incompatible version %d.%d", e.getMajor(), e.getMinor()));
	}
	catch (XBase& e) {
		log((CLOG_WARN "error communicating with server: %s", e.what()));
	}
	catch (...) {
		// probably timed out
		if (proxy != NULL) {
			delete proxy;
		}
		else if (own) {
			delete input;
			delete output;
		}
		throw;
	}

	// failed
	if (proxy != NULL) {
		delete proxy;
	}
	else if (own) {
		delete input;
		delete output;
	}

	return NULL;
}
