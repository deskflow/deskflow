#include "CServer.h"
#include "CEvent.h"
#include "IEventQueue.h"
#include "IScreen.h"
#include "CScreenProxy.h"
#include "ISocket.h"
#include "CSocketFactory.h"
#include "CMessageSocket.h"
#include "TMethodJob.h"
#include "CTrace.h"
#include <assert.h>
#include <string.h>
#include <ctype.h>

#if !defined(NDEBUG)
static const char*		s_dirName[] = { "left", "right", "top", "bottom" };
#endif

//
// CServerSocketJob
//

class CServerSocketJob : public IJob {
  public:
	typedef void (CServer::*ServerMethod)(ISocket*);

	CServerSocketJob(CServer*, ServerMethod, ISocket*);
	virtual ~CServerSocketJob();

	// IJob overrides
	virtual void		run();

  private:
	CServer*			m_server;
	ServerMethod		m_method;
	ISocket*			m_socket;
};

CServerSocketJob::CServerSocketJob(CServer* server,
								ServerMethod method, ISocket* socket) :
								m_server(server),
								m_method(method),
								m_socket(socket)
{
	// do nothing
}

CServerSocketJob::~CServerSocketJob()
{
	// do nothing
}

void					CServerSocketJob::run()
{
	(m_server->*m_method)(m_socket);
}


//
// CServer
//

class XServerScreenExists {	// FIXME
  public:
	XServerScreenExists(const CString&) { }
};

// the width/height of the zone on the edge of the local screen that
// will provoke a switch to a neighboring screen.  this generally
// shouldn't be changed because it effectively reduces the size of
// the local screen's screen.
// FIXME -- should get this from the local screen itself.  it may
// need a slightly larger zone (to avoid virtual screens) or it may
// be able to generate off-screen coordinates to provoke the switch
// in which case the size can be zero.
const SInt32			CServer::s_zoneSize = 1;

CServer::CServer() : m_running(false), m_done(false),
								m_localScreen(NULL),
								m_activeScreen(NULL),
								m_listenHost(),
								// FIXME -- define kDefaultPort
								m_listenPort(40001/*CProtocol::kDefaultPort*/),
								m_listenSocket(NULL)
{
	// FIXME
}

CServer::~CServer()
{
	assert(m_listenSocket == NULL);

	// FIXME
}

void					CServer::setListenPort(
								const CString& hostname, UInt16 port)
{
	m_listenHost = hostname;
	m_listenPort = port;
}

void					CServer::addLocalScreen(IScreen* screen)
{
	assert(screen != NULL);
	assert(m_running == false);
	assert(m_localScreen == NULL);

	addScreen(screen->getName(), screen);
	m_localScreen  = screen;
	m_activeScreen = screen;

	// open the screen as primary
	screen->open(true);
}

void					CServer::addRemoteScreen(const CString& name)
{
	addScreen(name, NULL);
}

void					CServer::addScreen(const CString& name, IScreen* screen)
{
	assert(!name.empty());

	// cannot add a screen multiple times
	if (m_map.count(name) != 0)
		throw XServerScreenExists(name);

	// add entry for screen in the map
	ScreenCell& cell = m_map[name];

	// set the cell's screen
	cell.m_screen = screen;
}

void					CServer::removeScreen(const CString& name)
{
	// screen must in map
	assert(!name.empty());
	assert(m_map.count(name) == 1);

	// look up cell
	ScreenCell& cell = m_map[name];

	// if this is the local screen then there must not be any other
	// screens and we must not be running.
	assert(cell.m_screen != m_localScreen || (m_map.size() == 1 && !m_running));

	// if this is the active screen then warp to the local screen, or
	// set no active screen if this is the local screen.
	if (cell.m_screen == m_localScreen) {
		m_activeScreen = NULL;
		m_localScreen  = NULL;
	}
	else if (cell.m_screen == m_activeScreen) {
		setActiveScreen(m_localScreen);
	}

	// close the screen
	if (cell.m_screen)
		cell.m_screen->close();

	// fix up map
	if (!cell.m_neighbor[kLeft].empty()) {
		assert(m_map.count(cell.m_neighbor[kLeft]) == 1);
		m_map[cell.m_neighbor[kLeft]].m_neighbor[kRight] =
											cell.m_neighbor[kRight];
	}
	if (!cell.m_neighbor[kRight].empty()) {
		assert(m_map.count(cell.m_neighbor[kRight]) == 1);
		m_map[cell.m_neighbor[kRight]].m_neighbor[kLeft] =
											cell.m_neighbor[kLeft];
	}
	if (!cell.m_neighbor[kTop].empty()) {
		assert(m_map.count(cell.m_neighbor[kTop]) == 1);
		m_map[cell.m_neighbor[kTop]].m_neighbor[kBottom] =
											cell.m_neighbor[kBottom];
	}
	if (!cell.m_neighbor[kBottom].empty()) {
		assert(m_map.count(cell.m_neighbor[kBottom]) == 1);
		m_map[cell.m_neighbor[kBottom]].m_neighbor[kTop] =
											cell.m_neighbor[kTop];
	}
}

void					CServer::connectEdge(
								const CString& src, EDirection srcSide,
								const CString& dst)
{
	// check input
	assert(!src.empty());
	assert(!dst.empty());
	assert(srcSide >= kFirstDirection && srcSide <= kLastDirection);

	// both screens must exist in map
	assert(m_map.count(src) == 1);
	assert(m_map.count(dst) == 1);

	// look up map entry
	ScreenCell& cell = m_map[src];

	// set edge
	cell.m_neighbor[srcSide] = dst;

	TRACE(("connect %s:%s to %s", src.c_str(),
								s_dirName[srcSide],
								cell.m_neighbor[srcSide].c_str()));
}

void					CServer::disconnectEdge(
								const CString& src, EDirection srcSide)
{
	// check input
	assert(!src.empty());
	assert(srcSide >= kFirstDirection && srcSide <= kLastDirection);
	assert(m_map.count(src) == 1);

	TRACE(("disconnect %s:%s from %s", src.c_str(),
								s_dirName[srcSide],
								m_map[src].m_neighbor[srcSide].c_str()));

	// look up map entry
	ScreenCell& cell = m_map[src];

	// set edge
	cell.m_neighbor[srcSide] = CString();
}

void					CServer::run()
{
	assert(m_running == false);
	assert(m_activeScreen != NULL);
	assert(m_activeScreen == m_localScreen);

	// prepare socket to listen for remote screens
	// FIXME -- need m_socketFactory (creates sockets of desired type)
//	m_listenSocket = m_socketFactory->createSocket();
m_listenSocket = CSOCKETFACTORY->create();
	m_listenSocket->setReadJob(new TMethodJob<CServer>(this,
										&CServer::newConnectionCB));
	// FIXME -- keep retrying until this works (in case of FIN_WAIT).
	// also, must clean up m_listenSocket if this method throws anywhere.
	m_listenSocket->listen(m_listenHost, m_listenPort);

	// now running
	m_running = true;

	// event loop
	IEventQueue* queue = CEQ;
	while (!m_done) {
		// wait for new connections, network messages, and user events
		queue->wait(-1.0);

		// handle events
		while (!queue->isEmpty()) {
			// get the next event
			CEvent event;
			queue->pop(&event);

			// handle it
			switch (event.m_any.m_type) {
			  case CEventBase::kNull:
				// do nothing
				break;

			  case CEventBase::kKeyDown:
			  case CEventBase::kKeyRepeat:
			  case CEventBase::kKeyUp:
				if (!onCommandKey(&event.m_key))
					relayEvent(&event);
				break;

			  case CEventBase::kMouseDown:
			  case CEventBase::kMouseUp:
			  case CEventBase::kMouseWheel:
				relayEvent(&event);
				break;

			  case CEventBase::kMouseMove:
				if (m_localScreen == m_activeScreen)
					onLocalMouseMove(event.m_mouse.m_x, event.m_mouse.m_y);
				else
					onRemoteMouseMove(event.m_mouse.m_x, event.m_mouse.m_y);
				break;

			  case CEventBase::kScreenSize:
				// FIXME
				break;
			}
		}
	}

	// reset
	m_running = false;
	m_done    = false;

	// tell screens to shutdown
	// FIXME

	// close our socket
	delete m_listenSocket;
	m_listenSocket = NULL;
}

void					CServer::onClipboardChanged(IScreen*)
{
	// FIXME -- should take screen name not screen pointer
	// FIXME
}

void					CServer::setActiveScreen(IScreen* screen)
{
	// FIXME -- should take screen name not screen pointer
	assert(screen != NULL);
	assert(m_map.count(screen->getName()) == 1);

	// ignore if no change
	if (m_activeScreen == screen)
		return;

	// get center of screen
	SInt32 w, h;
	screen->getSize(&w, &h);
	w >>= 1;
	h >>= 1;

	// switch
	switchScreen(screen, w, h);
}

IScreen*				CServer::getActiveScreen() const
{
	return m_activeScreen;
}

void					CServer::relayEvent(const CEvent* event)
{
	assert(event != NULL);
	assert(m_activeScreen != NULL);

	// ignore attempts to relay to the local screen
	if (m_activeScreen == m_localScreen)
		return;

	// relay the event
	switch (event->m_any.m_type) {
	  case CEventBase::kNull:
		// do nothing
		break;

	  case CEventBase::kKeyDown:
		m_activeScreen->onKeyDown(event->m_key.m_key);
		break;

	  case CEventBase::kKeyRepeat:
		m_activeScreen->onKeyRepeat(event->m_key.m_key, event->m_key.m_count);
		break;

	  case CEventBase::kKeyUp:
		m_activeScreen->onKeyUp(event->m_key.m_key);
		break;

	  case CEventBase::kMouseDown:
		m_activeScreen->onMouseDown(event->m_mouse.m_button);
		break;

	  case CEventBase::kMouseUp:
		m_activeScreen->onMouseUp(event->m_mouse.m_button);
		break;

	  case CEventBase::kMouseWheel:
		m_activeScreen->onMouseWheel(event->m_mouse.m_x);
		break;

	  case CEventBase::kMouseMove:
		assert(0 && "kMouseMove relayed");
		break;

	  default:
		assert(0 && "invalid event relayed");
		break;
	}
}

bool					CServer::onCommandKey(const CEventKey* /*keyEvent*/)
{
	// FIXME -- strip out command keys (e.g. lock to screen, warp, etc.)
	return false;
}

void					CServer::onLocalMouseMove(SInt32 x, SInt32 y)
{
	assert(m_activeScreen == m_localScreen);

	// ignore if locked to screen
	if (isLockedToScreen())
		return;

	// get local screen's size
	SInt32 w, h;
	m_activeScreen->getSize(&w, &h);

	// see if we should change screens
	EDirection dir;
	if (x < s_zoneSize) {
		x -= s_zoneSize;
		dir = kLeft;
	}
	else if (x >= w - s_zoneSize) {
		x += s_zoneSize;
		dir = kRight;
	}
	else if (y < s_zoneSize) {
		y -= s_zoneSize;
		dir = kTop;
	}
	else if (y >= h - s_zoneSize) {
		y += s_zoneSize;
		dir = kBottom;
	}
	else {
		// still on local screen
		return;
	}
	TRACE(("leave %s on %s", m_activeScreen->getName().c_str(), s_dirName[dir]));

	// get new screen.  if no screen in that direction then ignore move.
	IScreen* newScreen = getNeighbor(m_activeScreen, dir, x, y);
	if (newScreen == NULL)
		return;

	// remap position to account for resolution differences between screens
	mapPosition(m_activeScreen, dir, newScreen, x, y);

	// switch screen
	switchScreen(newScreen, x, y);
}

void					CServer::onRemoteMouseMove(SInt32 dx, SInt32 dy)
{
	assert(m_activeScreen != NULL);
	assert(m_activeScreen != m_localScreen);

	// put mouse back in center of local screen's grab area
// XXX	m_localScreen->warpToCenter();

	// save old position
	const SInt32 xOld = m_x;
	const SInt32 yOld = m_y;

	// accumulate mouse position
	m_x += dx;
	m_y += dy;

	// get active screen's size
	SInt32 w, h;
	m_activeScreen->getSize(&w, &h);

	// switch screens if mouse is outside screen and not locked to screen
	IScreen* newScreen = NULL;
	if (!isLockedToScreen()) {
		// find direction of neighbor
		EDirection dir;
		if (m_x < 0)
			dir = kLeft;
		else if (m_x > w - 1)
			dir = kRight;
		else if (m_y < 0)
			dir = kTop;
		else if (m_y > h - 1)
			dir = kBottom;
		else
			newScreen = m_activeScreen;

		// get neighbor if we should switch
		if (newScreen == NULL) {
			TRACE(("leave %s on %s", m_activeScreen->getName().c_str(),
								s_dirName[dir]));

			SInt32 x = m_x, y = m_y;
			newScreen = getNeighbor(m_activeScreen, dir, x, y);

			// remap position to account for resolution differences
			if (newScreen != NULL) {
				m_x = x;
				m_y = y;
				mapPosition(m_activeScreen, dir, newScreen, m_x, m_y);
			}
			else {
				if (m_x < 0)
					m_x = 0;
				else if (m_x > w - 1)
					m_x = w - 1;
				if (m_y < 0)
					m_y = 0;
				else if (m_y > h - 1)
					m_y = h - 1;
			}
		}
	}

	// clamp mouse position if locked to screen
	else {
		TRACE(("clamp to %s", m_activeScreen->getName().c_str()));

		if (m_x < 0)
			m_x = 0;
		else if (m_x > w - 1)
			m_x = w - 1;
		if (m_y < 0)
			m_y = 0;
		else if (m_y > h - 1)
			m_y = h - 1;
	}

	// if on same screen then warp cursor
	if (newScreen == NULL || newScreen == m_activeScreen) {
		// ignore if clamped mouse didn't move
		if (m_x != xOld || m_y != yOld) {
			TRACE(("move on %s to %d,%d",
								m_activeScreen->getName().c_str(), m_x, m_y));
			m_activeScreen->onMouseMove(m_x, m_y);
		}
	}

	// otherwise switch the screen
	else {
		switchScreen(newScreen, m_x, m_y);
	}
}

bool					CServer::isLockedToScreen() const
{
	// FIXME
	return false;
}

void					CServer::mapPosition(
								const IScreen* src, EDirection srcSide,
								const IScreen* dst, SInt32& x, SInt32& y) const
{
	assert(src != NULL);
	assert(dst != NULL);
	assert(srcSide >= kFirstDirection && srcSide <= kLastDirection);

	// get sizes
	SInt32 wSrc, hSrc, wDst, hDst;
	src->getSize(&wSrc, &hSrc);
	dst->getSize(&wDst, &hDst);

	// remap
	switch (srcSide) {
	  case kLeft:
	  case kRight:
		assert(y >= 0 && y < hSrc);
		y = static_cast<SInt32>(0.5 + y *
								static_cast<double>(hDst - 1) / (hSrc - 1));
		break;

	  case kTop:
	  case kBottom:
		assert(x >= 0 && x < wSrc);
		x = static_cast<SInt32>(0.5 + x *
								static_cast<double>(wSrc - 1) / (wSrc - 1));
		break;
	}
}

IScreen*				CServer::getNeighbor(
								const IScreen* src, EDirection dir) const
{
	// check input
	assert(src != NULL);
	assert(dir >= kFirstDirection && dir <= kLastDirection);
	assert(m_map.count(src->getName()) == 1);

	// look up source cell
	ScreenMap::const_iterator index = m_map.find(src->getName());
	do {
		// look up name of neighbor
		const ScreenCell& cell = index->second;
		const CString dstName(cell.m_neighbor[dir]);

		// if nothing in that direction then return NULL
		if (dstName.empty())
			return NULL;

		// look up neighbor cell
		assert(m_map.count(dstName) == 1);
		index = m_map.find(dstName);

		// if no screen pointer then can't go to that neighbor so keep
		// searching in the same direction.
#ifndef NDEBUG
		if (index->second.m_screen == NULL)
			TRACE(("skipping over unconnected screen %s", dstName.c_str()));
#endif
	} while (index->second.m_screen == NULL);

	return index->second.m_screen;
}

IScreen*				CServer::getNeighbor(
								const IScreen* src, EDirection srcSide,
								SInt32& x, SInt32& y) const
{
	// given a position relative to src and which side of the screen we
	// left, find the screen we should move onto and where.  if the
	// position is sufficiently far from src then we may cross multiple
	// screens.

	// check input
	assert(src != NULL);
	assert(srcSide >= kFirstDirection && srcSide <= kLastDirection);

	// get the first neighbor
	IScreen* dst = getNeighbor(src, srcSide);
	IScreen* lastGoodScreen = dst;

	// get the original screen's size (needed for kRight and kBottom)
	SInt32 w, h;
	src->getSize(&w, &h);

	// find destination screen, adjusting x or y (but not both)
	switch (srcSide) {
	  case kLeft:
		while (dst) {
			lastGoodScreen = dst;
			lastGoodScreen->getSize(&w, &h);
			x += w;
			if (x >= 0)
				break;
			TRACE(("skipping over screen %s", dst->getName().c_str()));
			dst = getNeighbor(lastGoodScreen, srcSide);
		}
		break;

	  case kRight:
		while (dst) {
			lastGoodScreen = dst;
			x -= w;
			lastGoodScreen->getSize(&w, &h);
			if (x < w)
				break;
			TRACE(("skipping over screen %s", dst->getName().c_str()));
			dst = getNeighbor(lastGoodScreen, srcSide);
		}
		break;

	  case kTop:
		while (dst) {
			lastGoodScreen = dst;
			lastGoodScreen->getSize(&w, &h);
			y += h;
			if (y >= 0)
				break;
			TRACE(("skipping over screen %s", dst->getName().c_str()));
			dst = getNeighbor(lastGoodScreen, srcSide);
		}
		break;

	  case kBottom:
		while (dst) {
			lastGoodScreen = dst;
			y -= h;
			lastGoodScreen->getSize(&w, &h);
			if (y < h)
				break;
			TRACE(("skipping over screen %s", dst->getName().c_str()));
			dst = getNeighbor(lastGoodScreen, srcSide);
		}
		break;
	}

	// if entering local screen then be sure to move in far enough to
	// avoid the switching zone.  if entering a side that doesn't have
	// a neighbor (i.e. an asymmetrical side) then we don't need to
	// move inwards because that side can't provoke a switch.
	if (lastGoodScreen == m_localScreen) {
		ScreenMap::const_iterator index = m_map.find(m_localScreen->getName());
		const ScreenCell& cell = index->second;
		switch (srcSide) {
		  case kLeft:
			if (!cell.m_neighbor[kRight].empty() && x > w - 1 - s_zoneSize)
				x = w - 1 - s_zoneSize;
			break;

		  case kRight:
			if (!cell.m_neighbor[kLeft].empty() && x < s_zoneSize)
				x = s_zoneSize;
			break;

		  case kTop:
			if (!cell.m_neighbor[kBottom].empty() && y > h - 1 - s_zoneSize)
				y = h - 1 - s_zoneSize;
			break;

		  case kBottom:
			if (!cell.m_neighbor[kTop].empty() && y < s_zoneSize)
				y = s_zoneSize;
			break;
		}
	}

	return lastGoodScreen;
}

void					CServer::switchScreen(
								IScreen* screen, SInt32 x, SInt32 y)
{
	assert(screen != NULL);
	assert(m_running == true);
	assert(m_activeScreen != NULL);
#ifndef NDEBUG
	{
		SInt32 w, h;
		screen->getSize(&w, &h);
		assert(x >= 0 && y >= 0 && x < w && y < h);
	}
#endif

	TRACE(("switch %s to %s at %d,%d", m_activeScreen->getName().c_str(),
								screen->getName().c_str(), x, y));

	// wrapping means leaving the active screen and entering it again.
	// since that's a waste of time we skip that and just warp the
	// mouse.
	if (m_activeScreen != screen) {
		// leave active screen
		m_activeScreen->leaveScreen();

		// cut over
		m_activeScreen = screen;

		// enter new screen
		m_activeScreen->enterScreen(x, y);
	}
	else {
		m_activeScreen->warpCursor(x, y);
	}

	// record new position
	m_x = x;
	m_y = y;
}

void					CServer::newConnectionCB()
{
	ISocket* socket = m_listenSocket->accept();
	TRACE(("accepted socket %p", socket));
	socket->setReadJob(new CServerSocketJob(this, &CServer::loginCB, socket));
	m_logins.insert(socket);
}

void					CServer::loginCB(ISocket* socket)
{
	// FIXME -- no fixed size buffers
	UInt8 buffer[512];
	SInt32 n = socket->read(buffer, sizeof(buffer));
	if (n == -1) {
		TRACE(("socket %p disconnected", socket));
		goto fail;
	}
	TRACE(("read %d bytes from socket %p", n, socket));
	if (n <= 10) {
		TRACE(("socket %p: bogus %d byte message; hanging up", socket, n));
		goto fail;
	}
	if (n > 10) {
		if (::memcmp(buffer, "SYNERGY\000\001", 9) != 0) {
			TRACE(("socket %p: bad login", socket));
			goto fail;
		}

		const SInt32 nameLen = static_cast<SInt32>(buffer[9]);
		if (nameLen < 1 || nameLen > 64) {
			TRACE(("socket %p: bad login name length %d", socket, nameLen));
			goto fail;
		}

		for (SInt32 i = 0; i < nameLen; ++i)
			if (!isalnum(buffer[10 + i])) {
				TRACE(("socket %p: bad login name", socket));
				goto fail;
			}

		CString name(reinterpret_cast<char*>(buffer + 10), nameLen);
		const ScreenMap::iterator index = m_map.find(name);
		if (index == m_map.end()) {
			TRACE(("socket %p: unknown screen %s", socket, name.c_str()));
			goto fail;
		}
		if (index->second.m_screen != NULL) {
			TRACE(("socket %p: screen %s already connected",
								socket, name.c_str()));
			goto fail;
		}

		TRACE(("socket %p: login %s", socket, name.c_str()));
		CScreenProxy* screen = new CScreenProxy(name, socket);
		m_logins.erase(socket);
		index->second.m_screen = screen;
		index->second.m_screen->open(false);
	}
	return;

fail:
	m_logins.erase(socket);
	delete socket;
}
