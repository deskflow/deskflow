#include "CServer.h"
#include "CHTTPServer.h"
#include "CInputPacketStream.h"
#include "COutputPacketStream.h"
#include "CServerProtocol.h"
#include "CProtocolUtil.h"
#include "IPrimaryScreen.h"
#include "ISocketFactory.h"
#include "ProtocolTypes.h"
#include "CNetworkAddress.h"
#include "ISocket.h"
#include "IListenSocket.h"
#include "CLock.h"
#include "CLog.h"
#include "CThread.h"
#include "CTimerThread.h"
#include "CStopwatch.h"
#include "CFunctionJob.h"
#include "TMethodJob.h"
#include "XScreen.h"
#include "XSocket.h"
#include "XSynergy.h"
#include "XThread.h"
#include <assert.h>
#include <memory>

// hack to work around operator=() bug in STL in g++ prior to v3
#if defined(__GNUC__) && (__GNUC__ < 3)
#define assign(_dst, _src, _type)	_dst.reset(_src)
#else
#define assign(_dst, _src, _type)	_dst = std::auto_ptr<_type >(_src)
#endif


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

const SInt32			CServer::s_httpMaxSimultaneousRequests = 3;

CServer::CServer() : m_cleanupSize(&m_mutex, 0),
								m_primary(NULL),
								m_active(NULL),
								m_primaryInfo(NULL),
								m_seqNum(0),
								m_httpServer(NULL),
								m_httpAvailable(&m_mutex,
										s_httpMaxSimultaneousRequests)
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
		while (m_primary == NULL) {
			try {
				openPrimaryScreen();
			}
			catch (XScreenOpenFailure&) {
				// can't open screen yet.  wait a few seconds to retry.
				log((CLOG_INFO "failed to open screen.  waiting to retry."));
				CThread::sleep(3.0);
			}
		}

		// start listening for HTTP requests
		m_httpServer = new CHTTPServer(this);
		CThread(new TMethodJob<CServer>(this, &CServer::acceptHTTPClients));

		// start listening for new clients
		CThread(new TMethodJob<CServer>(this, &CServer::acceptClients));

		// handle events
		log((CLOG_DEBUG "starting event handling"));
		m_primary->run();

		// clean up
		log((CLOG_NOTE "stopping server"));
		cleanupThreads();
		delete m_httpServer;
		m_httpServer = NULL;
		closePrimaryScreen();
	}
	catch (XBase& e) {
		log((CLOG_ERR "server error: %s", e.what()));

		// clean up
		log((CLOG_NOTE "stopping server"));
		cleanupThreads();
		delete m_httpServer;
		m_httpServer = NULL;
		closePrimaryScreen();
	}
	catch (XThread&) {
		// clean up
		log((CLOG_NOTE "stopping server"));
		cleanupThreads();
		delete m_httpServer;
		m_httpServer = NULL;
		if (m_primary != NULL) {
			closePrimaryScreen();
		}
		throw;
	}
	catch (...) {
		log((CLOG_DEBUG "unknown server error"));

		// clean up
		log((CLOG_NOTE "stopping server"));
		cleanupThreads();
		delete m_httpServer;
		m_httpServer = NULL;
		if (m_primary != NULL) {
			closePrimaryScreen();
		}
		throw;
	}
}

void					CServer::quit()
{
	m_primary->stop();
}

void					CServer::shutdown()
{
	// stop all running threads but don't wait too long since some
	// threads may be unable to proceed until this thread returns.
	cleanupThreads(3.0);

	// done with the HTTP server
	delete m_httpServer;
	m_httpServer = NULL;

	// note -- we do not attempt to close down the primary screen
}

bool					CServer::setConfig(const CConfig& config)
{
	typedef std::vector<CThread> CThreads;
	CThreads threads;
	{
		CLock lock(&m_mutex);

		// refuse configuration if it doesn't include the primary screen
		if (m_primaryInfo != NULL &&
			!config.isScreen(m_primaryInfo->m_name)) {
			return false;
		}

		// get the set of screens that are connected but are being
		// dropped from the configuration.  don't add the primary
		// screen.  also tell the secondary screen to disconnect.
		for (CScreenList::const_iterator index = m_screens.begin();
								index != m_screens.end(); ++index) {
			if (!config.isScreen(index->first) &&
				index->second != m_primaryInfo) {
				assert(index->second->m_protocol != NULL);
				index->second->m_protocol->sendClose();
				threads.push_back(index->second->m_thread);
			}
		}

		// wait a moment to allow each secondary screen to close
		// its connection before we close it (to avoid having our
		// socket enter TIME_WAIT).
		if (threads.size() > 0) {
			CThread::sleep(1.0);
		}

		// cancel the old secondary screen threads
		for (CThreads::iterator index = threads.begin();
								index != threads.end(); ++index) {
			index->cancel();
		}

		// cut over
		m_config = config;

		// tell primary screen about reconfiguration
		if (m_primary != NULL) {
			m_primary->onConfigure();
		}
	}

	// wait for old secondary screen threads to disconnect.  must
	// not hold lock while we do this so those threads can finish
	// any calls to this object.
	for (CThreads::iterator index = threads.begin();
								index != threads.end(); ++index) {
		index->wait();
	}

	return true;
}

CString					CServer::getPrimaryScreenName() const
{
	CLock lock(&m_mutex);
	return (m_primaryInfo == NULL) ? "" : m_primaryInfo->m_name;
}

void					CServer::getConfig(CConfig* config) const
{
	assert(config != NULL);

	CLock lock(&m_mutex);
	*config = m_config;
}

UInt32					CServer::getActivePrimarySides() const
{
	UInt32 sides = 0;
	CLock lock(&m_mutex);
	if (!m_config.getNeighbor(getPrimaryScreenName(),
								CConfig::kLeft).empty()) {
		sides |= CConfig::kLeftMask;
	}
	if (!m_config.getNeighbor(getPrimaryScreenName(),
								CConfig::kRight).empty()) {
		sides |= CConfig::kRightMask;
	}
	if (!m_config.getNeighbor(getPrimaryScreenName(),
								CConfig::kTop).empty()) {
		sides |= CConfig::kTopMask;
	}
	if (!m_config.getNeighbor(getPrimaryScreenName(),
								CConfig::kBottom).empty()) {
		sides |= CConfig::kBottomMask;
	}
	return sides;
}

void					CServer::setInfo(
								SInt32 w, SInt32 h, SInt32 zoneSize,
								SInt32 x, SInt32 y)
{
	CLock lock(&m_mutex);
	assert(m_primaryInfo != NULL);
	setInfoNoLock(m_primaryInfo->m_name, w, h, zoneSize, x, y);
}

void					CServer::setInfo(const CString& client,
								SInt32 w, SInt32 h, SInt32 zoneSize,
								SInt32 x, SInt32 y)
{
	CLock lock(&m_mutex);
	setInfoNoLock(client, w, h, zoneSize, x, y);
}

void					CServer::setInfoNoLock(const CString& screen,
								SInt32 w, SInt32 h, SInt32 zoneSize,
								SInt32 x, SInt32 y)
{
	assert(!screen.empty());
	assert(w > 0);
	assert(h > 0);
	assert(zoneSize >= 0);

	// screen must be connected
	CScreenList::iterator index = m_screens.find(screen);
	if (index == m_screens.end()) {
		throw XBadClient();
	}

	// screen is now ready (i.e. available to user)
	CScreenInfo* info = index->second;
	info->m_ready = true;

	// update screen info
	if (info == m_active) {
		// update the remote mouse coordinates
		m_x = x;
		m_y = y;
	}
	info->m_width    = w;
	info->m_height   = h;
	info->m_zoneSize = zoneSize;
	log((CLOG_NOTE "screen \"%s\" size=%dx%d zone=%d pos=%d,%d", screen.c_str(), w, h, zoneSize, x, y));

	// send acknowledgement (if screen isn't the primary)
	if (info->m_protocol != NULL) {
		info->m_protocol->sendInfoAcknowledgment();
	}

	// handle resolution change to primary screen
	else {
		if (info == m_active) {
			onMouseMovePrimaryNoLock(x, y);
		}
		else {
			onMouseMoveSecondaryNoLock(0, 0);
		}
	}
}

void					CServer::grabClipboard(ClipboardID id)
{
	CLock lock(&m_mutex);
	assert(m_primaryInfo != NULL);
	grabClipboardNoLock(id, 0, m_primaryInfo->m_name);
}

void					CServer::grabClipboard(
								ClipboardID id, UInt32 seqNum,
								const CString& client)
{
	CLock lock(&m_mutex);
	grabClipboardNoLock(id, seqNum, client);
}

void					CServer::grabClipboardNoLock(
								ClipboardID id, UInt32 seqNum,
								const CString& screen)
{
	// note -- must be locked on entry
	CClipboardInfo& clipboard = m_clipboards[id];

	// screen must be connected
	CScreenList::iterator index = m_screens.find(screen);
	if (index == m_screens.end()) {
		throw XBadClient();
	}

	// ignore grab if sequence number is old.  always allow primary
	// screen to grab.
	if (screen != m_primaryInfo->m_name &&
		seqNum < clipboard.m_clipboardSeqNum) {
		log((CLOG_INFO "ignored screen \"%s\" grab of clipboard %d", screen.c_str(), id));
		return;
	}

	// mark screen as owning clipboard
	log((CLOG_NOTE "screen \"%s\" grabbed clipboard %d from \"%s\"", screen.c_str(), id, clipboard.m_clipboardOwner.c_str()));
	clipboard.m_clipboardOwner  = screen;
	clipboard.m_clipboardSeqNum = seqNum;

	// no screens have the new clipboard except the sender
	clearGotClipboard(id);
	index->second->m_gotClipboard[id] = true;

	// tell all other screens to take ownership of clipboard
	for (index = m_screens.begin(); index != m_screens.end(); ++index) {
		if (index->first != screen) {
			CScreenInfo* info = index->second;
			if (info->m_protocol == NULL) {
				m_primary->grabClipboard(id);
			}
			else {
				info->m_protocol->sendGrabClipboard(id);
			}
		}
	}

	// get the clipboard data if primary has it, otherwise mark the
	// clipboard data as unknown.
	if (m_active->m_protocol == NULL) {
		// get clipboard immediately from primary screen
		m_primary->getClipboard(id, &clipboard.m_clipboard);
		clipboard.m_clipboardData  = clipboard.m_clipboard.marshall();
		clipboard.m_clipboardReady = true;
	}
	else {
		// clear out the clipboard since existing data is now out of date.
		if (clipboard.m_clipboard.open(0)) {
			clipboard.m_clipboard.close();
		}
		clipboard.m_clipboardReady = false;
	}
}

void					CServer::setClipboard(ClipboardID id,
								UInt32 seqNum, const CString& data)
{
	CLock lock(&m_mutex);
	CClipboardInfo& clipboard = m_clipboards[id];

	// ignore update if sequence number is old
	if (seqNum < clipboard.m_clipboardSeqNum) {
		log((CLOG_INFO "ignored screen \"%s\" update of clipboard %d", clipboard.m_clipboardOwner.c_str(), id));
		return;
	}

	// unmarshall into our clipboard buffer
	log((CLOG_NOTE "screen \"%s\" updated clipboard %d", clipboard.m_clipboardOwner.c_str(), id));
	clipboard.m_clipboardReady = true;
	clipboard.m_clipboardData  = data;
	clipboard.m_clipboard.unmarshall(clipboard.m_clipboardData, 0);

	// all screens have an out-of-date clipboard except the sender
	clearGotClipboard(id);
	CScreenList::const_iterator index =
								m_screens.find(clipboard.m_clipboardOwner);
	if (index != m_screens.end()) {
		index->second->m_gotClipboard[id] = true;
	}

	// send the new clipboard to the active screen (will do nothing if
	// the active screen is the one sending the new clipboard)
	sendClipboard(id);
}

bool					CServer::onCommandKey(KeyID /*id*/,
								KeyModifierMask /*mask*/, bool /*down*/)
{
	return false;
}

void					CServer::onKeyDown(KeyID id, KeyModifierMask mask)
{
	log((CLOG_DEBUG1 "onKeyDown id=%d mask=0x%04x", id, mask));
	CLock lock(&m_mutex);
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
	log((CLOG_DEBUG1 "onKeyUp id=%d mask=0x%04x", id, mask));
	CLock lock(&m_mutex);
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

void					CServer::onKeyRepeat(
								KeyID id, KeyModifierMask mask, SInt32 count)
{
	log((CLOG_DEBUG1 "onKeyRepeat id=%d mask=0x%04x count=%d", id, mask, count));
	CLock lock(&m_mutex);
	assert(m_active != NULL);

	// handle command keys
	if (onCommandKey(id, mask, false)) {
		onCommandKey(id, mask, true);
		return;
	}

	// relay
	if (m_active->m_protocol != NULL) {
		m_active->m_protocol->sendKeyRepeat(id, mask, count);
	}
}

void					CServer::onMouseDown(ButtonID id)
{
	log((CLOG_DEBUG1 "onMouseDown id=%d", id));
	CLock lock(&m_mutex);
	assert(m_active != NULL);

	// relay
	if (m_active->m_protocol != NULL) {
		m_active->m_protocol->sendMouseDown(id);
	}
}

void					CServer::onMouseUp(ButtonID id)
{
	log((CLOG_DEBUG1 "onMouseUp id=%d", id));
	CLock lock(&m_mutex);
	assert(m_active != NULL);

	// relay
	if (m_active->m_protocol != NULL) {
		m_active->m_protocol->sendMouseUp(id);
	}
}

bool					CServer::onMouseMovePrimary(SInt32 x, SInt32 y)
{
	log((CLOG_DEBUG2 "onMouseMovePrimary %d,%d", x, y));
	CLock lock(&m_mutex);
	return onMouseMovePrimaryNoLock(x, y);
}

bool					CServer::onMouseMovePrimaryNoLock(SInt32 x, SInt32 y)
{
	// mouse move on primary (server's) screen
	assert(m_active != NULL);
	assert(m_active->m_protocol == NULL);

	// ignore if mouse is locked to screen
	if (isLockedToScreenNoLock()) {
		return false;
	}

	// see if we should change screens
	CConfig::EDirection dir;
	if (x < m_active->m_zoneSize) {
		x  -= m_active->m_zoneSize;
		dir = CConfig::kLeft;
		log((CLOG_DEBUG1 "switch to left"));
	}
	else if (x >= m_active->m_width - m_active->m_zoneSize) {
		x  += m_active->m_zoneSize;
		dir = CConfig::kRight;
		log((CLOG_DEBUG1 "switch to right"));
	}
	else if (y < m_active->m_zoneSize) {
		y  -= m_active->m_zoneSize;
		dir = CConfig::kTop;
		log((CLOG_DEBUG1 "switch to top"));
	}
	else if (y >= m_active->m_height - m_active->m_zoneSize) {
		y  += m_active->m_zoneSize;
		dir = CConfig::kBottom;
		log((CLOG_DEBUG1 "switch to bottom"));
	}
	else {
		// still on local screen
		return false;
	}

	// get jump destination
	CScreenInfo* newScreen = getNeighbor(m_active, dir, x, y);

	// if no screen in jump direction then ignore the move
	if (newScreen == NULL) {
		return false;
	}

	// remap position to account for resolution differences
	mapPosition(m_active, dir, newScreen, x, y);

	// switch screen
	switchScreen(newScreen, x, y);
	return true;
}

void					CServer::onMouseMoveSecondary(SInt32 dx, SInt32 dy)
{
	log((CLOG_DEBUG2 "onMouseMoveSecondary %+d,%+d", dx, dy));
	CLock lock(&m_mutex);
	onMouseMoveSecondaryNoLock(dx, dy);
}

void					CServer::onMouseMoveSecondaryNoLock(
								SInt32 dx, SInt32 dy)
{
	// mouse move on secondary (client's) screen
	assert(m_active != NULL);
	if (m_active->m_protocol == NULL) {
		// we're actually on the primary screen.  this can happen
		// when the primary screen begins processing a mouse move
		// for a secondary screen, then the active (secondary)
		// screen disconnects causing us to jump to the primary
		// screen, and finally the primary screen finishes
		// processing the mouse move, still thinking it's for
		// a secondary screen.  we just ignore the motion.
		return;
	}

	// save old position
	const SInt32 xOld = m_x;
	const SInt32 yOld = m_y;

	// accumulate motion
	m_x += dx;
	m_y += dy;

	// switch screens if the mouse is outside the screen and not
	// locked to the screen
	CScreenInfo* newScreen = NULL;
	if (!isLockedToScreenNoLock()) {
		// find direction of neighbor
		CConfig::EDirection dir;
		if (m_x < 0)
			dir = CConfig::kLeft;
		else if (m_x > m_active->m_width - 1)
			dir = CConfig::kRight;
		else if (m_y < 0)
			dir = CConfig::kTop;
		else if (m_y > m_active->m_height - 1)
			dir = CConfig::kBottom;
		else
			newScreen = m_active;

		// get neighbor if we should switch
		if (newScreen == NULL) {
			log((CLOG_DEBUG1 "leave \"%s\" on %s", m_active->m_name.c_str(), CConfig::dirName(dir)));

			SInt32 x = m_x, y = m_y;
			newScreen = getNeighbor(m_active, dir, x, y);

			// remap position to account for resolution differences
			if (newScreen != NULL) {
				mapPosition(m_active, dir, newScreen, x, y);
				m_x = x;
				m_y = y;
			}
			else {
				log((CLOG_DEBUG1 "no neighbor; clamping"));
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
		log((CLOG_DEBUG1 "clamp to \"%s\"", m_active->m_name.c_str()));
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
			log((CLOG_DEBUG2 "move on %s to %d,%d", m_active->m_name.c_str(), m_x, m_y));
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
	log((CLOG_DEBUG1 "onMouseWheel %+d", delta));
	CLock lock(&m_mutex);
	assert(m_active != NULL);

	// relay
	if (m_active->m_protocol != NULL) {
		m_active->m_protocol->sendMouseWheel(delta);
	}
}

bool					CServer::isLockedToScreen() const
{
	CLock lock(&m_mutex);
	return isLockedToScreenNoLock();
}

bool					CServer::isLockedToScreenNoLock() const
{
	// locked if primary says we're locked
	if (m_primary->isLockedToScreen()) {
		return true;
	}

	// locked if scroll-lock is toggled on
	if ((m_primary->getToggleMask() & KeyModifierScrollLock) != 0) {
		return true;
	}

	// not locked
	return false;
}

void					CServer::switchScreen(CScreenInfo* dst,
								SInt32 x, SInt32 y)
{
	assert(dst != NULL);
	assert(x >= 0 && y >= 0 && x < dst->m_width && y < dst->m_height);
	assert(m_active != NULL);

	log((CLOG_NOTE "switch from \"%s\" to \"%s\" at %d,%d", m_active->m_name.c_str(), dst->m_name.c_str(), x, y));
	// FIXME -- we're not locked here but we probably should be

	// wrapping means leaving the active screen and entering it again.
	// since that's a waste of time we skip that and just warp the
	// mouse.
	if (m_active != dst) {
		// note if we're leaving the primary screen
		const bool leavingPrimary = (m_active->m_protocol == NULL);

		// if leaving the primary screen then update the clipboards
		// that it owns
		if (leavingPrimary) {
			for (ClipboardID id = 0; id < kClipboardEnd; ++id) {
				updatePrimaryClipboard(id);
			}
		}

		// leave active screen
		if (leavingPrimary) {
			m_primary->leave();
		}
		else {
			m_active->m_protocol->sendLeave();
		}

		// cut over
		m_active = dst;

		// increment enter sequence number
		++m_seqNum;

		// enter new screen
		if (m_active->m_protocol == NULL) {
			m_primary->enter(x, y);
		}
		else {
			m_active->m_protocol->sendEnter(x, y, m_seqNum,
								m_primary->getToggleMask());
		}

		// send the clipboard data to new active screen
		for (ClipboardID id = 0; id < kClipboardEnd; ++id) {
			sendClipboard(id);
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
								CConfig::EDirection dir) const
{
	assert(src != NULL);

	CString srcName = src->m_name;
	assert(!srcName.empty());
	log((CLOG_DEBUG2 "find neighbor on %s of \"%s\"", CConfig::dirName(dir), srcName.c_str()));
	for (;;) {
		// look up name of neighbor
		const CString dstName(m_config.getNeighbor(srcName, dir));

		// if nothing in that direction then return NULL
		if (dstName.empty()) {
			log((CLOG_DEBUG2 "no neighbor on %s of \"%s\"", CConfig::dirName(dir), srcName.c_str()));
			return NULL;
		}

		// look up neighbor cell.  if the screen is connected and
		// ready then we can stop.  otherwise we skip over an
		// unconnected screen.
		CScreenList::const_iterator index = m_screens.find(dstName);
		if (index != m_screens.end() && index->second->m_ready) {
			log((CLOG_DEBUG2 "\"%s\" is on %s of \"%s\"", dstName.c_str(), CConfig::dirName(dir), srcName.c_str()));
			return index->second;
		}

		log((CLOG_DEBUG2 "ignored \"%s\" on %s of \"%s\"", dstName.c_str(), CConfig::dirName(dir), srcName.c_str()));
		srcName = dstName;
	}
}

CServer::CScreenInfo*	CServer::getNeighbor(CScreenInfo* src,
								CConfig::EDirection srcSide,
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
	case CConfig::kLeft:
		while (dst != NULL) {
			lastGoodScreen = dst;
			w = lastGoodScreen->m_width;
			h = lastGoodScreen->m_height;
			x += w;
			if (x >= 0) {
				break;
			}
			log((CLOG_DEBUG2 "skipping over screen %s", dst->m_name.c_str()));
			dst = getNeighbor(lastGoodScreen, srcSide);
		}
		break;

	case CConfig::kRight:
		while (dst != NULL) {
			lastGoodScreen = dst;
			x -= w;
			w = lastGoodScreen->m_width;
			h = lastGoodScreen->m_height;
			if (x < w) {
				break;
			}
			log((CLOG_DEBUG2 "skipping over screen %s", dst->m_name.c_str()));
			dst = getNeighbor(lastGoodScreen, srcSide);
		}
		break;

	case CConfig::kTop:
		while (dst != NULL) {
			lastGoodScreen = dst;
			w = lastGoodScreen->m_width;
			h = lastGoodScreen->m_height;
			y += h;
			if (y >= 0) {
				break;
			}
			log((CLOG_DEBUG2 "skipping over screen %s", dst->m_name.c_str()));
			dst = getNeighbor(lastGoodScreen, srcSide);
		}
		break;

	case CConfig::kBottom:
		while (dst != NULL) {
			lastGoodScreen = dst;
			y -= h;
			w = lastGoodScreen->m_width;
			h = lastGoodScreen->m_height;
			if (y < h) {
				break;
			}
			log((CLOG_DEBUG2 "skipping over screen %s", dst->m_name.c_str()));
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
		case CConfig::kLeft:
			if (!m_config.getNeighbor(dstName, CConfig::kRight).empty() &&
				x > w - 1 - lastGoodScreen->m_zoneSize)
				x = w - 1 - lastGoodScreen->m_zoneSize;
			break;

		case CConfig::kRight:
			if (!m_config.getNeighbor(dstName, CConfig::kLeft).empty() &&
				x < lastGoodScreen->m_zoneSize)
				x = lastGoodScreen->m_zoneSize;
			break;

		case CConfig::kTop:
			if (!m_config.getNeighbor(dstName, CConfig::kBottom).empty() &&
				y > h - 1 - lastGoodScreen->m_zoneSize)
				y = h - 1 - lastGoodScreen->m_zoneSize;
			break;

		case CConfig::kBottom:
			if (!m_config.getNeighbor(dstName, CConfig::kTop).empty() &&
				y < lastGoodScreen->m_zoneSize)
				y = lastGoodScreen->m_zoneSize;
			break;
		}
	}

	return lastGoodScreen;
}

void					CServer::mapPosition(CScreenInfo* src,
								CConfig::EDirection srcSide,
								CScreenInfo* dst,
								SInt32& x, SInt32& y) const
{
	assert(src != NULL);
	assert(dst != NULL);
	assert(srcSide >= CConfig::kFirstDirection &&
		   srcSide <= CConfig::kLastDirection);

	switch (srcSide) {
	case CConfig::kLeft:
	case CConfig::kRight:
		if (y < 0)
			y = 0;
		else if (y >= src->m_height)
			y = dst->m_height - 1;
		else
			y = static_cast<SInt32>(0.5 + y *
								static_cast<double>(dst->m_height - 1) /
													(src->m_height - 1));
		break;

	case CConfig::kTop:
	case CConfig::kBottom:
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
	log((CLOG_DEBUG1 "starting to wait for clients"));

	// add this thread to the list of threads to cancel.  remove from
	// list in d'tor.
	CCleanupNote cleanupNote(this);

	std::auto_ptr<IListenSocket> listen;
	try {
		// create socket listener
//		listen = std::auto_ptr<IListenSocket>(m_socketFactory->createListen());
		assign(listen, new CTCPListenSocket, IListenSocket); // FIXME

		// bind to the desired port.  keep retrying if we can't bind
		// the address immediately.
		CStopwatch timer;
		CNetworkAddress addr(50001 /* FIXME -- m_port */);
		for (;;) {
			try {
				log((CLOG_DEBUG1 "binding listen socket"));
				listen->bind(addr);
				break;
			}
			catch (XSocketAddressInUse&) {
				// give up if we've waited too long
				if (timer.getTime() >= m_bindTimeout) {
					log((CLOG_DEBUG1 "waited too long to bind, giving up"));
					throw;
				}

				// wait a bit before retrying
				log((CLOG_DEBUG1 "bind failed;  waiting to retry"));
				CThread::sleep(5.0);
			}
		}

		// accept connections and begin processing them
		log((CLOG_DEBUG1 "waiting for client connections"));
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
	log((CLOG_DEBUG1 "negotiating with new client"));

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
		assign(input, new CInputPacketStream(srcInput, own), IInputStream);
		assign(output, new COutputPacketStream(srcOutput, own), IOutputStream);

		std::auto_ptr<IServerProtocol> protocol;
		std::auto_ptr<CConnectionNote> connectedNote;
		try {
			{
				// give the client a limited time to complete the handshake
				CTimerThread timer(30.0);

				// limit the maximum length of the hello
				static const UInt32 maxHelloLen = 1024;

				// say hello
				log((CLOG_DEBUG1 "saying hello"));
				CProtocolUtil::writef(output.get(), "Synergy%2i%2i",
										kMajorVersion, kMinorVersion);
				output->flush();

				// wait for the reply
				log((CLOG_DEBUG1 "waiting for hello reply"));
				UInt32 n = input->getSize();
				if (n > maxHelloLen) {
					throw XBadClient();
				}

				// get and parse the reply to hello
				SInt16 major, minor;
				try {
					log((CLOG_DEBUG1 "parsing hello reply"));
					CProtocolUtil::readf(input.get(), "Synergy%2i%2i%s",
										&major, &minor, &name);
				}
				catch (XIO&) {
					throw XBadClient();
				}

				// create a protocol interpreter for the version
				log((CLOG_DEBUG1 "creating interpreter for client \"%s\" version %d.%d", name.c_str(), major, minor));
				assign(protocol, CServerProtocol::create(major, minor,
									this, name, input.get(), output.get()),
									IServerProtocol);

				// client is now pending
				assign(connectedNote, new CConnectionNote(this,
									name, protocol.get()), CConnectionNote);

				// ask and wait for the client's info
				log((CLOG_DEBUG1 "waiting for info for client \"%s\"", name.c_str()));
				protocol->queryInfo();

				// now connected;  client no longer subject to timeout.
			}

			// handle messages from client.  returns when the client
			// disconnects.
			log((CLOG_NOTE "client \"%s\" has connected", name.c_str()));
			protocol->run();
		}
		catch (XDuplicateClient& e) {
			// client has duplicate name
			log((CLOG_WARN "a client with name \"%s\" is already connected", e.getName().c_str()));
			CProtocolUtil::writef(output.get(), kMsgEBusy);
		}
		catch (XUnknownClient& e) {
			// client has unknown name
			log((CLOG_WARN "a client with name \"%s\" is not in the map", e.getName().c_str()));
			CProtocolUtil::writef(output.get(), kMsgEUnknown);
		}
		catch (XIncompatibleClient& e) {
			// client is incompatible
			// FIXME -- could print network address if socket had suitable method
			log((CLOG_WARN "client \"%s\" has incompatible version %d.%d)", name.c_str(), e.getMajor(), e.getMinor()));
			CProtocolUtil::writef(output.get(), kMsgEIncompatible,
								kMajorVersion, kMinorVersion);
		}
		catch (XBadClient&) {
			// client not behaving
			// FIXME -- could print network address if socket had suitable method
			log((CLOG_WARN "protocol error from client \"%s\"", name.c_str()));
			CProtocolUtil::writef(output.get(), kMsgEBad);
		}

		// flush any pending output
		output.get()->flush();
	}
	catch (XBase& e) {
		// misc error
		log((CLOG_WARN "error communicating with client \"%s\": %s", name.c_str(), e.what()));
		// FIXME -- could print network address if socket had suitable method
	}
}

void					CServer::acceptHTTPClients(void*)
{
	log((CLOG_DEBUG1 "starting to wait for HTTP clients"));

	// add this thread to the list of threads to cancel.  remove from
	// list in d'tor.
	CCleanupNote cleanupNote(this);

	std::auto_ptr<IListenSocket> listen;
	try {
		// create socket listener
//		listen = std::auto_ptr<IListenSocket>(m_socketFactory->createListen());
		assign(listen, new CTCPListenSocket, IListenSocket); // FIXME

		// bind to the desired port.  keep retrying if we can't bind
		// the address immediately.
		CStopwatch timer;
		CNetworkAddress addr(50002 /* FIXME -- m_httpPort */);
		for (;;) {
			try {
				log((CLOG_DEBUG1 "binding listen socket"));
				listen->bind(addr);
				break;
			}
			catch (XSocketAddressInUse&) {
				// give up if we've waited too long
				if (timer.getTime() >= m_bindTimeout) {
					log((CLOG_DEBUG1 "waited too long to bind HTTP, giving up"));
					throw;
				}

				// wait a bit before retrying
				log((CLOG_DEBUG1 "bind HTTP failed;  waiting to retry"));
				CThread::sleep(5.0);
			}
		}

		// accept connections and begin processing them
		log((CLOG_DEBUG1 "waiting for HTTP connections"));
		for (;;) {
			// limit the number of HTTP requests being handled at once
			{
				CLock lock(&m_httpAvailable);
				while (m_httpAvailable == 0) {
					m_httpAvailable.wait();
				}
				assert(m_httpAvailable > 0);
				m_httpAvailable = m_httpAvailable - 1;
			}

			// accept connection
			CThread::testCancel();
			ISocket* socket = listen->accept();
			log((CLOG_NOTE "accepted HTTP connection"));
			CThread::testCancel();

			// handle HTTP request
			CThread(new TMethodJob<CServer>(
								this, &CServer::processHTTPRequest, socket));
		}
	}
	catch (XBase& e) {
		log((CLOG_ERR "cannot listen for HTTP clients: %s", e.what()));
		// FIXME -- quit?
		quit();
	}
}

void					CServer::processHTTPRequest(void* vsocket)
{
	// add this thread to the list of threads to cancel.  remove from
	// list in d'tor.
	CCleanupNote cleanupNote(this);

	ISocket* socket = reinterpret_cast<ISocket*>(vsocket);
	try {
		// process the request and force delivery
		m_httpServer->processRequest(socket);
		socket->getOutputStream()->flush();

		// wait a moment to give the client a chance to hangup first
		CThread::sleep(3.0);

		// clean up
		socket->close();
		delete socket;

		// increment available HTTP handlers
		{
			CLock lock(&m_httpAvailable);
			m_httpAvailable = m_httpAvailable + 1;
			m_httpAvailable.signal();
		}
	}
	catch (...) {
		delete socket;
		{
			CLock lock(&m_httpAvailable);
			m_httpAvailable = m_httpAvailable + 1;
			m_httpAvailable.signal();
		}
		throw;
	}
}

void					CServer::clearGotClipboard(ClipboardID id)
{
	for (CScreenList::const_iterator index = m_screens.begin();
								index != m_screens.end(); ++index) {
		index->second->m_gotClipboard[id] = false;
	}
}

void					CServer::sendClipboard(ClipboardID id)
{
	// do nothing if clipboard was already sent
	if (!m_active->m_gotClipboard[id]) {
		CClipboardInfo& clipboard = m_clipboards[id];
		if (clipboard.m_clipboardReady) {
			// send it
			if (m_active->m_protocol == NULL) {
				m_primary->setClipboard(id, &clipboard.m_clipboard);
			}
			else {
				m_active->m_protocol->sendClipboard(id,
								clipboard.m_clipboardData);
			}

			// clipboard has been sent
			m_active->m_gotClipboard[id] = true;
		}
	}
}

void					CServer::updatePrimaryClipboard(ClipboardID id)
{
	CClipboardInfo& clipboard = m_clipboards[id];

	// if leaving primary and the primary owns the clipboard
	// then update it.
	if (clipboard.m_clipboardOwner == m_primaryInfo->m_name) {
		assert(clipboard.m_clipboardReady == true);

		// save clipboard time
		IClipboard::Time time = clipboard.m_clipboard.getTime();

		// update
		m_primary->getClipboard(id, &clipboard.m_clipboard);

		// if clipboard changed then other screens have an
		// out-of-date clipboard.
		if (time != clipboard.m_clipboard.getTime()) {
			log((CLOG_DEBUG "clipboard %d time changed (%08x to %08x)", id, time, clipboard.m_clipboard.getTime()));

			// marshall data
			CString newData = clipboard.m_clipboard.marshall();

			// compare old and new data.  if identical then the clipboard
			// hasn't really changed.
			if (newData != clipboard.m_clipboardData) {
				log((CLOG_DEBUG "clipboard %d changed", id));
				clipboard.m_clipboardData = newData;
				clearGotClipboard(id);
			}
			else {
				log((CLOG_DEBUG "clipboard %d unchanged", id));
			}
			m_primaryInfo->m_gotClipboard[id] = true;
		}
	}
}

// FIXME -- use factory to create screen
#if defined(CONFIG_PLATFORM_WIN32)
#include "CMSWindowsPrimaryScreen.h"
#elif defined(CONFIG_PLATFORM_UNIX)
#include "CXWindowsPrimaryScreen.h"
#endif
void					CServer::openPrimaryScreen()
{
	assert(m_primary == NULL);

	// reset sequence number
	m_seqNum = 0;

	try {
		// add connection
		m_active      = addConnection(CString("primary"/* FIXME */), NULL);
		m_primaryInfo = m_active;

		// open screen
		log((CLOG_DEBUG1 "creating primary screen"));
#if defined(CONFIG_PLATFORM_WIN32)
		m_primary = new CMSWindowsPrimaryScreen;
#elif defined(CONFIG_PLATFORM_UNIX)
		m_primary = new CXWindowsPrimaryScreen;
#endif
		log((CLOG_DEBUG1 "opening primary screen"));
		m_primary->open(this);
	}
	catch (...) {
		delete m_primary;
		removeConnection(CString("primary"/* FIXME */));
		m_primary     = NULL;
		m_primaryInfo = NULL;
		m_active      = NULL;
		throw;
	}

	// set the clipboard owner to the primary screen and then get the
	// current clipboard data.
	for (ClipboardID id = 0; id < kClipboardEnd; ++id) {
		CClipboardInfo& clipboard = m_clipboards[id];
		m_primary->getClipboard(id, &clipboard.m_clipboard);
		clipboard.m_clipboardData  = clipboard.m_clipboard.marshall();
		clipboard.m_clipboardReady = true;
		clipboard.m_clipboardOwner = m_active->m_name;
	}
}

void					CServer::closePrimaryScreen()
{
	assert(m_primary != NULL);

	// remove connection
	removeConnection(CString("primary"/* FIXME */));

	// close the primary screen
	try {
		log((CLOG_DEBUG1 "closing primary screen"));
		m_primary->close();
	}
	catch (...) {
		// ignore
	}

	// clean up
	log((CLOG_DEBUG1 "destroying primary screen"));
	delete m_primary;
	m_primary = NULL;
}

void					CServer::addCleanupThread(const CThread& thread)
{
	CLock lock(&m_mutex);
	m_cleanupList.insert(m_cleanupList.begin(), new CThread(thread));
	m_cleanupSize = m_cleanupSize + 1;
}

void					CServer::removeCleanupThread(const CThread& thread)
{
	CLock lock(&m_mutex);
	for (CThreadList::iterator index = m_cleanupList.begin();
								index != m_cleanupList.end(); ++index) {
		if (**index == thread) {
			CThread* thread = *index;
			m_cleanupList.erase(index);
			m_cleanupSize = m_cleanupSize - 1;
			if (m_cleanupSize == 0) {
				m_cleanupSize.broadcast();
			}
			delete thread;
			return;
		}
	}
}

void					CServer::cleanupThreads(double timeout)
{
	log((CLOG_DEBUG1 "cleaning up threads"));

	// first cancel every thread except the current one (with mutex
	// locked so the cleanup list won't change).
	CLock lock(&m_mutex);
	CThread current(CThread::getCurrentThread());
	SInt32 minCount = 0;
	for (CThreadList::iterator index = m_cleanupList.begin();
								index != m_cleanupList.end(); ++index) {
		CThread* thread = *index;
		if (thread != &current) {
			thread->cancel();
		}
		else {
			minCount = 1;
		}
	}

	// now wait for the threads (with mutex unlocked as each thread
	// will remove itself from the list)
	CStopwatch timer(true);
	while (m_cleanupSize > minCount) {
		m_cleanupSize.wait(timer, timeout);
	}

	// delete remaining threads
	for (CThreadList::iterator index = m_cleanupList.begin();
								index != m_cleanupList.end(); ++index) {
		CThread* thread = *index;
		delete thread;
	}
	m_cleanupList.clear();
	m_cleanupSize = 0;

	log((CLOG_DEBUG1 "cleaned up threads"));
}

CServer::CScreenInfo*	CServer::addConnection(
								const CString& name, IServerProtocol* protocol)
{
	log((CLOG_DEBUG "adding connection \"%s\"", name.c_str()));

	CLock lock(&m_mutex);

	// can only have one screen with a given name at any given time
	if (m_screens.count(name) != 0) {
		throw XDuplicateClient(name);
	}

	// name must be in our configuration
	if (!m_config.isScreen(name)) {
		throw XUnknownClient(name);
	}

	// save screen info
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
	if (m_active == index->second && m_active != m_primaryInfo) {
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
								m_thread(CThread::getCurrentThread()),
								m_name(name),
								m_protocol(protocol),
								m_ready(false),
								m_width(0), m_height(0),
								m_zoneSize(0)
{
	for (ClipboardID id = 0; id < kClipboardEnd; ++id)
		m_gotClipboard[id] = false;
}

CServer::CScreenInfo::~CScreenInfo()
{
	// do nothing
}


//
// CServer::CClipboardInfo
//

CServer::CClipboardInfo::CClipboardInfo() :
								m_clipboard(),
								m_clipboardData(),
								m_clipboardOwner(),
								m_clipboardSeqNum(0),
								m_clipboardReady(false)
{
	// do nothing
}
