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

#include "CServer.h"
#include "CHTTPServer.h"
#include "CPrimaryClient.h"
#include "IPrimaryScreenFactory.h"
#include "CInputPacketStream.h"
#include "COutputPacketStream.h"
#include "CProtocolUtil.h"
#include "CClientProxy1_0.h"
#include "CClientProxy1_1.h"
#include "OptionTypes.h"
#include "ProtocolTypes.h"
#include "XScreen.h"
#include "XSynergy.h"
#include "CTCPListenSocket.h"
#include "IDataSocket.h"
#include "ISocketFactory.h"
#include "XSocket.h"
#include "IStreamFilterFactory.h"
#include "CLock.h"
#include "CThread.h"
#include "CTimerThread.h"
#include "XMT.h"
#include "XThread.h"
#include "CFunctionJob.h"
#include "CLog.h"
#include "CStopwatch.h"
#include "TMethodJob.h"
#include "CArch.h"

//
// CServer
//

const SInt32			CServer::s_httpMaxSimultaneousRequests = 3;

CServer::CServer(const CString& serverName) :
	m_name(serverName),
	m_error(false),
	m_bindTimeout(5.0 * 60.0),
	m_screenFactory(NULL),
	m_socketFactory(NULL),
	m_streamFilterFactory(NULL),
	m_acceptClientThread(NULL),
	m_active(NULL),
	m_primaryClient(NULL),
	m_seqNum(0),
	m_activeSaver(NULL),
	m_httpServer(NULL),
	m_httpAvailable(&m_mutex, s_httpMaxSimultaneousRequests),
	m_switchDir(kNoDirection),
	m_switchScreen(NULL),
	m_switchWaitDelay(0.0),
	m_switchWaitEngaged(false),
	m_switchTwoTapDelay(0.0),
	m_switchTwoTapEngaged(false),
	m_switchTwoTapArmed(false),
	m_switchTwoTapZone(3),
	m_status(kNotRunning)
{
	// do nothing
}

CServer::~CServer()
{
	delete m_screenFactory;
	delete m_socketFactory;
	delete m_streamFilterFactory;
}

void
CServer::open()
{
	// open the screen
	try {
		LOG((CLOG_INFO "opening screen"));
		openPrimaryScreen();
		setStatus(kNotRunning);
	}
	catch (XScreen& e) {
		// can't open screen
		setStatus(kError, e.what());
		LOG((CLOG_INFO "failed to open screen"));
		throw;
	}
	catch (XUnknownClient& e) {
		// can't open screen
		setStatus(kServerNameUnknown);
		LOG((CLOG_CRIT "unknown screen name `%s'", e.getName().c_str()));
		throw;
	}
}

void
CServer::mainLoop()
{
	// check preconditions
	{
		CLock lock(&m_mutex);
		assert(m_primaryClient != NULL);
	}

	try {
		setStatus(kNotRunning);
		LOG((CLOG_NOTE "starting server"));

		// start listening for new clients
		m_acceptClientThread = new CThread(startThread(
								new TMethodJob<CServer>(this,
									&CServer::acceptClients)));

		// start listening for HTTP requests
		if (m_config.getHTTPAddress().isValid()) {
			m_httpServer = new CHTTPServer(this);
			startThread(new TMethodJob<CServer>(this,
								&CServer::acceptHTTPClients));
		}

		// handle events
		m_primaryClient->mainLoop();

		// clean up
		LOG((CLOG_NOTE "stopping server"));

		// use a macro to write the stuff that should go into a finally
		// block so we can repeat it easily.  stroustrup's view that
		// "resource acquistion is initialization" is a better solution
		// than a finally block is parochial.  they both have their
		// place.  adding finally to C++ would've been a drop in a big
		// bucket.
#define FINALLY do {					\
		stopThreads();					\
		delete m_httpServer;			\
		m_httpServer = NULL;			\
		runStatusJobs();				\
		} while (false)
		FINALLY;
	}
	catch (XMT& e) {
		LOG((CLOG_ERR "server error: %s", e.what()));
		setStatus(kError, e.what());

		// clean up
		LOG((CLOG_NOTE "stopping server"));
		FINALLY;
		throw;
	}
	catch (XBase& e) {
		LOG((CLOG_ERR "server error: %s", e.what()));
		setStatus(kError, e.what());

		// clean up
		LOG((CLOG_NOTE "stopping server"));
		FINALLY;
	}
	catch (XThread&) {
		setStatus(kNotRunning);

		// clean up
		LOG((CLOG_NOTE "stopping server"));
		FINALLY;
		throw;
	}
	catch (...) {
		LOG((CLOG_DEBUG "unknown server error"));
		setStatus(kError);

		// clean up
		LOG((CLOG_NOTE "stopping server"));
		FINALLY;
		throw;
	}
#undef FINALLY

	// throw if there was an error
	if (m_error) {
		LOG((CLOG_DEBUG "forwarding child thread exception"));
		throw XServerRethrow();
	}
}

void
CServer::exitMainLoop()
{
	m_primaryClient->exitMainLoop();
}

void
CServer::exitMainLoopWithError()
{
	{
		CLock lock(&m_mutex);
		m_error = true;
	}
	exitMainLoop();
}

void
CServer::close()
{
	if (m_primaryClient != NULL) {
		closePrimaryScreen();
	}
	LOG((CLOG_INFO "closed screen"));
}

bool
CServer::setConfig(const CConfig& config)
{
	// refuse configuration if it doesn't include the primary screen
	{
		CLock lock(&m_mutex);
		if (m_primaryClient != NULL &&
			!config.isScreen(m_primaryClient->getName())) {
			return false;
		}
	}

	// close clients that are connected but being dropped from the
	// configuration.
	closeClients(config);

	// cut over
	CLock lock(&m_mutex);
	m_config = config;

	// process global options
	const CConfig::CScreenOptions* options = m_config.getOptions("");
	if (options != NULL && options->size() > 0) {
		for (CConfig::CScreenOptions::const_iterator index = options->begin();
									index != options->end(); ++index) {
			const OptionID id       = index->first;
			const OptionValue value = index->second;
			if (id == kOptionScreenSwitchDelay) {
				m_switchWaitDelay = 1.0e-3 * static_cast<double>(value);
				if (m_switchWaitDelay < 0.0) {
					m_switchWaitDelay = 0.0;
				}
				m_switchWaitEngaged = false;
			}
			else if (id == kOptionScreenSwitchTwoTap) {
				m_switchTwoTapDelay = 1.0e-3 * static_cast<double>(value);
				if (m_switchTwoTapDelay < 0.0) {
					m_switchTwoTapDelay = 0.0;
				}
				m_switchTwoTapEngaged = false;
			}
		}
	}

	// tell primary screen about reconfiguration
	if (m_primaryClient != NULL) {
		m_primaryClient->reconfigure(getActivePrimarySides());
	}

	// tell all (connected) clients about current options
	for (CClientList::const_iterator index = m_clients.begin();
								index != m_clients.end(); ++index) {
		IClient* client = index->second;
		sendOptions(client);
	}

	// notify of status
	runStatusJobs();

	return true;
}

void
CServer::setScreenFactory(IPrimaryScreenFactory* adopted)
{
	CLock lock(&m_mutex);
	delete m_screenFactory;
	m_screenFactory = adopted;
}

void
CServer::setSocketFactory(ISocketFactory* adopted)
{
	CLock lock(&m_mutex);
	delete m_socketFactory;
	m_socketFactory = adopted;
}

void
CServer::setStreamFilterFactory(IStreamFilterFactory* adopted)
{
	CLock lock(&m_mutex);
	delete m_streamFilterFactory;
	m_streamFilterFactory = adopted;
}

void
CServer::addStatusJob(IJob* job)
{
	m_statusJobs.addJob(job);
}

void
CServer::removeStatusJob(IJob* job)
{
	m_statusJobs.removeJob(job);
}

CString
CServer::getPrimaryScreenName() const
{
	return m_name;
}

UInt32
CServer::getNumClients() const
{
	CLock lock(&m_mutex);
	return m_clients.size();
}

void
CServer::getClients(std::vector<CString>& list) const
{
	CLock lock(&m_mutex);
	list.clear();
	for (CClientList::const_iterator index = m_clients.begin();
							index != m_clients.end(); ++index) {
		list.push_back(index->first);
	}
}

CServer::EStatus
CServer::getStatus(CString* msg) const
{
	CLock lock(&m_mutex);
	if (msg != NULL) {
		*msg = m_statusMessage;
	}
	return m_status;
}

void
CServer::getConfig(CConfig* config) const
{
	assert(config != NULL);

	CLock lock(&m_mutex);
	*config = m_config;
}

void
CServer::runStatusJobs() const
{
	m_statusJobs.runJobs();
}

void
CServer::setStatus(EStatus status, const char* msg)
{
	{
		CLock lock(&m_mutex);
		m_status = status;
		if (m_status == kError) {
			m_statusMessage = (msg == NULL) ? "Error" : msg;
		}
		else {
			m_statusMessage = (msg == NULL) ? "" : msg;
		}
	}
	runStatusJobs();
}

UInt32
CServer::getActivePrimarySides() const
{
	// note -- m_mutex must be locked on entry
	UInt32 sides = 0;
	if (!m_config.getNeighbor(getPrimaryScreenName(), kLeft).empty()) {
		sides |= kLeftMask;
	}
	if (!m_config.getNeighbor(getPrimaryScreenName(), kRight).empty()) {
		sides |= kRightMask;
	}
	if (!m_config.getNeighbor(getPrimaryScreenName(), kTop).empty()) {
		sides |= kTopMask;
	}
	if (!m_config.getNeighbor(getPrimaryScreenName(), kBottom).empty()) {
		sides |= kBottomMask;
	}
	return sides;
}

void
CServer::onError()
{
	setStatus(kError);

	// stop all running threads but don't wait too long since some
	// threads may be unable to proceed until this thread returns.
	stopThreads(3.0);

	// done with the HTTP server
	CLock lock(&m_mutex);
	delete m_httpServer;
	m_httpServer = NULL;

	// note -- we do not attempt to close down the primary screen
}

void
CServer::onInfoChanged(const CString& name, const CClientInfo& info)
{
	CLock lock(&m_mutex);

	// look up client
	CClientList::iterator index = m_clients.find(name);
	if (index == m_clients.end()) {
		throw XBadClient();
	}
	IClient* client = index->second;
	assert(client != NULL);

	// update the remote mouse coordinates
	if (client == m_active) {
		m_x = info.m_mx;
		m_y = info.m_my;
	}
	LOG((CLOG_INFO "screen \"%s\" shape=%d,%d %dx%d zone=%d pos=%d,%d", name.c_str(), info.m_x, info.m_y, info.m_w, info.m_h, info.m_zoneSize, info.m_mx, info.m_my));

	// handle resolution change to primary screen
	if (client == m_primaryClient) {
		if (client == m_active) {
			onMouseMovePrimaryNoLock(m_x, m_y);
		}
		else {
			onMouseMoveSecondaryNoLock(0, 0);
		}
	}
}

bool
CServer::onGrabClipboard(const CString& name, ClipboardID id, UInt32 seqNum)
{
	CLock lock(&m_mutex);

	// screen must be connected
	CClientList::iterator grabber = m_clients.find(name);
	if (grabber == m_clients.end()) {
		throw XBadClient();
	}

	// ignore grab if sequence number is old.  always allow primary
	// screen to grab.
	CClipboardInfo& clipboard = m_clipboards[id];
	if (name != m_primaryClient->getName() &&
		seqNum < clipboard.m_clipboardSeqNum) {
		LOG((CLOG_INFO "ignored screen \"%s\" grab of clipboard %d", name.c_str(), id));
		return false;
	}

	// mark screen as owning clipboard
	LOG((CLOG_INFO "screen \"%s\" grabbed clipboard %d from \"%s\"", name.c_str(), id, clipboard.m_clipboardOwner.c_str()));
	clipboard.m_clipboardOwner  = name;
	clipboard.m_clipboardSeqNum = seqNum;

	// clear the clipboard data (since it's not known at this point)
	if (clipboard.m_clipboard.open(0)) {
		clipboard.m_clipboard.empty();
		clipboard.m_clipboard.close();
	}
	clipboard.m_clipboardData = clipboard.m_clipboard.marshall();

	// tell all other screens to take ownership of clipboard.  tell the
	// grabber that it's clipboard isn't dirty.
	for (CClientList::iterator index = m_clients.begin();
								index != m_clients.end(); ++index) {
		IClient* client = index->second;
		if (index == grabber) {
			client->setClipboardDirty(id, false);
		}
		else {
			client->grabClipboard(id);
		}
	}

	return true;
}

void
CServer::onClipboardChanged(ClipboardID id, UInt32 seqNum, const CString& data)
{
	CLock lock(&m_mutex);
	onClipboardChangedNoLock(id, seqNum, data);
}

void
CServer::onClipboardChangedNoLock(ClipboardID id,
				UInt32 seqNum, const CString& data)
{
	CClipboardInfo& clipboard = m_clipboards[id];

	// ignore update if sequence number is old
	if (seqNum < clipboard.m_clipboardSeqNum) {
		LOG((CLOG_INFO "ignored screen \"%s\" update of clipboard %d (missequenced)", clipboard.m_clipboardOwner.c_str(), id));
		return;
	}

	// ignore if data hasn't changed
	if (data == clipboard.m_clipboardData) {
		LOG((CLOG_DEBUG "ignored screen \"%s\" update of clipboard %d (unchanged)", clipboard.m_clipboardOwner.c_str(), id));
		return;
	}

	// unmarshall into our clipboard buffer
	LOG((CLOG_INFO "screen \"%s\" updated clipboard %d", clipboard.m_clipboardOwner.c_str(), id));
	clipboard.m_clipboardData = data;
	clipboard.m_clipboard.unmarshall(clipboard.m_clipboardData, 0);

	// tell all clients except the sender that the clipboard is dirty
	CClientList::const_iterator sender =
								m_clients.find(clipboard.m_clipboardOwner);
	for (CClientList::const_iterator index = m_clients.begin();
								index != m_clients.end(); ++index) {
		IClient* client = index->second;
		client->setClipboardDirty(id, index != sender);
	}

	// send the new clipboard to the active screen
	m_active->setClipboard(id, m_clipboards[id].m_clipboardData);
}

void
CServer::onScreensaver(bool activated)
{
	LOG((CLOG_DEBUG "onScreenSaver %s", activated ? "activated" : "deactivated"));
	CLock lock(&m_mutex);

	if (activated) {
		// save current screen and position
		m_activeSaver = m_active;
		m_xSaver      = m_x;
		m_ySaver      = m_y;

		// jump to primary screen
		if (m_active != m_primaryClient) {
			switchScreen(m_primaryClient, 0, 0, true);
		}
	}
	else {
		// jump back to previous screen and position.  we must check
		// that the position is still valid since the screen may have
		// changed resolutions while the screen saver was running.
		if (m_activeSaver != NULL && m_activeSaver != m_primaryClient) {
			// check position
			IClient* screen = m_activeSaver;
			SInt32 x, y, w, h;
			screen->getShape(x, y, w, h);
			SInt32 zoneSize = screen->getJumpZoneSize();
			if (m_xSaver < x + zoneSize) {
				m_xSaver = x + zoneSize;
			}
			else if (m_xSaver >= x + w - zoneSize) {
				m_xSaver = x + w - zoneSize - 1;
			}
			if (m_ySaver < y + zoneSize) {
				m_ySaver = y + zoneSize;
			}
			else if (m_ySaver >= y + h - zoneSize) {
				m_ySaver = y + h - zoneSize - 1;
			}

			// jump
			switchScreen(screen, m_xSaver, m_ySaver, false);
		}

		// reset state
		m_activeSaver = NULL;
	}

	// send message to all clients
	for (CClientList::const_iterator index = m_clients.begin();
								index != m_clients.end(); ++index) {
		IClient* client = index->second;
		client->screensaver(activated);
	}
}

void
CServer::onOneShotTimerExpired(UInt32 id)
{
	CLock lock(&m_mutex);

	// ignore if it's an old timer or if switch wait isn't engaged anymore
	if (!m_switchWaitEngaged || id != m_switchWaitTimer) {
		return;
	}

	// ignore if mouse is locked to screen
	if (isLockedToScreenNoLock()) {
		LOG((CLOG_DEBUG1 "locked to screen"));
		clearSwitchState();
		return;
	}

	// switch screen
	switchScreen(m_switchScreen, m_switchWaitX, m_switchWaitY, false);
}

void
CServer::onKeyDown(KeyID id, KeyModifierMask mask, KeyButton button)
{
	LOG((CLOG_DEBUG1 "onKeyDown id=%d mask=0x%04x button=0x%04x", id, mask, button));
	CLock lock(&m_mutex);
	assert(m_active != NULL);

	// handle command keys
	if (onCommandKey(id, mask, true)) {
		return;
	}

	// relay
	m_active->keyDown(id, mask, button);
}

void
CServer::onKeyUp(KeyID id, KeyModifierMask mask, KeyButton button)
{
	LOG((CLOG_DEBUG1 "onKeyUp id=%d mask=0x%04x button=0x%04x", id, mask, button));
	CLock lock(&m_mutex);
	assert(m_active != NULL);

	// handle command keys
	if (onCommandKey(id, mask, false)) {
		return;
	}

	// relay
	m_active->keyUp(id, mask, button);
}

void
CServer::onKeyRepeat(KeyID id, KeyModifierMask mask,
				SInt32 count, KeyButton button)
{
	LOG((CLOG_DEBUG1 "onKeyRepeat id=%d mask=0x%04x count=%d button=0x%04x", id, mask, count, button));
	CLock lock(&m_mutex);
	assert(m_active != NULL);

	// handle command keys
	if (onCommandKey(id, mask, false)) {
		onCommandKey(id, mask, true);
		return;
	}

	// relay
	m_active->keyRepeat(id, mask, count, button);
}

void
CServer::onMouseDown(ButtonID id)
{
	LOG((CLOG_DEBUG1 "onMouseDown id=%d", id));
	CLock lock(&m_mutex);
	assert(m_active != NULL);

	// relay
	m_active->mouseDown(id);
}

void
CServer::onMouseUp(ButtonID id)
{
	LOG((CLOG_DEBUG1 "onMouseUp id=%d", id));
	CLock lock(&m_mutex);
	assert(m_active != NULL);

	// relay
	m_active->mouseUp(id);
}

bool
CServer::onMouseMovePrimary(SInt32 x, SInt32 y)
{
	LOG((CLOG_DEBUG2 "onMouseMovePrimary %d,%d", x, y));
	CLock lock(&m_mutex);
	return onMouseMovePrimaryNoLock(x, y);
}

bool
CServer::onMouseMovePrimaryNoLock(SInt32 x, SInt32 y)
{
	// mouse move on primary (server's) screen
	assert(m_primaryClient != NULL);
	assert(m_active == m_primaryClient);

	// get screen shape
	SInt32 ax, ay, aw, ah;
	m_active->getShape(ax, ay, aw, ah);
	SInt32 zoneSize = m_active->getJumpZoneSize();

	// see if we should change screens
	EDirection dir;
	if (x < ax + zoneSize) {
		x  -= zoneSize;
		dir = kLeft;
	}
	else if (x >= ax + aw - zoneSize) {
		x  += zoneSize;
		dir = kRight;
	}
	else if (y < ay + zoneSize) {
		y  -= zoneSize;
		dir = kTop;
	}
	else if (y >= ay + ah - zoneSize) {
		y  += zoneSize;
		dir = kBottom;
	}
	else {
		// still on local screen.  check if we're inside the tap region.
		SInt32 tapZone = (zoneSize < m_switchTwoTapZone) ?
							m_switchTwoTapZone : zoneSize;
		bool inTapZone = (x <  ax + tapZone ||
						  x >= ax + aw - tapZone ||
						  y <  ay + tapZone ||
						  y >= ay + ah - tapZone);

		// failed to switch
		onNoSwitch(inTapZone);
		return false;
	}

	// get jump destination
	IClient* newScreen = getNeighbor(m_active, dir, x, y);

	// should we switch or not?
	if (isSwitchOkay(newScreen, dir, x, y)) {
		// switch screen
		switchScreen(newScreen, x, y, false);
		return true;
	}
	else {
		return false;
	}
}

void
CServer::onMouseMoveSecondary(SInt32 dx, SInt32 dy)
{
	LOG((CLOG_DEBUG2 "onMouseMoveSecondary %+d,%+d", dx, dy));
	CLock lock(&m_mutex);
	onMouseMoveSecondaryNoLock(dx, dy);
}

void
CServer::onMouseMoveSecondaryNoLock(SInt32 dx, SInt32 dy)
{
	// mouse move on secondary (client's) screen
	assert(m_active != NULL);
	if (m_active == m_primaryClient) {
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

	// get screen shape
	SInt32 ax, ay, aw, ah;
	m_active->getShape(ax, ay, aw, ah);

	// find direction of neighbor and get the neighbor
	bool jump = true;
	IClient* newScreen;
	do {
		EDirection dir;
		if (m_x < ax) {
			dir = kLeft;
		}
		else if (m_x > ax + aw - 1) {
			dir = kRight;
		}
		else if (m_y < ay) {
			dir = kTop;
		}
		else if (m_y > ay + ah - 1) {
			dir = kBottom;
		}
		else {
			// we haven't left the screen
			newScreen = m_active;
			jump      = false;

			// if waiting and mouse is not on the border we're waiting
			// on then stop waiting.  also if it's not on the border
			// then arm the double tap.
			if (m_switchScreen != NULL) {
				bool clearWait;
				SInt32 zoneSize = m_primaryClient->getJumpZoneSize();
				switch (m_switchDir) {
				case kLeft:
					clearWait = (m_x >= ax + zoneSize);
					break;

				case kRight:
					clearWait = (m_x <= ax + aw - 1 - zoneSize);
					break;

				case kTop:
					clearWait = (m_y >= ay + zoneSize);
					break;

				case kBottom:
					clearWait = (m_y <= ay + ah - 1 + zoneSize);
					break;

				default:
					clearWait = false;
					break;
				}
				if (clearWait) {
					// still on local screen.  check if we're inside the
					// tap region.
					SInt32 tapZone = (zoneSize < m_switchTwoTapZone) ?
										m_switchTwoTapZone : zoneSize;
					bool inTapZone = (m_x <  ax + tapZone ||
									  m_x >= ax + aw - tapZone ||
									  m_y <  ay + tapZone ||
									  m_y >= ay + ah - tapZone);

					// failed to switch
					onNoSwitch(inTapZone);
				}
			}

			// skip rest of block
			break;
		}

		// try to switch screen.  get the neighbor.
		newScreen = getNeighbor(m_active, dir, m_x, m_y);

		// see if we should switch
		if (!isSwitchOkay(newScreen, dir, m_x, m_y)) {
			newScreen = m_active;
			jump      = false;
		}
	} while (false);

	if (jump) {
		// switch screens
		switchScreen(newScreen, m_x, m_y, false);
	}
	else {
		// same screen.  clamp mouse to edge.
		m_x = xOld + dx;
		m_y = yOld + dy;
		if (m_x < ax) {
			m_x = ax;
			LOG((CLOG_DEBUG2 "clamp to left of \"%s\"", m_active->getName().c_str()));
		}
		else if (m_x > ax + aw - 1) {
			m_x = ax + aw - 1;
			LOG((CLOG_DEBUG2 "clamp to right of \"%s\"", m_active->getName().c_str()));
		}
		if (m_y < ay) {
			m_y = ay;
			LOG((CLOG_DEBUG2 "clamp to top of \"%s\"", m_active->getName().c_str()));
		}
		else if (m_y > ay + ah - 1) {
			m_y = ay + ah - 1;
			LOG((CLOG_DEBUG2 "clamp to bottom of \"%s\"", m_active->getName().c_str()));
		}

		// warp cursor if it moved.
		if (m_x != xOld || m_y != yOld) {
			LOG((CLOG_DEBUG2 "move on %s to %d,%d", m_active->getName().c_str(), m_x, m_y));
			m_active->mouseMove(m_x, m_y);
		}
	}
}

void
CServer::onMouseWheel(SInt32 delta)
{
	LOG((CLOG_DEBUG1 "onMouseWheel %+d", delta));
	CLock lock(&m_mutex);
	assert(m_active != NULL);

	// relay
	m_active->mouseWheel(delta);
}

bool
CServer::onCommandKey(KeyID /*id*/, KeyModifierMask /*mask*/, bool /*down*/)
{
	return false;
}

bool
CServer::isLockedToScreenNoLock() const
{
	// locked if scroll-lock is toggled on
	if ((m_primaryClient->getToggleMask() & KeyModifierScrollLock) != 0) {
		LOG((CLOG_DEBUG "locked by ScrollLock"));
		return true;
	}

	// locked if primary says we're locked
	if (m_primaryClient->isLockedToScreen()) {
		return true;
	}

	// not locked
	return false;
}

void
CServer::switchScreen(IClient* dst, SInt32 x, SInt32 y, bool forScreensaver)
{
	// note -- must be locked on entry

	assert(dst != NULL);
#ifndef NDEBUG
	{
		SInt32 dx, dy, dw, dh;
		dst->getShape(dx, dy, dw, dh);
		assert(x >= dx && y >= dy && x < dx + dw && y < dy + dh);
	}
#endif
	assert(m_active != NULL);

	LOG((CLOG_INFO "switch from \"%s\" to \"%s\" at %d,%d", m_active->getName().c_str(), dst->getName().c_str(), x, y));

	// stop waiting to switch
	clearSwitchState();

	// record new position
	m_x = x;
	m_y = y;

	// wrapping means leaving the active screen and entering it again.
	// since that's a waste of time we skip that and just warp the
	// mouse.
	if (m_active != dst) {
		// leave active screen
		if (!m_active->leave()) {
			// cannot leave screen
			LOG((CLOG_WARN "can't leave screen"));
			return;
		}

		// update the primary client's clipboards if we're leaving the
		// primary screen.
		if (m_active == m_primaryClient) {
			for (ClipboardID id = 0; id < kClipboardEnd; ++id) {
				CClipboardInfo& clipboard = m_clipboards[id];
				if (clipboard.m_clipboardOwner == m_primaryClient->getName()) {
					CString clipboardData;
					m_primaryClient->getClipboard(id, clipboardData);
					onClipboardChangedNoLock(id,
								clipboard.m_clipboardSeqNum, clipboardData);
				}
			}
		}

		// cut over
		m_active = dst;

		// increment enter sequence number
		++m_seqNum;

		// enter new screen
		m_active->enter(x, y, m_seqNum,
								m_primaryClient->getToggleMask(),
								forScreensaver);

		// send the clipboard data to new active screen
		for (ClipboardID id = 0; id < kClipboardEnd; ++id) {
			m_active->setClipboard(id, m_clipboards[id].m_clipboardData);
		}
	}
	else {
		m_active->mouseMove(x, y);
	}
}

IClient*
CServer::getNeighbor(IClient* src, EDirection dir) const
{
	// note -- must be locked on entry

	assert(src != NULL);

	// get source screen name
	CString srcName = src->getName();
	assert(!srcName.empty());
	LOG((CLOG_DEBUG2 "find neighbor on %s of \"%s\"", CConfig::dirName(dir), srcName.c_str()));

	// get first neighbor.  if it's the source then the source jumps
	// to itself and we return the source.
	CString dstName(m_config.getNeighbor(srcName, dir));
	if (dstName == srcName) {
		LOG((CLOG_DEBUG2 "\"%s\" is on %s of \"%s\"", dstName.c_str(), CConfig::dirName(dir), srcName.c_str()));
		return src;
	}

	// keep checking
	for (;;) {
		// if nothing in that direction then return NULL. if the
		// destination is the source then we can make no more
		// progress in this direction.  since we haven't found a
		// connected neighbor we return NULL.
		if (dstName.empty() || dstName == srcName) {
			LOG((CLOG_DEBUG2 "no neighbor on %s of \"%s\"", CConfig::dirName(dir), srcName.c_str()));
			return NULL;
		}

		// look up neighbor cell.  if the screen is connected and
		// ready then we can stop.
		CClientList::const_iterator index = m_clients.find(dstName);
		if (index != m_clients.end()) {
			LOG((CLOG_DEBUG2 "\"%s\" is on %s of \"%s\"", dstName.c_str(), CConfig::dirName(dir), srcName.c_str()));
			return index->second;
		}

		// skip over unconnected screen
		LOG((CLOG_DEBUG2 "ignored \"%s\" on %s of \"%s\"", dstName.c_str(), CConfig::dirName(dir), srcName.c_str()));
		srcName = dstName;

		// look up name of neighbor of skipped screen
		dstName = m_config.getNeighbor(srcName, dir);
	}
}

IClient*
CServer::getNeighbor(IClient* src,
				EDirection srcSide, SInt32& x, SInt32& y) const
{
	// note -- must be locked on entry

	assert(src != NULL);

	// get the first neighbor
	IClient* dst = getNeighbor(src, srcSide);
	if (dst == NULL) {
		return NULL;
	}

	// get the source screen's size (needed for kRight and kBottom)
	SInt32 sx, sy, sw, sh;
	SInt32 dx, dy, dw, dh;
	IClient* lastGoodScreen = src;
	lastGoodScreen->getShape(sx, sy, sw, sh);
	lastGoodScreen->getShape(dx, dy, dw, dh);

	// find destination screen, adjusting x or y (but not both).  the
	// searches are done in a sort of canonical screen space where
	// the upper-left corner is 0,0 for each screen.  we adjust from
	// actual to canonical position on entry to and from canonical to
	// actual on exit from the search.
	switch (srcSide) {
	case kLeft:
		x -= dx;
		while (dst != NULL) {
			lastGoodScreen = dst;
			lastGoodScreen->getShape(dx, dy, dw, dh);
			x += dw;
			if (x >= 0) {
				break;
			}
			LOG((CLOG_DEBUG2 "skipping over screen %s", dst->getName().c_str()));
			dst = getNeighbor(lastGoodScreen, srcSide);
		}
		assert(lastGoodScreen != NULL);
		x += dx;
		break;

	case kRight:
		x -= dx;
		while (dst != NULL) {
			x -= dw;
			lastGoodScreen = dst;
			lastGoodScreen->getShape(dx, dy, dw, dh);
			if (x < dw) {
				break;
			}
			LOG((CLOG_DEBUG2 "skipping over screen %s", dst->getName().c_str()));
			dst = getNeighbor(lastGoodScreen, srcSide);
		}
		assert(lastGoodScreen != NULL);
		x += dx;
		break;

	case kTop:
		y -= dy;
		while (dst != NULL) {
			lastGoodScreen = dst;
			lastGoodScreen->getShape(dx, dy, dw, dh);
			y += dh;
			if (y >= 0) {
				break;
			}
			LOG((CLOG_DEBUG2 "skipping over screen %s", dst->getName().c_str()));
			dst = getNeighbor(lastGoodScreen, srcSide);
		}
		assert(lastGoodScreen != NULL);
		y += dy;
		break;

	case kBottom:
		y -= dy;
		while (dst != NULL) {
			y -= dh;
			lastGoodScreen = dst;
			lastGoodScreen->getShape(dx, dy, dw, dh);
			if (y < sh) {
				break;
			}
			LOG((CLOG_DEBUG2 "skipping over screen %s", dst->getName().c_str()));
			dst = getNeighbor(lastGoodScreen, srcSide);
		}
		assert(lastGoodScreen != NULL);
		y += dy;
		break;
	}

	// save destination screen
	assert(lastGoodScreen != NULL);
	dst = lastGoodScreen;

	// if entering primary screen then be sure to move in far enough
	// to avoid the jump zone.  if entering a side that doesn't have
	// a neighbor (i.e. an asymmetrical side) then we don't need to
	// move inwards because that side can't provoke a jump.
	if (dst == m_primaryClient) {
		const CString dstName(dst->getName());
		switch (srcSide) {
		case kLeft:
			if (!m_config.getNeighbor(dstName, kRight).empty() &&
				x > dx + dw - 1 - dst->getJumpZoneSize())
				x = dx + dw - 1 - dst->getJumpZoneSize();
			break;

		case kRight:
			if (!m_config.getNeighbor(dstName, kLeft).empty() &&
				x < dx + dst->getJumpZoneSize())
				x = dx + dst->getJumpZoneSize();
			break;

		case kTop:
			if (!m_config.getNeighbor(dstName, kBottom).empty() &&
				y > dy + dh - 1 - dst->getJumpZoneSize())
				y = dy + dh - 1 - dst->getJumpZoneSize();
			break;

		case kBottom:
			if (!m_config.getNeighbor(dstName, kTop).empty() &&
				y < dy + dst->getJumpZoneSize())
				y = dy + dst->getJumpZoneSize();
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
	case kLeft:
	case kRight:
		y -= sy;
		if (y < 0) {
			y = 0;
		}
		else if (y >= sh) {
			y = dh - 1;
		}
		else {
			y = static_cast<SInt32>(0.5 + y *
								static_cast<double>(dh - 1) / (sh - 1));
		}
		y += dy;
		break;

	case kTop:
	case kBottom:
		x -= sx;
		if (x < 0) {
			x = 0;
		}
		else if (x >= sw) {
			x = dw - 1;
		}
		else {
			x = static_cast<SInt32>(0.5 + x *
								static_cast<double>(dw - 1) / (sw - 1));
		}
		x += dx;
		break;
	}

	return dst;
}

bool
CServer::isSwitchOkay(IClient* newScreen, EDirection dir, SInt32 x, SInt32 y)
{
	LOG((CLOG_DEBUG1 "try to leave \"%s\" on %s", m_active->getName().c_str(), CConfig::dirName(dir)));

	// is there a neighbor?
	if (newScreen == NULL) {
		// there's no neighbor.  we don't want to switch and we don't
		// want to try to switch later.
		LOG((CLOG_DEBUG1 "no neighbor %s", CConfig::dirName(dir)));
		clearSwitchState();
		return false;
	}

	// should we switch or not?
	bool preventSwitch = false;
	bool allowSwitch   = false;

	// note if the switch direction has changed.  save the new
	// direction and screen if so.
	bool isNewDirection  = (dir != m_switchDir);
	if (isNewDirection || m_switchScreen == NULL) {
		m_switchDir    = dir;
		m_switchScreen = newScreen;
	}

	// is this a double tap and do we care?
	if (!allowSwitch && m_switchTwoTapDelay > 0.0) {
		if (isNewDirection || !m_switchTwoTapEngaged) {
			// tapping a different or new edge.  prepare for second tap.
			preventSwitch         = true;
			m_switchTwoTapEngaged = true;
			m_switchTwoTapArmed   = false;
			m_switchTwoTapTimer.reset();
			LOG((CLOG_DEBUG1 "waiting for second tap"));
		}
		else {
			// second tap if we were armed.  if soon enough then switch.
			if (m_switchTwoTapArmed &&
				m_switchTwoTapTimer.getTime() <= m_switchTwoTapDelay) {
				allowSwitch = true;
			}
			else {
				// not fast enough.  reset the clock.
				preventSwitch         = true;
				m_switchTwoTapEngaged = true;
				m_switchTwoTapArmed   = false;
				m_switchTwoTapTimer.reset();
				LOG((CLOG_DEBUG1 "waiting for second tap"));
			}
		}
	}

	// if waiting before a switch then prepare to switch later
	if (!allowSwitch && m_switchWaitDelay > 0.0) {
		if (isNewDirection || !m_switchWaitEngaged) {
			m_switchWaitEngaged = true;
			m_switchWaitX       = x;
			m_switchWaitY       = y;
			m_switchWaitTimer   = m_primaryClient->addOneShotTimer(
													m_switchWaitDelay);
			LOG((CLOG_DEBUG1 "waiting to switch"));
		}
		preventSwitch = true;
	}

	// ignore if mouse is locked to screen
	if (!preventSwitch && isLockedToScreenNoLock()) {
		LOG((CLOG_DEBUG1 "locked to screen"));
		preventSwitch = true;

		// don't try to switch later.  it's possible that we might
		// not be locked to the screen when the wait delay expires
		// and could switch then but we'll base the decision on
		// when the user first attempts the switch.  this also
		// ensures that all switch tests are using the same
		clearSwitchState();
	}

	return !preventSwitch;
}

void
CServer::onNoSwitch(bool inTapZone)
{
	if (m_switchTwoTapEngaged) {
		if (m_switchTwoTapTimer.getTime() > m_switchTwoTapDelay) {
			// second tap took too long.  disengage.
			m_switchTwoTapEngaged = false;
			m_switchTwoTapArmed   = false;
		}
		else if (!inTapZone) {
			// we've moved away from the edge and there's still
			// time to get back for a double tap.
			m_switchTwoTapArmed = true;
		}
	}

	// once the mouse moves away from the edge we no longer want to
	// switch after a delay.
	m_switchWaitEngaged = false;
}

void
CServer::clearSwitchState()
{
	if (m_switchScreen != NULL) {
		m_switchDir           = kNoDirection;
		m_switchScreen        = NULL;
		m_switchWaitEngaged   = false;
		m_switchTwoTapEngaged = false;
	}
}

void
CServer::closeClients(const CConfig& config)
{
	CThreadList threads;
	{
		CLock lock(&m_mutex);

		// get the set of clients that are connected but are being
		// dropped from the configuration (or who's canonical name
		// is changing) and tell them to disconnect.  note that
		// since m_clientThreads doesn't include a thread for the
		// primary client we will not close it.
		for (CClientThreadList::iterator
								index  = m_clientThreads.begin();
								index != m_clientThreads.end(); ) {
			const CString& name = index->first;
			if (!config.isCanonicalName(name)) {
				// lookup IClient with name
				CClientList::const_iterator index2 = m_clients.find(name);
				assert(index2 != m_clients.end());

				// save the thread and remove it from m_clientThreads
				threads.push_back(index->second);
				m_clientThreads.erase(index++);

				// close that client
				assert(index2->second != m_primaryClient);
				index2->second->close();

				// don't switch to it if we planned to
				if (index2->second == m_switchScreen) {
					clearSwitchState();
				}
			}
			else {
				++index;
			}
		}
	}

	// wait a moment to allow each client to close its connection
	// before we close it (to avoid having our socket enter TIME_WAIT).
	if (threads.size() > 0) {
		ARCH->sleep(1.0);
	}

	// cancel the old client threads
	for (CThreadList::iterator index = threads.begin();
								index != threads.end(); ++index) {
		index->cancel();
	}

	// wait for old client threads to terminate.  we must not hold
	// the lock while we do this so those threads can finish any
	// calls to this object.
	for (CThreadList::iterator index = threads.begin();
								index != threads.end(); ++index) {
		index->wait();
	}

	// clean up thread list
	reapThreads();
}

CThread
CServer::startThread(IJob* job)
{
	CLock lock(&m_mutex);

	// reap completed threads
	doReapThreads(m_threads);

	// add new thread to list
	CThread thread(job);
	m_threads.push_back(thread);
	LOG((CLOG_DEBUG1 "started thread 0x%08x", thread.getID()));
	return thread;
}

void
CServer::stopThreads(double timeout)
{
	LOG((CLOG_DEBUG1 "stopping threads"));

	// cancel the accept client thread to prevent more clients from
	// connecting while we're shutting down.
	CThread* acceptClientThread;
	{
		CLock lock(&m_mutex);
		acceptClientThread   = m_acceptClientThread;
		m_acceptClientThread = NULL;
	}
	if (acceptClientThread != NULL) {
		acceptClientThread->cancel();
		acceptClientThread->wait(timeout);
		delete acceptClientThread;
	}

	// close all clients (except the primary)
	{
		CConfig emptyConfig;
		closeClients(emptyConfig);
	}

	// swap thread list so nobody can mess with it
	CThreadList threads;
	{
		CLock lock(&m_mutex);
		threads.swap(m_threads);
	}

	// cancel every thread
	for (CThreadList::iterator index = threads.begin();
								index != threads.end(); ++index) {
		index->cancel();
	}

	// now wait for the threads
	CStopwatch timer(true);
	while (threads.size() > 0 && (timeout < 0.0 || timer.getTime() < timeout)) {
		doReapThreads(threads);
		ARCH->sleep(0.01);
	}

	// delete remaining threads
	for (CThreadList::iterator index = threads.begin();
								index != threads.end(); ++index) {
		LOG((CLOG_DEBUG1 "reaped running thread 0x%08x", index->getID()));
	}

	LOG((CLOG_DEBUG1 "stopped threads"));
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
		if (index->wait(0.0)) {
			// thread terminated
			LOG((CLOG_DEBUG1 "reaped thread 0x%08x", index->getID()));
			index = threads.erase(index);
		}
		else {
			// thread is running
			++index;
		}
	}
}

void
CServer::acceptClients(void*)
{
	LOG((CLOG_DEBUG1 "starting to wait for clients"));

	IListenSocket* listen = NULL;
	try {
		// create socket listener
		if (m_socketFactory != NULL) {
			listen = m_socketFactory->createListen();
		}
		assert(listen != NULL);

		// bind to the desired port.  keep retrying if we can't bind
		// the address immediately.
		CStopwatch timer;
		for (;;) {
			try {
				LOG((CLOG_DEBUG1 "binding listen socket"));
				listen->bind(m_config.getSynergyAddress());
				break;
			}
			catch (XSocketAddressInUse& e) {
				setStatus(kError, e.what());
				LOG((CLOG_WARN "bind failed: %s", e.what()));

				// give up if we've waited too long
				if (timer.getTime() >= m_bindTimeout) {
					LOG((CLOG_ERR "waited too long to bind, giving up"));
					throw;
				}

				// wait a bit before retrying
				ARCH->sleep(5.0);
			}
		}

		// accept connections and begin processing them
		setStatus(kRunning);
		LOG((CLOG_DEBUG1 "waiting for client connections"));
		for (;;) {
			// accept connection
			CThread::testCancel();
			IDataSocket* socket = listen->accept();
			LOG((CLOG_NOTE "accepted client connection"));
			CThread::testCancel();

			// start handshake thread
			startThread(new TMethodJob<CServer>(
								this, &CServer::runClient, socket));
		}
	}
	catch (XBase& e) {
		setStatus(kError, e.what());
		LOG((CLOG_ERR "cannot listen for clients: %s", e.what()));
		delete listen;
		exitMainLoopWithError();
	}
	catch (...) {
		setStatus(kNotRunning);
		delete listen;
		throw;
	}
}

void
CServer::runClient(void* vsocket)
{
	// get the socket pointer from the argument
	assert(vsocket != NULL);
	IDataSocket* socket = reinterpret_cast<IDataSocket*>(vsocket);

	// create proxy
	CClientProxy* proxy = NULL;
	try {
		proxy = handshakeClient(socket);
		if (proxy == NULL) {
			delete socket;
			return;
		}
	}
	catch (...) {
		delete socket;
		throw;
	}

	// add the connection
	try {
		addConnection(proxy);

		// save this client's thread
		CLock lock(&m_mutex);
		m_clientThreads.insert(std::make_pair(proxy->getName(),
								CThread::getCurrentThread()));

		// send configuration options
		sendOptions(proxy);
	}
	catch (XDuplicateClient& e) {
		// client has duplicate name
		LOG((CLOG_WARN "a client with name \"%s\" is already connected", e.getName().c_str()));
		try {
			CProtocolUtil::writef(proxy->getOutputStream(), kMsgEBusy);
		}
		catch (XSocket&) {
			// ignore
		}
		delete proxy;
		delete socket;
		return;
	}
	catch (XUnknownClient& e) {
		// client has unknown name
		LOG((CLOG_WARN "a client with name \"%s\" is not in the map", e.getName().c_str()));
		try {
			CProtocolUtil::writef(proxy->getOutputStream(), kMsgEUnknown);
		}
		catch (XSocket&) {
			// ignore
		}
		delete proxy;
		delete socket;
		return;
	}
	catch (...) {
		delete proxy;
		delete socket;
		throw;
	}

	// activate screen saver on new client if active on the primary screen
	{
		CLock lock(&m_mutex);
		if (m_activeSaver != NULL) {
			proxy->screensaver(true);
		}
	}

	// handle client messages
	try {
		LOG((CLOG_NOTE "client \"%s\" has connected", proxy->getName().c_str()));
		proxy->mainLoop();
	}
	catch (XBadClient&) {
		// client not behaving
		LOG((CLOG_WARN "protocol error from client \"%s\"", proxy->getName().c_str()));
		try {
			CProtocolUtil::writef(proxy->getOutputStream(), kMsgEBad);
		}
		catch (XSocket&) {
			// ignore.  client probably aborted the connection.
		}
	}
	catch (XBase& e) {
		// misc error
		LOG((CLOG_WARN "error communicating with client \"%s\": %s", proxy->getName().c_str(), e.what()));
	}
	catch (...) {
		// mainLoop() was probably cancelled
		removeConnection(proxy->getName());
		delete socket;
		throw;
	}

	// clean up
	removeConnection(proxy->getName());
	delete socket;
}

CClientProxy*
CServer::handshakeClient(IDataSocket* socket)
{
	LOG((CLOG_DEBUG1 "negotiating with new client"));

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

	CClientProxy* proxy = NULL;
	CString name("<unknown>");
	try {
		// give the client a limited time to complete the handshake
		CTimerThread timer(30.0);

		// say hello
		LOG((CLOG_DEBUG1 "saying hello"));
		CProtocolUtil::writef(output, kMsgHello,
										kProtocolMajorVersion,
										kProtocolMinorVersion);
		output->flush();

		// wait for the reply
		LOG((CLOG_DEBUG1 "waiting for hello reply"));
		UInt32 n = input->getSize();

		// limit the maximum length of the hello
		if (n > kMaxHelloLength) {
			throw XBadClient();
		}

		// get and parse the reply to hello
		SInt16 major, minor;
		try {
			LOG((CLOG_DEBUG1 "parsing hello reply"));
			CProtocolUtil::readf(input, kMsgHelloBack,
										&major, &minor, &name);
		}
		catch (XIO&) {
			throw XBadClient();
		}

		// disallow invalid version numbers
		if (major <= 0 || minor < 0) {
			throw XIncompatibleClient(major, minor);
		}

		// convert name to canonical form (if any)
		if (m_config.isScreen(name)) {
			name = m_config.getCanonicalName(name);
		}

		// create client proxy for highest version supported by the client
		LOG((CLOG_DEBUG1 "creating proxy for client \"%s\" version %d.%d", name.c_str(), major, minor));
		if (major == 1) {
			switch (minor) {
			case 0:
				proxy = new CClientProxy1_0(this, name, input, output);
				break;

			case 1:
				proxy = new CClientProxy1_1(this, name, input, output);
				break;
			}
		}

		// hangup (with error) if version isn't supported
		if (proxy == NULL) {
			throw XIncompatibleClient(major, minor);
		}

		// negotiate
		// FIXME

		// ask and wait for the client's info
		LOG((CLOG_DEBUG1 "waiting for info for client \"%s\"", name.c_str()));
		proxy->open();

		return proxy;
	}
	catch (XIncompatibleClient& e) {
		// client is incompatible
		LOG((CLOG_WARN "client \"%s\" has incompatible version %d.%d)", name.c_str(), e.getMajor(), e.getMinor()));
		try {
			CProtocolUtil::writef(output, kMsgEIncompatible,
							kProtocolMajorVersion, kProtocolMinorVersion);
		}
		catch (XSocket&) {
			// ignore
		}
	}
	catch (XBadClient&) {
		// client not behaving
		LOG((CLOG_WARN "protocol error from client \"%s\"", name.c_str()));
		try {
			CProtocolUtil::writef(output, kMsgEBad);
		}
		catch (XSocket&) {
			// ignore.  client probably aborted the connection.
		}
	}
	catch (XBase& e) {
		// misc error
		LOG((CLOG_WARN "error communicating with client \"%s\": %s", name.c_str(), e.what()));
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

void
CServer::acceptHTTPClients(void*)
{
	LOG((CLOG_DEBUG1 "starting to wait for HTTP clients"));

	IListenSocket* listen = NULL;
	try {
		// create socket listener
		listen = new CTCPListenSocket;

		// bind to the desired port.  keep retrying if we can't bind
		// the address immediately.
		CStopwatch timer;
		for (;;) {
			try {
				LOG((CLOG_DEBUG1 "binding HTTP listen socket"));
				listen->bind(m_config.getHTTPAddress());
				break;
			}
			catch (XSocketBind& e) {
				LOG((CLOG_DEBUG1 "bind HTTP failed: %s", e.what()));

				// give up if we've waited too long
				if (timer.getTime() >= m_bindTimeout) {
					LOG((CLOG_DEBUG1 "waited too long to bind HTTP, giving up"));
					throw;
				}

				// wait a bit before retrying
				ARCH->sleep(5.0);
			}
		}

		// accept connections and begin processing them
		LOG((CLOG_DEBUG1 "waiting for HTTP connections"));
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
			LOG((CLOG_NOTE "accepted HTTP connection"));
			CThread::testCancel();

			// handle HTTP request
			startThread(new TMethodJob<CServer>(
								this, &CServer::processHTTPRequest, socket));
		}

		// clean up
		delete listen;
	}
	catch (XBase& e) {
		LOG((CLOG_ERR "cannot listen for HTTP clients: %s", e.what()));
		delete listen;
		exitMainLoopWithError();
	}
	catch (...) {
		delete listen;
		throw;
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
		ARCH->sleep(3.0);

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
CServer::sendOptions(IClient* client) const
{
	// note -- must be locked on entry

	COptionsList optionsList;

	// look up options for client
	const CConfig::CScreenOptions* options =
						m_config.getOptions(client->getName());
	if (options != NULL && options->size() > 0) {
		// convert options to a more convenient form for sending
		optionsList.reserve(2 * options->size());
		for (CConfig::CScreenOptions::const_iterator index = options->begin();
									index != options->end(); ++index) {
			optionsList.push_back(index->first);
			optionsList.push_back(static_cast<UInt32>(index->second));
		}
	}

	// look up global options
	options = m_config.getOptions("");
	if (options != NULL && options->size() > 0) {
		// convert options to a more convenient form for sending
		optionsList.reserve(optionsList.size() + 2 * options->size());
		for (CConfig::CScreenOptions::const_iterator index = options->begin();
									index != options->end(); ++index) {
			optionsList.push_back(index->first);
			optionsList.push_back(static_cast<UInt32>(index->second));
		}
	}

	// send the options
	client->setOptions(optionsList);
}

void
CServer::openPrimaryScreen()
{
	assert(m_primaryClient == NULL);

	// reset sequence number
	m_seqNum = 0;

	// canonicalize the primary screen name
	CString primaryName = m_config.getCanonicalName(getPrimaryScreenName());
	if (primaryName.empty()) {
		throw XUnknownClient(getPrimaryScreenName());
	}

	// clear clipboards
	for (ClipboardID id = 0; id < kClipboardEnd; ++id) {
		CClipboardInfo& clipboard   = m_clipboards[id];
		clipboard.m_clipboardOwner  = primaryName;
		clipboard.m_clipboardSeqNum = m_seqNum;
		if (clipboard.m_clipboard.open(0)) {
			clipboard.m_clipboard.empty();
			clipboard.m_clipboard.close();
		}
		clipboard.m_clipboardData   = clipboard.m_clipboard.marshall();
	}

	try {
		// create the primary client
		m_primaryClient = new CPrimaryClient(m_screenFactory,
								this, this, primaryName);

		// add connection
		addConnection(m_primaryClient);
		m_active = m_primaryClient;

		// open the screen
		LOG((CLOG_DEBUG1 "opening primary screen"));
		m_primaryClient->open();

		// tell it about the active sides
		m_primaryClient->reconfigure(getActivePrimarySides());

		// tell primary client about its options
		sendOptions(m_primaryClient);
	}
	catch (...) {
		// if m_active is NULL then we haven't added the connection
		// for the primary client so we don't try to remove it.
		if (m_active != NULL) {
			removeConnection(primaryName);
		}
		else {
			delete m_primaryClient;
		}
		m_active        = NULL;
		m_primaryClient = NULL;
		throw;
	}
}

void
CServer::closePrimaryScreen()
{
	assert(m_primaryClient != NULL);

	// close the primary screen
	try {
		LOG((CLOG_DEBUG1 "closing primary screen"));
		m_primaryClient->close();
	}
	catch (...) {
		// ignore
	}

	// remove connection
	removeConnection(m_primaryClient->getName());
	m_primaryClient = NULL;
}

void
CServer::addConnection(IClient* client)
{
	assert(client != NULL);

	LOG((CLOG_DEBUG "adding connection \"%s\"", client->getName().c_str()));

	{
		CLock lock(&m_mutex);

		// name must be in our configuration
		if (!m_config.isScreen(client->getName())) {
			throw XUnknownClient(client->getName());
		}

		// can only have one screen with a given name at any given time
		if (m_clients.count(client->getName()) != 0) {
			throw XDuplicateClient(client->getName());
		}

		// save screen info
		m_clients.insert(std::make_pair(client->getName(), client));
		LOG((CLOG_DEBUG "added connection \"%s\"", client->getName().c_str()));
	}
	runStatusJobs();
}

void
CServer::removeConnection(const CString& name)
{
	LOG((CLOG_DEBUG "removing connection \"%s\"", name.c_str()));
	bool updateStatus;
	{
		CLock lock(&m_mutex);

		// find client
		CClientList::iterator index = m_clients.find(name);
		assert(index != m_clients.end());

		// if this is active screen then we have to jump off of it
		IClient* active = (m_activeSaver != NULL) ? m_activeSaver : m_active;
		if (active == index->second && active != m_primaryClient) {
			// record new position (center of primary screen)
			m_primaryClient->getCursorCenter(m_x, m_y);

			// stop waiting to switch if we were
			if (active == m_switchScreen) {
				clearSwitchState();
			}

			// don't notify active screen since it probably already
			// disconnected.
			LOG((CLOG_INFO "jump from \"%s\" to \"%s\" at %d,%d", active->getName().c_str(), m_primaryClient->getName().c_str(), m_x, m_y));

			// cut over
			m_active = m_primaryClient;

			// enter new screen (unless we already have because of the
			// screen saver)
			if (m_activeSaver == NULL) {
				m_primaryClient->enter(m_x, m_y, m_seqNum,
									m_primaryClient->getToggleMask(), false);
			}
		}

		// if this screen had the cursor when the screen saver activated
		// then we can't switch back to it when the screen saver
		// deactivates.
		if (m_activeSaver == index->second) {
			m_activeSaver = NULL;
		}

		// done with client
		delete index->second;
		m_clients.erase(index);

		// remove any thread for this client
		m_clientThreads.erase(name);

		updateStatus = (m_clients.size() <= 1);
	}

	if (updateStatus) {
		runStatusJobs();
	}
}


//
// CServer::CClipboardInfo
//

CString
CServer::XServerRethrow::getWhat() const throw()
{
	return format("XServerRethrow", "child thread failed");
}


//
// CServer::CClipboardInfo
//

CServer::CClipboardInfo::CClipboardInfo() :
	m_clipboard(),
	m_clipboardData(),
	m_clipboardOwner(),
	m_clipboardSeqNum(0)
{
	// do nothing
}
