#include "CServer.h"
#include "CHTTPServer.h"
#include "CInputPacketStream.h"
#include "COutputPacketStream.h"
#include "CProtocolUtil.h"
#include "CServerProtocol.h"
#include "IPrimaryScreen.h"
#include "ProtocolTypes.h"
#include "XScreen.h"
#include "XSynergy.h"
#include "CNetworkAddress.h"
#include "IDataSocket.h"
#include "IListenSocket.h"
#include "ISocketFactory.h"
#include "XSocket.h"
#include "CLock.h"
#include "CThread.h"
#include "CTimerThread.h"
#include "XThread.h"
#include "CFunctionJob.h"
#include "CLog.h"
#include "CStopwatch.h"
#include "TMethodJob.h"
#include <memory>

// hack to work around operator=() bug in STL in g++ prior to v3
#if defined(__GNUC__) && (__GNUC__ < 3)
#define assign(_dst, _src, _type)	_dst.reset(_src)
#else
#define assign(_dst, _src, _type)	_dst = std::auto_ptr<_type >(_src)
#endif

//
// CServer
//

const SInt32			CServer::s_httpMaxSimultaneousRequests = 3;

CServer::CServer(const CString& serverName) :
	m_name(serverName),
	m_primary(NULL),
	m_active(NULL),
	m_primaryInfo(NULL),
	m_seqNum(0),
	m_httpServer(NULL),
	m_httpAvailable(&m_mutex, s_httpMaxSimultaneousRequests)
{
	m_socketFactory = NULL;
	m_securityFactory = NULL;
	m_bindTimeout = 5.0 * 60.0;
}

CServer::~CServer()
{
	// do nothing
}

bool
CServer::open()
{
	// open the screen
	try {
		log((CLOG_INFO "opening screen"));
		openPrimaryScreen();
		return true;
	}
	catch (XScreenOpenFailure&) {
		// can't open screen yet.  wait a few seconds to retry.
		CThread::sleep(3.0);
		log((CLOG_INFO "failed to open screen"));
		return false;
	}
	catch (XUnknownClient& e) {
		// can't open screen yet.  wait a few seconds to retry.
		CThread::sleep(3.0);
		log((CLOG_CRIT "unknown screen name `%s'", e.getName().c_str()));
		return false;
	}
}

void
CServer::run()
{
	// check preconditions
	{
		CLock lock(&m_mutex);
		assert(m_primary != NULL);
	}

	try {
		log((CLOG_NOTE "starting server"));

		// start listening for new clients
		startThread(new TMethodJob<CServer>(this, &CServer::acceptClients));

		// start listening for HTTP requests
		if (m_config.getHTTPAddress().isValid()) {
			m_httpServer = new CHTTPServer(this);
			startThread(new TMethodJob<CServer>(this,
								&CServer::acceptHTTPClients));
		}

		// handle events
		log((CLOG_DEBUG "starting event handling"));
		m_primary->run();

		// clean up
		log((CLOG_NOTE "stopping server"));
		stopThreads();
		delete m_httpServer;
		m_httpServer = NULL;
		closePrimaryScreen();
	}
	catch (XBase& e) {
		log((CLOG_ERR "server error: %s", e.what()));

		// clean up
		log((CLOG_NOTE "stopping server"));
		stopThreads();
		delete m_httpServer;
		m_httpServer = NULL;
		if (m_primary != NULL) {
			closePrimaryScreen();
		}
	}
	catch (XThread&) {
		// clean up
		log((CLOG_NOTE "stopping server"));
		stopThreads();
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
		stopThreads();
		delete m_httpServer;
		m_httpServer = NULL;
		if (m_primary != NULL) {
			closePrimaryScreen();
		}
		throw;
	}
}

void
CServer::quit()
{
	m_primary->stop();
}

void
CServer::shutdown()
{
	// stop all running threads but don't wait too long since some
	// threads may be unable to proceed until this thread returns.
	stopThreads(3.0);

	// done with the HTTP server
	delete m_httpServer;
	m_httpServer = NULL;

	// note -- we do not attempt to close down the primary screen
}

bool
CServer::setConfig(const CConfig& config)
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
		// dropped from the configuration (or who's canonical name
		// is changing).  don't add the primary screen.  also tell
		// the secondary screen to disconnect.
		for (CScreenList::const_iterator index = m_screens.begin();
								index != m_screens.end(); ++index) {
			if (index->second != m_primaryInfo &&
				!config.isCanonicalName(index->first)) {
				assert(index->second->m_protocol != NULL);
				index->second->m_protocol->sendClose();
				threads.push_back(index->second->m_thread);
			}
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

	// wait for old secondary screen threads to disconnect.  must
	// not hold lock while we do this so those threads can finish
	// any calls to this object.
	for (CThreads::iterator index = threads.begin();
								index != threads.end(); ++index) {
		index->wait();
	}

	// clean up thread list
	reapThreads();

	// cut over
	CLock lock(&m_mutex);
	m_config = config;

	// tell primary screen about reconfiguration
	if (m_primary != NULL) {
		m_primary->onConfigure();
	}

	return true;
}

CString
CServer::getPrimaryScreenName() const
{
	return m_name;
}

void
CServer::getConfig(CConfig* config) const
{
	assert(config != NULL);

	CLock lock(&m_mutex);
	*config = m_config;
}

UInt32
CServer::getActivePrimarySides() const
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

void
CServer::setInfo(SInt32 x, SInt32 y, SInt32 w, SInt32 h,
				SInt32 zoneSize, SInt32 mx, SInt32 my)
{
	CLock lock(&m_mutex);
	assert(m_primaryInfo != NULL);
	setInfoNoLock(m_primaryInfo->m_name, x, y, w, h, zoneSize, mx, my);
}

void
CServer::setInfo(const CString& client,
				SInt32 x, SInt32 y, SInt32 w, SInt32 h,
				SInt32 zoneSize, SInt32 mx, SInt32 my)
{
	CLock lock(&m_mutex);
	setInfoNoLock(client, x, y, w, h, zoneSize, mx, my);
}

void
CServer::setInfoNoLock(const CString& screen,
				SInt32 x, SInt32 y, SInt32 w, SInt32 h,
				SInt32 zoneSize, SInt32 mx, SInt32 my)
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
		m_x = mx;
		m_y = my;
	}
	info->m_x        = x;
	info->m_y        = y;
	info->m_w        = w;
	info->m_h        = h;
	info->m_zoneSize = zoneSize;
	log((CLOG_INFO "screen \"%s\" shape=%d,%d %dx%d zone=%d pos=%d,%d", screen.c_str(), x, y, w, h, zoneSize, mx, my));

	// send acknowledgement (if screen isn't the primary)
	if (info->m_protocol != NULL) {
		info->m_protocol->sendInfoAcknowledgment();
	}

	// handle resolution change to primary screen
	else {
		if (info == m_active) {
			onMouseMovePrimaryNoLock(mx, my);
		}
		else {
			onMouseMoveSecondaryNoLock(0, 0);
		}
	}
}

void
CServer::grabClipboard(ClipboardID id)
{
	CLock lock(&m_mutex);
	assert(m_primaryInfo != NULL);
	grabClipboardNoLock(id, 0, m_primaryInfo->m_name);
}

void
CServer::grabClipboard(ClipboardID id, UInt32 seqNum, const CString& client)
{
	CLock lock(&m_mutex);
	grabClipboardNoLock(id, seqNum, client);
}

void
CServer::grabClipboardNoLock(ClipboardID id,
				UInt32 seqNum, const CString& screen)
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
	log((CLOG_INFO "screen \"%s\" grabbed clipboard %d from \"%s\"", screen.c_str(), id, clipboard.m_clipboardOwner.c_str()));
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

void
CServer::setClipboard(ClipboardID id, UInt32 seqNum, const CString& data)
{
	CLock lock(&m_mutex);
	CClipboardInfo& clipboard = m_clipboards[id];

	// ignore update if sequence number is old
	if (seqNum < clipboard.m_clipboardSeqNum) {
		log((CLOG_INFO "ignored screen \"%s\" update of clipboard %d", clipboard.m_clipboardOwner.c_str(), id));
		return;
	}

	// unmarshall into our clipboard buffer
	log((CLOG_INFO "screen \"%s\" updated clipboard %d", clipboard.m_clipboardOwner.c_str(), id));
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

bool
CServer::onCommandKey(KeyID /*id*/, KeyModifierMask /*mask*/, bool /*down*/)
{
	return false;
}

void
CServer::onKeyDown(KeyID id, KeyModifierMask mask)
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

void
CServer::onKeyUp(KeyID id, KeyModifierMask mask)
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

void
CServer::onKeyRepeat(KeyID id, KeyModifierMask mask, SInt32 count)
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

void
CServer::onMouseDown(ButtonID id)
{
	log((CLOG_DEBUG1 "onMouseDown id=%d", id));
	CLock lock(&m_mutex);
	assert(m_active != NULL);

	// relay
	if (m_active->m_protocol != NULL) {
		m_active->m_protocol->sendMouseDown(id);
	}
}

void
CServer::onMouseUp(ButtonID id)
{
	log((CLOG_DEBUG1 "onMouseUp id=%d", id));
	CLock lock(&m_mutex);
	assert(m_active != NULL);

	// relay
	if (m_active->m_protocol != NULL) {
		m_active->m_protocol->sendMouseUp(id);
	}
}

bool
CServer::onMouseMovePrimary(SInt32 x, SInt32 y)
{
	log((CLOG_DEBUG2 "onMouseMovePrimary %d,%d", x, y));
	CLock lock(&m_mutex);
	return onMouseMovePrimaryNoLock(x, y);
}

bool
CServer::onMouseMovePrimaryNoLock(SInt32 x, SInt32 y)
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
	if (x < m_active->m_x + m_active->m_zoneSize) {
		x  -= m_active->m_zoneSize;
		dir = CConfig::kLeft;
		log((CLOG_DEBUG1 "switch to left"));
	}
	else if (x >= m_active->m_x + m_active->m_w - m_active->m_zoneSize) {
		x  += m_active->m_zoneSize;
		dir = CConfig::kRight;
		log((CLOG_DEBUG1 "switch to right"));
	}
	else if (y < m_active->m_y + m_active->m_zoneSize) {
		y  -= m_active->m_zoneSize;
		dir = CConfig::kTop;
		log((CLOG_DEBUG1 "switch to top"));
	}
	else if (y >= m_active->m_y + m_active->m_h - m_active->m_zoneSize) {
		y  += m_active->m_zoneSize;
		dir = CConfig::kBottom;
		log((CLOG_DEBUG1 "switch to bottom"));
	}
	else {
		// still on local screen
		return false;
	}

	// get jump destination and, if no screen in jump direction,
	// then ignore the move.
	CScreenInfo* newScreen = getNeighbor(m_active, dir, x, y);
	if (newScreen == NULL) {
		return false;
	}

	// switch screen
	switchScreen(newScreen, x, y);
	return true;
}

void
CServer::onMouseMoveSecondary(SInt32 dx, SInt32 dy)
{
	log((CLOG_DEBUG2 "onMouseMoveSecondary %+d,%+d", dx, dy));
	CLock lock(&m_mutex);
	onMouseMoveSecondaryNoLock(dx, dy);
}

void
CServer::onMouseMoveSecondaryNoLock(SInt32 dx, SInt32 dy)
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
		if (m_x < m_active->m_x) {
			dir = CConfig::kLeft;
		}
		else if (m_x > m_active->m_x + m_active->m_w - 1) {
			dir = CConfig::kRight;
		}
		else if (m_y < m_active->m_y) {
			dir = CConfig::kTop;
		}
		else if (m_y > m_active->m_y + m_active->m_h - 1) {
			dir = CConfig::kBottom;
		}
		else {
			newScreen = m_active;

			// keep compiler quiet about unset variable
			dir = CConfig::kLeft;
		}

		// get neighbor if we should switch
		if (newScreen == NULL) {
			log((CLOG_DEBUG1 "leave \"%s\" on %s", m_active->m_name.c_str(), CConfig::dirName(dir)));

			// get new position or clamp to current screen
			newScreen = getNeighbor(m_active, dir, m_x, m_y);
			if (newScreen == NULL) {
				log((CLOG_DEBUG1 "no neighbor; clamping"));
				if (m_x < m_active->m_x)
					m_x = m_active->m_x;
				else if (m_x > m_active->m_x + m_active->m_w - 1)
					m_x = m_active->m_x + m_active->m_w - 1;
				if (m_y < m_active->m_y)
					m_y = m_active->m_y;
				else if (m_y > m_active->m_y + m_active->m_h - 1)
					m_y = m_active->m_y + m_active->m_h - 1;
			}
		}
	}
	else {
		// clamp to edge when locked
		log((CLOG_DEBUG1 "clamp to \"%s\"", m_active->m_name.c_str()));
		if (m_x < m_active->m_x)
			m_x = m_active->m_x;
		else if (m_x > m_active->m_x + m_active->m_w - 1)
			m_x = m_active->m_x + m_active->m_w - 1;
		if (m_y < m_active->m_y)
			m_y = m_active->m_y;
		else if (m_y > m_active->m_y + m_active->m_h - 1)
			m_y = m_active->m_y + m_active->m_h - 1;
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

void
CServer::onMouseWheel(SInt32 delta)
{
	log((CLOG_DEBUG1 "onMouseWheel %+d", delta));
	CLock lock(&m_mutex);
	assert(m_active != NULL);

	// relay
	if (m_active->m_protocol != NULL) {
		m_active->m_protocol->sendMouseWheel(delta);
	}
}

void
CServer::onScreenSaver(bool activated)
{
	log((CLOG_DEBUG "onScreenSaver %s", activated ? "activated" : "deactivated"));
	CLock lock(&m_mutex);

	if (activated) {
		// save current screen and position
		m_activeSaver = m_active;
		m_xSaver      = m_x;
		m_ySaver      = m_y;

		// jump to primary screen
		if (m_active != m_primaryInfo) {
// FIXME -- should have separate "center" pixel reported by screen
			m_x = m_primaryInfo->m_x + (m_primaryInfo->m_w >> 1);
			m_y = m_primaryInfo->m_y + (m_primaryInfo->m_h >> 1);
			m_active = m_primaryInfo;
			m_primary->enter(m_x, m_y);
		}
	}
	else {
		// jump back to previous screen and position.  we must check
		// that the position is still valid since the screen may have
		// changed resolutions while the screen saver was running.
		if (m_activeSaver != NULL && m_activeSaver != m_primaryInfo) {
			// check position
			CScreenInfo* screen = m_activeSaver;
			if (m_xSaver < screen->m_x + screen->m_zoneSize) {
				m_xSaver = screen->m_x + screen->m_zoneSize;
			}
			else if (m_xSaver >= screen->m_x +
								screen->m_w - screen->m_zoneSize) {
				m_xSaver = screen->m_x + screen->m_w - screen->m_zoneSize - 1;
			}
			if (m_ySaver < screen->m_y + screen->m_zoneSize) {
				m_ySaver = screen->m_y + screen->m_zoneSize;
			}
			else if (m_ySaver >= screen->m_y +
								screen->m_h - screen->m_zoneSize) {
				m_ySaver = screen->m_y + screen->m_h - screen->m_zoneSize - 1;
			}

			// now jump
			switchScreen(screen, m_xSaver, m_ySaver);
		}

		// reset state
		m_activeSaver = NULL;
	}

	// send message to all secondary screens
	for (CScreenList::const_iterator index = m_screens.begin();
								index != m_screens.end(); ++index) {
		if (index->second->m_protocol != NULL) {
			index->second->m_protocol->sendScreenSaver(activated);
		}
	}
}

bool
CServer::isLockedToScreen() const
{
	CLock lock(&m_mutex);
	return isLockedToScreenNoLock();
}

bool
CServer::isLockedToScreenNoLock() const
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

void
CServer::switchScreen(CScreenInfo* dst, SInt32 x, SInt32 y)
{
	assert(dst != NULL);
	assert(x >= dst->m_x && y >= dst->m_y);
	assert(x < dst->m_x + dst->m_w && y < dst->m_y + dst->m_h);
	assert(m_active != NULL);

	log((CLOG_INFO "switch from \"%s\" to \"%s\" at %d,%d", m_active->m_name.c_str(), dst->m_name.c_str(), x, y));
	// FIXME -- we're not locked here but we probably should be

	// record new position
	m_x = x;
	m_y = y;

	// wrapping means leaving the active screen and entering it again.
	// since that's a waste of time we skip that and just warp the
	// mouse.
	if (m_active != dst) {
		// note if we're leaving the primary screen
		const bool leavingPrimary = (m_active->m_protocol == NULL);

		// leave active screen
		if (leavingPrimary) {
			if (!m_primary->leave()) {
				// cannot leave primary screen
				log((CLOG_WARN "can't leave primary screen"));
				return;
			}

			// update the clipboards that the primary screen owns
			for (ClipboardID id = 0; id < kClipboardEnd; ++id) {
				updatePrimaryClipboard(id);
			}
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
}

CServer::CScreenInfo*
CServer::getNeighbor(CScreenInfo* src, CConfig::EDirection dir) const
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

CServer::CScreenInfo*
CServer::getNeighbor(CScreenInfo* src,
				CConfig::EDirection srcSide, SInt32& x, SInt32& y) const
{
	assert(src != NULL);

	// get the first neighbor
	CScreenInfo* dst = getNeighbor(src, srcSide);
	if (dst == NULL) {
		return NULL;
	}

	// get the source screen's size (needed for kRight and kBottom)
	SInt32 w = src->m_w, h = src->m_h;

	// find destination screen, adjusting x or y (but not both).  the
	// searches are done in a sort of canonical screen space where
	// the upper-left corner is 0,0 for each screen.  we adjust from
	// actual to canonical position on entry to and from canonical to
	// actual on exit from the search.
	CScreenInfo* lastGoodScreen = src;
	switch (srcSide) {
	case CConfig::kLeft:
		x -= src->m_x;
		while (dst != NULL && dst != lastGoodScreen) {
			lastGoodScreen = dst;
			w = lastGoodScreen->m_w;
			h = lastGoodScreen->m_h;
			x += w;
			if (x >= 0) {
				break;
			}
			log((CLOG_DEBUG2 "skipping over screen %s", dst->m_name.c_str()));
			dst = getNeighbor(lastGoodScreen, srcSide);
		}
		assert(lastGoodScreen != NULL);
		x += lastGoodScreen->m_x;
		break;

	case CConfig::kRight:
		x -= src->m_x;
		while (dst != NULL) {
			lastGoodScreen = dst;
			x -= w;
			w = lastGoodScreen->m_w;
			h = lastGoodScreen->m_h;
			if (x < w) {
				break;
			}
			log((CLOG_DEBUG2 "skipping over screen %s", dst->m_name.c_str()));
			dst = getNeighbor(lastGoodScreen, srcSide);
		}
		assert(lastGoodScreen != NULL);
		x += lastGoodScreen->m_x;
		break;

	case CConfig::kTop:
		y -= src->m_y;
		while (dst != NULL) {
			lastGoodScreen = dst;
			w = lastGoodScreen->m_w;
			h = lastGoodScreen->m_h;
			y += h;
			if (y >= 0) {
				break;
			}
			log((CLOG_DEBUG2 "skipping over screen %s", dst->m_name.c_str()));
			dst = getNeighbor(lastGoodScreen, srcSide);
		}
		assert(lastGoodScreen != NULL);
		y += lastGoodScreen->m_y;
		break;

	case CConfig::kBottom:
		y -= src->m_y;
		while (dst != NULL) {
			lastGoodScreen = dst;
			y -= h;
			w = lastGoodScreen->m_w;
			h = lastGoodScreen->m_h;
			if (y < h) {
				break;
			}
			log((CLOG_DEBUG2 "skipping over screen %s", dst->m_name.c_str()));
			dst = getNeighbor(lastGoodScreen, srcSide);
		}
		assert(lastGoodScreen != NULL);
		y += lastGoodScreen->m_y;
		break;
	}

	// save destination screen
	assert(lastGoodScreen != NULL);
	dst = lastGoodScreen;

	// if entering primary screen then be sure to move in far enough
	// to avoid the jump zone.  if entering a side that doesn't have
	// a neighbor (i.e. an asymmetrical side) then we don't need to
	// move inwards because that side can't provoke a jump.
	if (dst->m_protocol == NULL) {
		const CString dstName(dst->m_name);
		switch (srcSide) {
		case CConfig::kLeft:
			if (!m_config.getNeighbor(dstName, CConfig::kRight).empty() &&
				x > dst->m_x + w - 1 - dst->m_zoneSize)
				x = dst->m_x + w - 1 - dst->m_zoneSize;
			break;

		case CConfig::kRight:
			if (!m_config.getNeighbor(dstName, CConfig::kLeft).empty() &&
				x < dst->m_x + dst->m_zoneSize)
				x = dst->m_x + dst->m_zoneSize;
			break;

		case CConfig::kTop:
			if (!m_config.getNeighbor(dstName, CConfig::kBottom).empty() &&
				y > dst->m_y + h - 1 - dst->m_zoneSize)
				y = dst->m_y + h - 1 - dst->m_zoneSize;
			break;

		case CConfig::kBottom:
			if (!m_config.getNeighbor(dstName, CConfig::kTop).empty() &&
				y < dst->m_y + dst->m_zoneSize)
				y = dst->m_y + dst->m_zoneSize;
			break;
		}
	}

	// adjust the coordinate orthogonal to srcSide to account for
	// resolution differences.  for example, if y is 200 pixels from
	// the top on a screen 1000 pixels high (20% from the top) when
	// we cross the left edge onto a screen 600 pixels high then y
	// should be set 120 pixels from the top (again 20% from the
	// top).
	switch (srcSide) {
	case CConfig::kLeft:
	case CConfig::kRight:
		y -= src->m_y;
		if (y < 0) {
			y = 0;
		}
		else if (y >= src->m_h) {
			y = dst->m_h - 1;
		}
		else {
			y = static_cast<SInt32>(0.5 + y *
								static_cast<double>(dst->m_h - 1) /
													(src->m_h - 1));
		}
		y += dst->m_y;
		break;

	case CConfig::kTop:
	case CConfig::kBottom:
		x -= src->m_x;
		if (x < 0) {
			x = 0;
		}
		else if (x >= src->m_w) {
			x = dst->m_w - 1;
		}
		else {
			x = static_cast<SInt32>(0.5 + x *
								static_cast<double>(dst->m_w - 1) /
													(src->m_w - 1));
		}
		x += dst->m_x;
		break;
	}

	return dst;
}

void
CServer::startThread(IJob* job)
{
	CLock lock(&m_mutex);
	doReapThreads(m_threads);
	CThread* thread = new CThread(job);
	m_threads.push_back(thread);
	log((CLOG_DEBUG1 "started thread %p", thread));
}

void
CServer::stopThreads(double timeout)
{
	log((CLOG_DEBUG1 "stopping threads"));

	// swap thread list so nobody can mess with it
	CThreadList threads;
	{
		CLock lock(&m_mutex);
		threads.swap(m_threads);
	}

	// cancel every thread
	for (CThreadList::iterator index = threads.begin();
								index != threads.end(); ++index) {
		CThread* thread = *index;
		thread->cancel();
	}

	// now wait for the threads
	CStopwatch timer(true);
	while (threads.size() > 0 && (timeout < 0.0 || timer.getTime() < timeout)) {
		doReapThreads(threads);
		CThread::sleep(0.01);
	}

	// delete remaining threads
	for (CThreadList::iterator index = threads.begin();
								index != threads.end(); ++index) {
		CThread* thread = *index;
		log((CLOG_DEBUG1 "reaped running thread %p", thread));
		delete thread;
	}

	log((CLOG_DEBUG1 "stopped threads"));
}

void
CServer::reapThreads()
{
	CLock lock(&m_mutex);
	doReapThreads(m_threads);
}

void
CServer::doReapThreads(CThreadList& threads)
{
	for (CThreadList::iterator index = threads.begin();
								index != threads.end(); ) {
		CThread* thread = *index;
		if (thread->wait(0.0)) {
			// thread terminated
			index = threads.erase(index);
			log((CLOG_DEBUG1 "reaped thread %p", thread));
			delete thread;
		}
		else {
			// thread is running
			++index;
		}
	}
}

#include "CTCPListenSocket.h"
void
CServer::acceptClients(void*)
{
	log((CLOG_DEBUG1 "starting to wait for clients"));

	std::auto_ptr<IListenSocket> listen;
	try {
		// create socket listener
//		listen = std::auto_ptr<IListenSocket>(m_socketFactory->createListen());
		assign(listen, new CTCPListenSocket, IListenSocket); // FIXME

		// bind to the desired port.  keep retrying if we can't bind
		// the address immediately.
		CStopwatch timer;
		for (;;) {
			try {
				log((CLOG_DEBUG1 "binding listen socket"));
				listen->bind(m_config.getSynergyAddress());
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
			IDataSocket* socket = listen->accept();
			log((CLOG_NOTE "accepted client connection"));
			CThread::testCancel();

			// start handshake thread
			startThread(new TMethodJob<CServer>(
								this, &CServer::handshakeClient, socket));
		}
	}
	catch (XBase& e) {
		log((CLOG_ERR "cannot listen for clients: %s", e.what()));
		quit();
	}
}

void
CServer::handshakeClient(void* vsocket)
{
	log((CLOG_DEBUG1 "negotiating with new client"));

	// get the socket pointer from the argument
	assert(vsocket != NULL);
	std::auto_ptr<IDataSocket> socket(reinterpret_cast<IDataSocket*>(vsocket));

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

		bool connected = false;
		std::auto_ptr<IServerProtocol> protocol;
		try {
			{
				// give the client a limited time to complete the handshake
				CTimerThread timer(30.0);

				// limit the maximum length of the hello
				static const UInt32 maxHelloLen = 1024;

				// say hello
				log((CLOG_DEBUG1 "saying hello"));
				CProtocolUtil::writef(output.get(), "Synergy%2i%2i",
										kProtocolMajorVersion,
										kProtocolMinorVersion);
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

				// convert name to canonical form (if any)
				if (m_config.isScreen(name)) {
					name = m_config.getCanonicalName(name);
				}

				// create a protocol interpreter for the version
				log((CLOG_DEBUG1 "creating interpreter for client \"%s\" version %d.%d", name.c_str(), major, minor));
				assign(protocol, CServerProtocol::create(major, minor,
									this, name, input.get(), output.get()),
									IServerProtocol);

				// client is now pending
				addConnection(name, protocol.get());
				connected = true;

				// ask and wait for the client's info
				log((CLOG_DEBUG1 "waiting for info for client \"%s\"", name.c_str()));
				protocol->queryInfo();

				// now connected;  client no longer subject to timeout.
			}

			// activate screen saver on new client if active on the
			// primary screen
			if (m_primary->isScreenSaverActive()) {
				protocol->sendScreenSaver(true);
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
								kProtocolMajorVersion, kProtocolMinorVersion);
		}
		catch (XBadClient&) {
			// client not behaving
			// FIXME -- could print network address if socket had suitable method
			log((CLOG_WARN "protocol error from client \"%s\"", name.c_str()));
			CProtocolUtil::writef(output.get(), kMsgEBad);
		}
		catch (...) {
			if (connected) {
				removeConnection(name);
			}
			throw;
		}

		// flush any pending output
		output.get()->flush();
		if (connected) {
			removeConnection(name);
		}
	}
	catch (XBase& e) {
		// misc error
		log((CLOG_WARN "error communicating with client \"%s\": %s", name.c_str(), e.what()));
		// FIXME -- could print network address if socket had suitable method
	}
}

void
CServer::acceptHTTPClients(void*)
{
	log((CLOG_DEBUG1 "starting to wait for HTTP clients"));

	std::auto_ptr<IListenSocket> listen;
	try {
		// create socket listener
//		listen = std::auto_ptr<IListenSocket>(m_socketFactory->createListen());
		assign(listen, new CTCPListenSocket, IListenSocket); // FIXME

		// bind to the desired port.  keep retrying if we can't bind
		// the address immediately.
		CStopwatch timer;
		for (;;) {
			try {
				log((CLOG_DEBUG1 "binding HTTP listen socket"));
				listen->bind(m_config.getHTTPAddress());
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
			IDataSocket* socket = listen->accept();
			log((CLOG_NOTE "accepted HTTP connection"));
			CThread::testCancel();

			// handle HTTP request
			startThread(new TMethodJob<CServer>(
								this, &CServer::processHTTPRequest, socket));
		}
	}
	catch (XBase& e) {
		log((CLOG_ERR "cannot listen for HTTP clients: %s", e.what()));
		// FIXME -- quit?
		quit();
	}
}

void
CServer::processHTTPRequest(void* vsocket)
{
	IDataSocket* socket = reinterpret_cast<IDataSocket*>(vsocket);
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

void
CServer::clearGotClipboard(ClipboardID id)
{
	for (CScreenList::const_iterator index = m_screens.begin();
								index != m_screens.end(); ++index) {
		index->second->m_gotClipboard[id] = false;
	}
}

void
CServer::sendClipboard(ClipboardID id)
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

void
CServer::updatePrimaryClipboard(ClipboardID id)
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
#if WINDOWS_LIKE
#include "CMSWindowsPrimaryScreen.h"
#elif UNIX_LIKE
#include "CXWindowsPrimaryScreen.h"
#endif
void
CServer::openPrimaryScreen()
{
	assert(m_primary == NULL);

	// reset sequence number
	m_seqNum = 0;

	CString primary = m_config.getCanonicalName(m_name);
	if (primary.empty()) {
		throw XUnknownClient(m_name);
	}
	try {
		// add connection
		m_active        = addConnection(primary, NULL);
		m_primaryInfo   = m_active;

		// open screen
		log((CLOG_DEBUG1 "creating primary screen"));
#if WINDOWS_LIKE
		m_primary = new CMSWindowsPrimaryScreen;
#elif UNIX_LIKE
		m_primary = new CXWindowsPrimaryScreen;
#endif
		log((CLOG_DEBUG1 "opening primary screen"));
		m_primary->open(this);
	}
	catch (...) {
		if (m_primary != NULL) {
			removeConnection(primary);
			delete m_primary;
		}
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

void
CServer::closePrimaryScreen()
{
	assert(m_primary != NULL);

	// remove connection
	CString primary = m_config.getCanonicalName(m_name);
	removeConnection(primary);

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

CServer::CScreenInfo*
CServer::addConnection(const CString& name, IServerProtocol* protocol)
{
	log((CLOG_DEBUG "adding connection \"%s\"", name.c_str()));

	CLock lock(&m_mutex);

	// name must be in our configuration
	if (!m_config.isScreen(name)) {
		throw XUnknownClient(name);
	}

	// can only have one screen with a given name at any given time
	if (m_screens.count(name) != 0) {
		throw XDuplicateClient(name);
	}

	// save screen info
	CScreenInfo* newScreen = new CScreenInfo(name, protocol);
	m_screens.insert(std::make_pair(name, newScreen));
	log((CLOG_DEBUG "added connection \"%s\"", name.c_str()));

	return newScreen;
}

void
CServer::removeConnection(const CString& name)
{
	log((CLOG_DEBUG "removing connection \"%s\"", name.c_str()));
	CLock lock(&m_mutex);

	// find screen info
	CScreenList::iterator index = m_screens.find(name);
	assert(index != m_screens.end());

	// if this is active screen then we have to jump off of it
	if (m_active == index->second && m_active != m_primaryInfo) {
		// record new position (center of primary screen)
// FIXME -- should have separate "center" pixel reported by screen
		m_x = m_primaryInfo->m_x + (m_primaryInfo->m_w >> 1);
		m_y = m_primaryInfo->m_y + (m_primaryInfo->m_h >> 1);

		// don't notify active screen since it probably already disconnected
		log((CLOG_INFO "jump from \"%s\" to \"%s\" at %d,%d", m_active->m_name.c_str(), m_primaryInfo->m_name.c_str(), m_x, m_y));

		// cut over
		m_active = m_primaryInfo;

		// enter new screen
		m_primary->enter(m_x, m_y);
	}

	// if this screen had the cursor when the screen saver activated
	// then we can't switch back to it when the screen saver
	// deactivates.
	if (m_activeSaver == index->second) {
		m_activeSaver = NULL;
	}

	// done with screen info
	delete index->second;
	m_screens.erase(index);
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
	m_x(0),
	m_y(0),
	m_w(0),
	m_h(0),
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
