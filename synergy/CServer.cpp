#include "CServer.h"
#include "CInputPacketStream.h"
#include "COutputPacketStream.h"
#include "CServerProtocol.h"
#include "CProtocolUtil.h"
#include "IPrimaryScreen.h"
#include "ISocketFactory.h"
#include "ProtocolTypes.h"
#include "XSynergy.h"
#include "CNetworkAddress.h"
#include "ISocket.h"
#include "IListenSocket.h"
#include "XSocket.h"
#include "CLock.h"
#include "CThread.h"
#include "CTimerThread.h"
#include "CStopwatch.h"
#include "TMethodJob.h"
#include "CLog.h"
#include <assert.h>
#include <memory>

/* XXX
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
if (fork() == 0) abort();
else { wait(0); exit(1); }
*/

//
// CServer
//

CServer::CServer() : m_done(&m_mutex, false),
								m_primary(NULL),
								m_active(NULL),
								m_primaryInfo(NULL)
{
	m_socketFactory = NULL;
	m_securityFactory = NULL;
	m_bindTimeout = 5.0 * 60.0;
}

CServer::~CServer()
{
}

void					CServer::run()
{
	try {
		log((CLOG_NOTE "starting server"));

		// connect to primary screen
		openPrimaryScreen();

		// start listening for new clients
		CThread(new TMethodJob<CServer>(this, &CServer::acceptClients));

		// start listening for configuration connections
		// FIXME

		// wait until done
		log((CLOG_DEBUG "waiting for quit"));
		CLock lock(&m_mutex);
		while (m_done == false) {
			m_done.wait();
		}

		// clean up
		log((CLOG_DEBUG "stopping server"));
		closePrimaryScreen();
		cleanupThreads();
	}
	catch (XBase& e) {
		log((CLOG_ERR "server error: %s\n", e.what()));

		// clean up
		closePrimaryScreen();
		cleanupThreads();
	}
	catch (...) {
		log((CLOG_DEBUG "server shutdown"));

		// clean up
		closePrimaryScreen();
		cleanupThreads();
		throw;
	}
}

void					CServer::setScreenMap(const CScreenMap& screenMap)
{
	CLock lock(&m_mutex);
	// FIXME -- must disconnect screens no longer listed
	//   (that may include warping back to server's screen)
	// FIXME -- server screen must be in new map or map is rejected
	m_screenMap = screenMap;
}

void					CServer::getScreenMap(CScreenMap* screenMap) const
{
	assert(screenMap != NULL);

	CLock lock(&m_mutex);
	*screenMap = m_screenMap;
}

void					CServer::setInfo(const CString& client,
								SInt32 w, SInt32 h, SInt32 zoneSize)
{
	CLock lock(&m_mutex);

	// client must be connected
	CScreenList::iterator index = m_screens.find(client);
	if (index == m_screens.end()) {
		throw XBadClient();
	}

	// update client info
	CScreenInfo* info = index->second;
	if (info == m_active) {
		// FIXME -- ensure mouse is still on screen.  warp it if necessary.
	}
	info->m_width    = w;
	info->m_height   = h;
	info->m_zoneSize = zoneSize;
	log((CLOG_NOTE "client \"%s\" size=%dx%d zone=%d", client.c_str(), w, h, zoneSize));
}

bool					CServer::onCommandKey(KeyID /*id*/,
								KeyModifierMask /*mask*/, bool /*down*/)
{
	return false;
}

void					CServer::onKeyDown(KeyID id, KeyModifierMask mask)
{
	log((CLOG_DEBUG "onKeyDown id=%d mask=0x%04x", id, mask));
	assert(m_active != NULL);

	// handle command keys
	if (onCommandKey(id, mask, true)) {
		return;
	}

	// relay
	if (m_active->m_protocol != NULL) {
		m_active->m_protocol->sendKeyDown(id, mask);
	}
}

void					CServer::onKeyUp(KeyID id, KeyModifierMask mask)
{
	log((CLOG_DEBUG "onKeyUp id=%d mask=0x%04x", id, mask));
	assert(m_active != NULL);

	// handle command keys
	if (onCommandKey(id, mask, false)) {
		return;
	}

	// relay
	if (m_active->m_protocol != NULL) {
		m_active->m_protocol->sendKeyUp(id, mask);
	}
}

void					CServer::onKeyRepeat(KeyID id, KeyModifierMask mask)
{
	log((CLOG_DEBUG "onKeyRepeat id=%d mask=0x%04x", id, mask));
	assert(m_active != NULL);

	// handle command keys
	if (onCommandKey(id, mask, false)) {
		onCommandKey(id, mask, true);
		return;
	}

	// relay
	if (m_active->m_protocol != NULL) {
		m_active->m_protocol->sendKeyRepeat(id, mask);
	}
}

void					CServer::onMouseDown(ButtonID id)
{
	log((CLOG_DEBUG "onMouseDown id=%d", id));
	assert(m_active != NULL);

	// relay
	if (m_active->m_protocol != NULL) {
		m_active->m_protocol->sendMouseDown(id);
	}
}

void					CServer::onMouseUp(ButtonID id)
{
	log((CLOG_DEBUG "onMouseUp id=%d", id));
	assert(m_active != NULL);

	// relay
	if (m_active->m_protocol != NULL) {
		m_active->m_protocol->sendMouseUp(id);
	}
}

void					CServer::onMouseMovePrimary(SInt32 x, SInt32 y)
{
	log((CLOG_DEBUG "onMouseMovePrimary %d,%d", x, y));

	// mouse move on primary (server's) screen
	assert(m_active != NULL);
	assert(m_active->m_protocol == NULL);

	// ignore if mouse is locked to screen
	if (isLockedToScreen()) {
		return;
	}

	// see if we should change screens
	CScreenMap::EDirection dir;
	if (x < m_active->m_zoneSize) {
		x  -= m_active->m_zoneSize;
		dir = CScreenMap::kLeft;
		log((CLOG_DEBUG "switch to left"));
	}
	else if (x >= m_active->m_width - m_active->m_zoneSize) {
		x  += m_active->m_zoneSize;
		dir = CScreenMap::kRight;
		log((CLOG_DEBUG "switch to right"));
	}
	else if (y < m_active->m_zoneSize) {
		y  -= m_active->m_zoneSize;
		dir = CScreenMap::kTop;
		log((CLOG_DEBUG "switch to top"));
	}
	else if (y >= m_active->m_height - m_active->m_zoneSize) {
		y  += m_active->m_zoneSize;
		dir = CScreenMap::kBottom;
		log((CLOG_DEBUG "switch to bottom"));
	}
	else {
		// still on local screen
		return;
	}

	// get jump destination
	CScreenInfo* newScreen = getNeighbor(m_active, dir, x, y);

	// if no screen in jump direction then ignore the move
	if (newScreen == NULL) {
		return;
	}

	// remap position to account for resolution differences
	mapPosition(m_active, dir, newScreen, x, y);

	// switch screen
	switchScreen(newScreen, x, y);
}

void					CServer::onMouseMoveSecondary(SInt32 dx, SInt32 dy)
{
	log((CLOG_DEBUG "onMouseMoveSecondary %+d,%+d", dx, dy));

	// mouse move on secondary (client's) screen
	assert(m_active != NULL);
	assert(m_active->m_protocol != NULL);

	// save old position
	const SInt32 xOld = m_x;
	const SInt32 yOld = m_y;

	// accumulate motion
	m_x += dx;
	m_y += dy;

	// switch screens if the mouse is outside the screen and not
	// locked to the screen
	CScreenInfo* newScreen = NULL;
	if (!isLockedToScreen()) {
		// find direction of neighbor
		CScreenMap::EDirection dir;
		if (m_x < 0)
			dir = CScreenMap::kLeft;
		else if (m_x > m_active->m_width - 1)
			dir = CScreenMap::kRight;
		else if (m_y < 0)
			dir = CScreenMap::kTop;
		else if (m_y > m_active->m_height - 1)
			dir = CScreenMap::kBottom;
		else
			newScreen = m_active;

		// get neighbor if we should switch
		if (newScreen == NULL) {
			log((CLOG_DEBUG "leave \"%s\" on %s", m_active->m_name.c_str(), CScreenMap::dirName(dir)));

			SInt32 x = m_x, y = m_y;
			newScreen = getNeighbor(m_active, dir, x, y);

			// remap position to account for resolution differences
			if (newScreen != NULL) {
				mapPosition(m_active, dir, newScreen, x, y);
				m_x = x;
				m_y = y;
			}
			else {
				log((CLOG_DEBUG "no neighbor; clamping"));
				if (m_x < 0)
					m_x = 0;
				else if (m_x > m_active->m_width - 1)
					m_x = m_active->m_width - 1;
				if (m_y < 0)
					m_y = 0;
				else if (m_y > m_active->m_height - 1)
					m_y = m_active->m_height - 1;
			}
		}
	}
	else {
		// clamp to edge when locked
		log((CLOG_DEBUG "clamp to \"%s\"", m_active->m_name.c_str()));
		if (m_x < 0)
			m_x = 0;
		else if (m_x > m_active->m_width - 1)
			m_x = m_active->m_width - 1;
		if (m_y < 0)
			m_y = 0;
		else if (m_y > m_active->m_height - 1)
			m_y = m_active->m_height - 1;
	}

	// warp cursor if on same screen
	if (newScreen == NULL || newScreen == m_active) {
		// do nothing if mouse didn't move
		if (m_x != xOld || m_y != yOld) {
			log((CLOG_DEBUG "move on %s to %d,%d", m_active->m_name.c_str(), m_x, m_y));
			m_active->m_protocol->sendMouseMove(m_x, m_y);
		}
	}

	// otherwise screen screens
	else {
		switchScreen(newScreen, m_x, m_y);
	}
}

void					CServer::onMouseWheel(SInt32 delta)
{
	log((CLOG_DEBUG "onMouseWheel %+d", delta));
	assert(m_active != NULL);

	// relay
	if (m_active->m_protocol != NULL) {
		m_active->m_protocol->sendMouseWheel(delta);
	}
}

bool					CServer::isLockedToScreen() const
{
	// FIXME
	return false;
}

void					CServer::switchScreen(CScreenInfo* dst,
								SInt32 x, SInt32 y)
{
	assert(dst != NULL);
	assert(x >= 0 && y >= 0 && x < dst->m_width && y < dst->m_height);
	assert(m_active != NULL);

	log((CLOG_NOTE "switch from \"%s\" to \"%s\" at %d,%d", m_active->m_name.c_str(), dst->m_name.c_str(), x, y));

	// wrapping means leaving the active screen and entering it again.
	// since that's a waste of time we skip that and just warp the
	// mouse.
	if (m_active != dst) {
		// leave active screen
		if (m_active->m_protocol == NULL) {
			m_primary->leave();
		}
		else {
			m_active->m_protocol->sendLeave();
		}

		// cut over
		m_active = dst;

		// enter new screen
		if (m_active->m_protocol == NULL) {
			m_primary->enter(x, y);
		}
		else {
			m_active->m_protocol->sendEnter(x, y);
		}
	}
	else {
		if (m_active->m_protocol == NULL) {
			m_primary->warpCursor(x, y);
		}
		else {
			m_active->m_protocol->sendMouseMove(x, y);
		}
	}

	// record new position
	m_x = x;
	m_y = y;
}

CServer::CScreenInfo*	CServer::getNeighbor(CScreenInfo* src,
								CScreenMap::EDirection dir) const
{
	assert(src != NULL);

	CString srcName = src->m_name;
	assert(!srcName.empty());
	log((CLOG_DEBUG "find neighbor on %s of \"%s\"", CScreenMap::dirName(dir), srcName.c_str()));
	for (;;) {
		// look up name of neighbor
		const CString dstName(m_screenMap.getNeighbor(srcName, dir));

		// if nothing in that direction then return NULL
		if (dstName.empty()) {
			log((CLOG_DEBUG "no neighbor on %s of \"%s\"", CScreenMap::dirName(dir), srcName.c_str()));
			return NULL;
		}

		// look up neighbor cell.  if the screen is connected then
		// we can stop.  otherwise we skip over an unconnected
		// screen.
		CScreenList::const_iterator index = m_screens.find(dstName);
		if (index != m_screens.end()) {
			log((CLOG_DEBUG "\"%s\" is on %s of \"%s\"", dstName.c_str(), CScreenMap::dirName(dir), srcName.c_str()));
			return index->second;
		}

		log((CLOG_DEBUG "ignored \"%s\" on %s of \"%s\"", dstName.c_str(), CScreenMap::dirName(dir), srcName.c_str()));
		srcName = dstName;
	}
}

CServer::CScreenInfo*	CServer::getNeighbor(CScreenInfo* src,
								CScreenMap::EDirection srcSide,
								SInt32& x, SInt32& y) const
{
	assert(src != NULL);

	// get the first neighbor
	CScreenInfo* lastGoodScreen = src;
	CScreenInfo* dst = getNeighbor(src, srcSide);

	// get the source screen's size (needed for kRight and kBottom)
	SInt32 w = src->m_width, h = src->m_height;

	// find destination screen, adjusting x or y (but not both)
	switch (srcSide) {
	  case CScreenMap::kLeft:
		while (dst != NULL) {
			lastGoodScreen = dst;
			w = lastGoodScreen->m_width;
			h = lastGoodScreen->m_height;
			x += w;
			if (x >= 0) {
				break;
			}
			log((CLOG_DEBUG "skipping over screen %s", dst->m_name.c_str()));
			dst = getNeighbor(lastGoodScreen, srcSide);
		}
		break;

	  case CScreenMap::kRight:
		while (dst != NULL) {
			lastGoodScreen = dst;
			x -= w;
			w = lastGoodScreen->m_width;
			h = lastGoodScreen->m_height;
			if (x < w) {
				break;
			}
			log((CLOG_DEBUG "skipping over screen %s", dst->m_name.c_str()));
			dst = getNeighbor(lastGoodScreen, srcSide);
		}
		break;

	  case CScreenMap::kTop:
		while (dst != NULL) {
			lastGoodScreen = dst;
			w = lastGoodScreen->m_width;
			h = lastGoodScreen->m_height;
			y += h;
			if (y >= 0) {
				break;
			}
			log((CLOG_DEBUG "skipping over screen %s", dst->m_name.c_str()));
			dst = getNeighbor(lastGoodScreen, srcSide);
		}
		break;

	  case CScreenMap::kBottom:
		while (dst != NULL) {
			lastGoodScreen = dst;
			y -= h;
			w = lastGoodScreen->m_width;
			h = lastGoodScreen->m_height;
			if (y < h) {
				break;
			}
			log((CLOG_DEBUG "skipping over screen %s", dst->m_name.c_str()));
			dst = getNeighbor(lastGoodScreen, srcSide);
		}
		break;
	}
	assert(lastGoodScreen != NULL);

	// no neighbor if best neighbor is the source itself
	if (lastGoodScreen == src)
		return NULL;

	// if entering primary screen then be sure to move in far enough
	// to avoid the jump zone.  if entering a side that doesn't have
	// a neighbor (i.e. an asymmetrical side) then we don't need to
	// move inwards because that side can't provoke a jump.
	if (lastGoodScreen->m_protocol == NULL) {
		const CString dstName(lastGoodScreen->m_name);
		switch (srcSide) {
		  case CScreenMap::kLeft:
			if (!m_screenMap.getNeighbor(dstName, CScreenMap::kRight).empty() &&
				x > w - 1 - lastGoodScreen->m_zoneSize)
				x = w - 1 - lastGoodScreen->m_zoneSize;
			break;

		  case CScreenMap::kRight:
			if (!m_screenMap.getNeighbor(dstName, CScreenMap::kLeft).empty() &&
				x < lastGoodScreen->m_zoneSize)
				x = lastGoodScreen->m_zoneSize;
			break;

		  case CScreenMap::kTop:
			if (!m_screenMap.getNeighbor(dstName, CScreenMap::kBottom).empty() &&
				y > h - 1 - lastGoodScreen->m_zoneSize)
				y = h - 1 - lastGoodScreen->m_zoneSize;
			break;

		  case CScreenMap::kBottom:
			if (!m_screenMap.getNeighbor(dstName, CScreenMap::kTop).empty() &&
				y < lastGoodScreen->m_zoneSize)
				y = lastGoodScreen->m_zoneSize;
			break;
		}
	}

	return lastGoodScreen;
}

void					CServer::mapPosition(CScreenInfo* src,
								CScreenMap::EDirection srcSide,
								CScreenInfo* dst,
								SInt32& x, SInt32& y) const
{
	assert(src != NULL);
	assert(dst != NULL);
	assert(srcSide >= CScreenMap::kFirstDirection &&
		   srcSide <= CScreenMap::kLastDirection);

	switch (srcSide) {
	  case CScreenMap::kLeft:
	  case CScreenMap::kRight:
		if (y < 0)
			y = 0;
		else if (y >= src->m_height)
			y = dst->m_height - 1;
		else
			y = static_cast<SInt32>(0.5 + y *
								static_cast<double>(dst->m_height - 1) /
													(src->m_height - 1));
		break;

	  case CScreenMap::kTop:
	  case CScreenMap::kBottom:
		if (x < 0)
			x = 0;
		else if (x >= src->m_width)
			x = dst->m_width - 1;
		else
			x = static_cast<SInt32>(0.5 + x *
								static_cast<double>(dst->m_width - 1) /
													(src->m_width - 1));
		break;
	}
}

#include "CTCPListenSocket.h"
void					CServer::acceptClients(void*)
{
	log((CLOG_DEBUG "starting to wait for clients"));

	// add this thread to the list of threads to cancel.  remove from
	// list in d'tor.
	CCleanupNote cleanupNote(this);

	std::auto_ptr<IListenSocket> listen;
	try {
		// create socket listener
//		listen.reset(m_socketFactory->createListen());
		listen.reset(new CTCPListenSocket);		// FIXME

		// bind to the desired port.  keep retrying if we can't bind
		// the address immediately.
		CStopwatch timer;
		CNetworkAddress addr(50001 /* FIXME -- m_port */);
		for (;;) {
			try {
				log((CLOG_DEBUG "binding listen socket"));
				listen->bind(addr);
				break;
			}
			catch (XSocketAddressInUse&) {
				// give up if we've waited too long
				if (timer.getTime() >= m_bindTimeout) {
					log((CLOG_DEBUG "waited too long to bind, giving up"));
					throw;
				}

				// wait a bit before retrying
				log((CLOG_DEBUG "bind failed;  waiting to retry"));
				CThread::sleep(5.0);
			}
		}

		// accept connections and begin processing them
		log((CLOG_DEBUG "waiting for client connections"));
		for (;;) {
			// accept connection
			CThread::testCancel();
			ISocket* socket = listen->accept();
			log((CLOG_NOTE "accepted client connection"));
			CThread::testCancel();

			// start handshake thread
			CThread(new TMethodJob<CServer>(
								this, &CServer::handshakeClient, socket));
		}
	}
	catch (XBase& e) {
		log((CLOG_ERR "cannot listen for clients: %s", e.what()));
		quit();
	}
}

void					CServer::handshakeClient(void* vsocket)
{
	log((CLOG_DEBUG "negotiating with new client"));

	// get the socket pointer from the argument
	assert(vsocket != NULL);
	std::auto_ptr<ISocket> socket(reinterpret_cast<ISocket*>(vsocket));

	// add this thread to the list of threads to cancel.  remove from
	// list in d'tor.
	CCleanupNote cleanupNote(this);

	CString name("<unknown>");
	try {
		// get the input and output streams
		IInputStream*  srcInput  = socket->getInputStream();
		IOutputStream* srcOutput = socket->getOutputStream();
		std::auto_ptr<IInputStream> input;
		std::auto_ptr<IOutputStream> output;

		// attach the encryption layer
		bool own = false;
		if (m_securityFactory != NULL) {
/* FIXME -- implement ISecurityFactory
			input.reset(m_securityFactory->createInputFilter(srcInput, own));
			output.reset(m_securityFactory->createOutputFilter(srcOutput, own));
			srcInput  = input.get();
			srcOutput = output.get();
			own       = true;
*/
		}

		// attach the packetizing filters
		input.reset(new CInputPacketStream(srcInput, own));
		output.reset(new COutputPacketStream(srcOutput, own));

		std::auto_ptr<IServerProtocol> protocol;
		std::auto_ptr<CConnectionNote> connectedNote;
		{
			// give the client a limited time to complete the handshake
			CTimerThread timer(30.0);

			// limit the maximum length of the hello
			static const UInt32 maxHelloLen = 1024;

			// say hello
			log((CLOG_DEBUG "saying hello"));
			CProtocolUtil::writef(output.get(), "Synergy%2i%2i",
										kMajorVersion, kMinorVersion);
			output->flush();

			// wait for the reply
			log((CLOG_DEBUG "waiting for hello reply"));
			UInt32 n = input->getSize();
			if (n > maxHelloLen) {
				throw XBadClient();
			}

			// get and parse the reply to hello
			SInt32 major, minor;
			try {
				log((CLOG_DEBUG "parsing hello reply"));
				CProtocolUtil::readf(input.get(), "Synergy%2i%2i%s",
										&major, &minor, &name);
			}
			catch (XIO&) {
				throw XBadClient();
			}
			if (major < 0 || minor < 0) {
				throw XBadClient();
			}

			// create a protocol interpreter for the version
			log((CLOG_DEBUG "creating interpreter for client %s version %d.%d", name.c_str(), major, minor));
			protocol.reset(CServerProtocol::create(major, minor,
									this, name, input.get(), output.get()));

			// client is now pending
			connectedNote.reset(new CConnectionNote(this,
									name, protocol.get()));

			// ask and wait for the client's info
			log((CLOG_DEBUG "waiting for info for client %s", name.c_str()));
			protocol->queryInfo();
		}

		// handle messages from client.  returns when the client
		// disconnects.
		log((CLOG_NOTE "client %s is connected", name.c_str()));
		protocol->run();
	}
	catch (XIncompatibleClient& e) {
		// client is incompatible
		log((CLOG_WARN "client \"%s\" has incompatible version %d.%d)", name.c_str(), e.getMajor(), e.getMinor()));
		// FIXME -- could print network address if socket had suitable method
	}
	catch (XBadClient&) {
		// client not behaving
		log((CLOG_WARN "protocol error from client \"%s\"", name.c_str()));
		// FIXME -- could print network address if socket had suitable method
	}
	catch (XBase& e) {
		// misc error
		log((CLOG_WARN "error communicating with client \"%s\": %s", name.c_str(), e.what()));
		// FIXME -- could print network address if socket had suitable method
	}
}

void					CServer::quit()
{
	CLock lock(&m_mutex);
	m_done = true;
	m_done.broadcast();
}

// FIXME -- use factory to create screen
#include "CXWindowsPrimaryScreen.h"
void					CServer::openPrimaryScreen()
{
	assert(m_primary == NULL);

	// open screen
	log((CLOG_DEBUG "creating primary screen"));
	m_primary = new CXWindowsPrimaryScreen;
	log((CLOG_DEBUG "opening primary screen"));
	m_primary->open(this);

	// add connection
	m_active = addConnection(CString("primary"/* FIXME */), NULL);
	m_primaryInfo = m_active;

	// update info
	m_primary->getSize(&m_active->m_width, &m_active->m_height);
	m_active->m_zoneSize = m_primary->getJumpZoneSize();
	log((CLOG_NOTE "server size=%dx%d zone=%d", m_active->m_width, m_active->m_height, m_active->m_zoneSize));
	// FIXME -- need way for primary screen to call us back
}

void					CServer::closePrimaryScreen()
{
	assert(m_primary != NULL);

	// remove connection
	removeConnection(CString("primary"/* FIXME */));

	// close the primary screen
	try {
		log((CLOG_DEBUG "closing primary screen"));
		m_primary->close();
	}
	catch (...) {
		// ignore
	}

	// clean up
	log((CLOG_DEBUG "destroying primary screen"));
	delete m_primary;
	m_primary = NULL;
}

void					CServer::addCleanupThread(const CThread& thread)
{
	CLock lock(&m_mutex);
	m_cleanupList.insert(m_cleanupList.begin(), new CThread(thread));
}

void					CServer::removeCleanupThread(const CThread& thread)
{
	CLock lock(&m_mutex);
	for (CThreadList::iterator index = m_cleanupList.begin();
								index != m_cleanupList.end(); ++index) {
		if (**index == thread) {
			m_cleanupList.erase(index);
			delete *index;
			return;
		}
	}
}

void					CServer::cleanupThreads()
{
	log((CLOG_DEBUG "cleaning up threads"));
	m_mutex.lock();
	while (m_cleanupList.begin() != m_cleanupList.end()) {
		// get the next thread and cancel it
		CThread* thread = m_cleanupList.front();
		thread->cancel();

		// wait for thread to finish with cleanup list unlocked.  the
		// thread will remove itself from the cleanup list.
		m_mutex.unlock();
		thread->wait();
		m_mutex.lock();
	}

	// FIXME -- delete remaining threads from list
	m_mutex.unlock();
	log((CLOG_DEBUG "cleaned up threads"));
}

CServer::CScreenInfo*	CServer::addConnection(
								const CString& name, IServerProtocol* protocol)
{
	log((CLOG_DEBUG "adding connection \"%s\"", name.c_str()));
	CLock lock(&m_mutex);
	assert(m_screens.count(name) == 0);
	CScreenInfo* newScreen = new CScreenInfo(name, protocol);
	m_screens.insert(std::make_pair(name, newScreen));
	return newScreen;
}

void					CServer::removeConnection(const CString& name)
{
	log((CLOG_DEBUG "removing connection \"%s\"", name.c_str()));
	CLock lock(&m_mutex);

	// find screen info
	CScreenList::iterator index = m_screens.find(name);
	assert(index != m_screens.end());

	// if this is active screen then we have to jump off of it
	if (m_active == index->second) {
		// record new position (center of primary screen)
		m_x = m_primaryInfo->m_width >> 1;
		m_y = m_primaryInfo->m_height >> 1;

		// don't notify active screen since it probably already disconnected
		log((CLOG_NOTE "jump from \"%s\" to \"%s\" at %d,%d", m_active->m_name.c_str(), m_primaryInfo->m_name.c_str(), m_x, m_y));

		// cut over
		m_active = m_primaryInfo;

		// enter new screen
		m_primary->enter(m_x, m_y);
	}

	// done with screen info
	delete index->second;
	m_screens.erase(index);
}


//
// CServer::CCleanupNote
//

CServer::CCleanupNote::CCleanupNote(CServer* server) : m_server(server)
{
	assert(m_server != NULL);
	m_server->addCleanupThread(CThread::getCurrentThread());
}

CServer::CCleanupNote::~CCleanupNote()
{
	m_server->removeCleanupThread(CThread::getCurrentThread());
}


//
// CServer::CConnectionNote
//

CServer::CConnectionNote::CConnectionNote(CServer* server,
								const CString& name,
								IServerProtocol* protocol) :
								m_server(server),
								m_name(name)
{
	assert(m_server != NULL);
	m_server->addConnection(m_name, protocol);
}

CServer::CConnectionNote::~CConnectionNote()
{
	m_server->removeConnection(m_name);
}


//
// CServer::CScreenInfo
//

CServer::CScreenInfo::CScreenInfo(const CString& name,
								IServerProtocol* protocol) :
								m_name(name),
								m_protocol(protocol),
								m_width(0), m_height(0),
								m_zoneSize(0)
{
	// do nothing
}

CServer::CScreenInfo::~CScreenInfo()
{
	// do nothing
}
