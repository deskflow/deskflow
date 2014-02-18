/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Bolton Software Ltd.
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "CServer.h"
#include "CClientProxy.h"
#include "CClientProxyUnknown.h"
#include "CPrimaryClient.h"
#include "IPlatformScreen.h"
#include "OptionTypes.h"
#include "ProtocolTypes.h"
#include "XScreen.h"
#include "XSynergy.h"
#include "IDataSocket.h"
#include "IListenSocket.h"
#include "XSocket.h"
#include "IEventQueue.h"
#include "CLog.h"
#include "TMethodEventJob.h"
#include "CArch.h"
#include "CKeyState.h"
#include "CScreen.h"
#include "CThread.h"
#include "TMethodJob.h"
#include "CFileChunker.h"
#include <cstring>
#include <cstdlib>
#include <sstream>
#include <fstream>
#include <stdexcept>

//
// CServer
//

CServer::CServer(CConfig& config, CPrimaryClient* primaryClient, CScreen* screen, IEventQueue* events, bool enableDragDrop) :
	m_events(events),
	m_mock(false),
	m_primaryClient(primaryClient),
	m_active(primaryClient),
	m_seqNum(0),
	m_xDelta(0),
	m_yDelta(0),
	m_xDelta2(0),
	m_yDelta2(0),
	m_config(&config),
	m_inputFilter(config.getInputFilter()),
	m_activeSaver(NULL),
	m_switchDir(kNoDirection),
	m_switchScreen(NULL),
	m_switchWaitDelay(0.0),
	m_switchWaitTimer(NULL),
	m_switchTwoTapDelay(0.0),
	m_switchTwoTapEngaged(false),
	m_switchTwoTapArmed(false),
	m_switchTwoTapZone(3),
	m_switchNeedsShift(false),
	m_switchNeedsControl(false),
	m_switchNeedsAlt(false),
	m_relativeMoves(false),
	m_keyboardBroadcasting(false),
	m_lockedToScreen(false),
	m_screen(screen),
	m_sendFileThread(NULL),
	m_writeToDropDirThread(NULL),
	m_ignoreFileTransfer(false),
	m_enableDragDrop(enableDragDrop)
{
	// must have a primary client and it must have a canonical name
	assert(m_primaryClient != NULL);
	assert(config.isScreen(primaryClient->getName()));
	assert(m_screen != NULL);

	CString primaryName = getName(primaryClient);

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

	// install event handlers
	m_events->adoptHandler(CEvent::kTimer, this,
							new TMethodEventJob<CServer>(this,
								&CServer::handleSwitchWaitTimeout));
	m_events->adoptHandler(m_events->forIKeyState().keyDown(),
							m_inputFilter,
							new TMethodEventJob<CServer>(this,
								&CServer::handleKeyDownEvent));
	m_events->adoptHandler(m_events->forIKeyState().keyUp(),
							m_inputFilter,
							new TMethodEventJob<CServer>(this,
								&CServer::handleKeyUpEvent));
	m_events->adoptHandler(m_events->forIKeyState().keyRepeat(),
							m_inputFilter,
							new TMethodEventJob<CServer>(this,
								&CServer::handleKeyRepeatEvent));
	m_events->adoptHandler(m_events->forIPrimaryScreen().buttonDown(),
							m_inputFilter,
							new TMethodEventJob<CServer>(this,
								&CServer::handleButtonDownEvent));
	m_events->adoptHandler(m_events->forIPrimaryScreen().buttonUp(),
							m_inputFilter,
							new TMethodEventJob<CServer>(this,
								&CServer::handleButtonUpEvent));
	m_events->adoptHandler(m_events->forIPrimaryScreen().motionOnPrimary(),
							m_primaryClient->getEventTarget(),
							new TMethodEventJob<CServer>(this,
								&CServer::handleMotionPrimaryEvent));
	m_events->adoptHandler(m_events->forIPrimaryScreen().motionOnSecondary(),
							m_primaryClient->getEventTarget(),
							new TMethodEventJob<CServer>(this,
								&CServer::handleMotionSecondaryEvent));
	m_events->adoptHandler(m_events->forIPrimaryScreen().wheel(),
							m_primaryClient->getEventTarget(),
							new TMethodEventJob<CServer>(this,
								&CServer::handleWheelEvent));
	m_events->adoptHandler(m_events->forIPrimaryScreen().screensaverActivated(),
							m_primaryClient->getEventTarget(),
							new TMethodEventJob<CServer>(this,
								&CServer::handleScreensaverActivatedEvent));
	m_events->adoptHandler(m_events->forIPrimaryScreen().screensaverDeactivated(),
							m_primaryClient->getEventTarget(),
							new TMethodEventJob<CServer>(this,
								&CServer::handleScreensaverDeactivatedEvent));
	m_events->adoptHandler(m_events->forCServer().switchToScreen(),
							m_inputFilter,
							new TMethodEventJob<CServer>(this,
								&CServer::handleSwitchToScreenEvent));
	m_events->adoptHandler(m_events->forCServer().switchInDirection(),
							m_inputFilter,
							new TMethodEventJob<CServer>(this,
								&CServer::handleSwitchInDirectionEvent));
	m_events->adoptHandler(m_events->forCServer().keyboardBroadcast(),
							m_inputFilter,
							new TMethodEventJob<CServer>(this,
								&CServer::handleKeyboardBroadcastEvent));
	m_events->adoptHandler(m_events->forCServer().lockCursorToScreen(),
							m_inputFilter,
							new TMethodEventJob<CServer>(this,
								&CServer::handleLockCursorToScreenEvent));
	m_events->adoptHandler(m_events->forIPrimaryScreen().fakeInputBegin(),
							m_inputFilter,
							new TMethodEventJob<CServer>(this,
								&CServer::handleFakeInputBeginEvent));
	m_events->adoptHandler(m_events->forIPrimaryScreen().fakeInputEnd(),
							m_inputFilter,
							new TMethodEventJob<CServer>(this,
								&CServer::handleFakeInputEndEvent));

	if (m_enableDragDrop) {
		m_events->adoptHandler(m_events->forIScreen().fileChunkSending(),
								this,
								new TMethodEventJob<CServer>(this,
									&CServer::handleFileChunkSendingEvent));
		m_events->adoptHandler(m_events->forIScreen().fileRecieveCompleted(),
								this,
								new TMethodEventJob<CServer>(this,
									&CServer::handleFileRecieveCompletedEvent));
	}

	// add connection
	addClient(m_primaryClient);

	// set initial configuration
	setConfig(config);

	// enable primary client
	m_primaryClient->enable();
	m_inputFilter->setPrimaryClient(m_primaryClient);

	// Determine if scroll lock is already set. If so, lock the cursor to the primary screen
	int keyValue = m_primaryClient->getToggleMask();
	if (m_primaryClient->getToggleMask() & KeyModifierScrollLock) {
		LOG((CLOG_DEBUG "scroll lock on initially. locked to screen"));
		m_lockedToScreen = true;
	}

}

CServer::~CServer()
{
	if (m_mock) {
		return;
	}

	// remove event handlers and timers
	m_events->removeHandler(m_events->forIKeyState().keyDown(),
							m_inputFilter);
	m_events->removeHandler(m_events->forIKeyState().keyUp(),
							m_inputFilter);
	m_events->removeHandler(m_events->forIKeyState().keyRepeat(),
							m_inputFilter);
	m_events->removeHandler(m_events->forIPrimaryScreen().buttonDown(),
							m_inputFilter);
	m_events->removeHandler(m_events->forIPrimaryScreen().buttonUp(),
							m_inputFilter);
	m_events->removeHandler(m_events->forIPrimaryScreen().motionOnPrimary(),
							m_primaryClient->getEventTarget());
	m_events->removeHandler(m_events->forIPrimaryScreen().motionOnSecondary(),
							m_primaryClient->getEventTarget());
	m_events->removeHandler(m_events->forIPrimaryScreen().wheel(),
							m_primaryClient->getEventTarget());
	m_events->removeHandler(m_events->forIPrimaryScreen().screensaverActivated(),
							m_primaryClient->getEventTarget());
	m_events->removeHandler(m_events->forIPrimaryScreen().screensaverDeactivated(),
							m_primaryClient->getEventTarget());
	m_events->removeHandler(m_events->forIPrimaryScreen().fakeInputBegin(),
							m_inputFilter);
	m_events->removeHandler(m_events->forIPrimaryScreen().fakeInputEnd(),
							m_inputFilter);
	m_events->removeHandler(CEvent::kTimer, this);
	stopSwitch();

	// force immediate disconnection of secondary clients
	disconnect();
	for (COldClients::iterator index = m_oldClients.begin();
							index != m_oldClients.begin(); ++index) {
		CBaseClientProxy* client = index->first;
		m_events->deleteTimer(index->second);
		m_events->removeHandler(CEvent::kTimer, client);
		m_events->removeHandler(m_events->forCClientProxy().disconnected(), client);
		delete client;
	}

	// remove input filter
	m_inputFilter->setPrimaryClient(NULL);

	// disable and disconnect primary client
	m_primaryClient->disable();
	removeClient(m_primaryClient);
}

bool
CServer::setConfig(const CConfig& config)
{
	// refuse configuration if it doesn't include the primary screen
	if (!config.isScreen(m_primaryClient->getName())) {
		return false;
	}

	// close clients that are connected but being dropped from the
	// configuration.
	closeClients(config);

	// cut over
	processOptions();

	// add ScrollLock as a hotkey to lock to the screen.  this was a
	// built-in feature in earlier releases and is now supported via
	// the user configurable hotkey mechanism.  if the user has already
	// registered ScrollLock for something else then that will win but
	// we will unfortunately generate a warning.  if the user has
	// configured a CLockCursorToScreenAction then we don't add
	// ScrollLock as a hotkey.
	if (!m_config->hasLockToScreenAction()) {
		IPlatformScreen::CKeyInfo* key =
			IPlatformScreen::CKeyInfo::alloc(kKeyScrollLock, 0, 0, 0);
		CInputFilter::CRule rule(new CInputFilter::CKeystrokeCondition(m_events, key));
		rule.adoptAction(new CInputFilter::CLockCursorToScreenAction(m_events), true);
		m_inputFilter->addFilterRule(rule);
	}

	// tell primary screen about reconfiguration
	m_primaryClient->reconfigure(getActivePrimarySides());

	// tell all (connected) clients about current options
	for (CClientList::const_iterator index = m_clients.begin();
								index != m_clients.end(); ++index) {
		CBaseClientProxy* client = index->second;
		sendOptions(client);
	}

	return true;
}

void
CServer::adoptClient(CBaseClientProxy* client)
{
	assert(client != NULL);

	// watch for client disconnection
	m_events->adoptHandler(m_events->forCClientProxy().disconnected(), client,
							new TMethodEventJob<CServer>(this,
								&CServer::handleClientDisconnected, client));

	// name must be in our configuration
	if (!m_config->isScreen(client->getName())) {
		LOG((CLOG_WARN "unrecognised client name \"%s\", check server config", client->getName().c_str()));
		closeClient(client, kMsgEUnknown);
		return;
	}

	// add client to client list
	if (!addClient(client)) {
		// can only have one screen with a given name at any given time
		LOG((CLOG_WARN "a client with name \"%s\" is already connected", getName(client).c_str()));
		closeClient(client, kMsgEBusy);
		return;
	}
	LOG((CLOG_NOTE "client \"%s\" has connected", getName(client).c_str()));

	// send configuration options to client
	sendOptions(client);

	// activate screen saver on new client if active on the primary screen
	if (m_activeSaver != NULL) {
		client->screensaver(true);
	}

	// send notification
	CServer::CScreenConnectedInfo* info =
		new CServer::CScreenConnectedInfo(getName(client));
	m_events->addEvent(CEvent(m_events->forCServer().connected(),
								m_primaryClient->getEventTarget(), info));
}

void
CServer::disconnect()
{
	// close all secondary clients
	if (m_clients.size() > 1 || !m_oldClients.empty()) {
		CConfig emptyConfig(m_events);
		closeClients(emptyConfig);
	}
	else {
		m_events->addEvent(CEvent(m_events->forCServer().disconnected(), this));
	}
}

UInt32
CServer::getNumClients() const
{
	return (SInt32)m_clients.size();
}

void
CServer::getClients(std::vector<CString>& list) const
{
	list.clear();
	for (CClientList::const_iterator index = m_clients.begin();
							index != m_clients.end(); ++index) {
		list.push_back(index->first);
	}
}

CString
CServer::getName(const CBaseClientProxy* client) const
{
	CString name = m_config->getCanonicalName(client->getName());
	if (name.empty()) {
		name = client->getName();
	}
	return name;
}

UInt32
CServer::getActivePrimarySides() const
{
	UInt32 sides = 0;
	if (!isLockedToScreenServer()) {
		if (hasAnyNeighbor(m_primaryClient, kLeft)) {
			sides |= kLeftMask;
		}
		if (hasAnyNeighbor(m_primaryClient, kRight)) {
			sides |= kRightMask;
		}
		if (hasAnyNeighbor(m_primaryClient, kTop)) {
			sides |= kTopMask;
		}
		if (hasAnyNeighbor(m_primaryClient, kBottom)) {
			sides |= kBottomMask;
		}
	}
	return sides;
}

bool
CServer::isLockedToScreenServer() const
{
	// locked if scroll-lock is toggled on
	return m_lockedToScreen;
}

bool
CServer::isLockedToScreen() const
{
	// locked if we say we're locked
	if (isLockedToScreenServer()) {
		LOG((CLOG_DEBUG "locked to screen"));
		return true;
	}

	// locked if primary says we're locked
	if (m_primaryClient->isLockedToScreen()) {
		return true;
	}

	// not locked
	return false;
}

SInt32
CServer::getJumpZoneSize(CBaseClientProxy* client) const
{
	if (client == m_primaryClient) {
		return m_primaryClient->getJumpZoneSize();
	}
	else {
		return 0;
	}
}

void
CServer::switchScreen(CBaseClientProxy* dst,
				SInt32 x, SInt32 y, bool forScreensaver)
{
	assert(dst != NULL);
#ifndef NDEBUG
	{
		SInt32 dx, dy, dw, dh;
		dst->getShape(dx, dy, dw, dh);
		assert(x >= dx && y >= dy && x < dx + dw && y < dy + dh);
	}
#endif
	assert(m_active != NULL);

	LOG((CLOG_INFO "switch from \"%s\" to \"%s\" at %d,%d", getName(m_active).c_str(), getName(dst).c_str(), x, y));

	// stop waiting to switch
	stopSwitch();

	// record new position
	m_x       = x;
	m_y       = y;
	m_xDelta  = 0;
	m_yDelta  = 0;
	m_xDelta2 = 0;
	m_yDelta2 = 0;

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
				if (clipboard.m_clipboardOwner == getName(m_primaryClient)) {
					onClipboardChanged(m_primaryClient,
						id, clipboard.m_clipboardSeqNum);
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
			m_active->setClipboard(id, &m_clipboards[id].m_clipboard);
		}

		CServer::CSwitchToScreenInfo* info =
			CServer::CSwitchToScreenInfo::alloc(m_active->getName());
		m_events->addEvent(CEvent(m_events->forCServer().screenSwitched(), this, info));
	}
	else {
		m_active->mouseMove(x, y);
	}
}

void
CServer::jumpToScreen(CBaseClientProxy* newScreen)
{
	assert(newScreen != NULL);

	// record the current cursor position on the active screen
	m_active->setJumpCursorPos(m_x, m_y);

	// get the last cursor position on the target screen
	SInt32 x, y;
	newScreen->getJumpCursorPos(x, y);
	
	switchScreen(newScreen, x, y, false);
}

float
CServer::mapToFraction(CBaseClientProxy* client,
				EDirection dir, SInt32 x, SInt32 y) const
{
	SInt32 sx, sy, sw, sh;
	client->getShape(sx, sy, sw, sh);
	switch (dir) {
	case kLeft:
	case kRight:
		return static_cast<float>(y - sy + 0.5f) / static_cast<float>(sh);

	case kTop:
	case kBottom:
		return static_cast<float>(x - sx + 0.5f) / static_cast<float>(sw);

	case kNoDirection:
		assert(0 && "bad direction");
		break;
	}
	return 0.0f;
}

void
CServer::mapToPixel(CBaseClientProxy* client,
				EDirection dir, float f, SInt32& x, SInt32& y) const
{
	SInt32 sx, sy, sw, sh;
	client->getShape(sx, sy, sw, sh);
	switch (dir) {
	case kLeft:
	case kRight:
		y = static_cast<SInt32>(f * sh) + sy;
		break;

	case kTop:
	case kBottom:
		x = static_cast<SInt32>(f * sw) + sx;
		break;

	case kNoDirection:
		assert(0 && "bad direction");
		break;
	}
}

bool
CServer::hasAnyNeighbor(CBaseClientProxy* client, EDirection dir) const
{
	assert(client != NULL);

	return m_config->hasNeighbor(getName(client), dir);
}

CBaseClientProxy*
CServer::getNeighbor(CBaseClientProxy* src,
				EDirection dir, SInt32& x, SInt32& y) const
{
	// note -- must be locked on entry

	assert(src != NULL);

	// get source screen name
	CString srcName = getName(src);
	assert(!srcName.empty());
	LOG((CLOG_DEBUG2 "find neighbor on %s of \"%s\"", CConfig::dirName(dir), srcName.c_str()));

	// convert position to fraction
	float t = mapToFraction(src, dir, x, y);

	// search for the closest neighbor that exists in direction dir
	float tTmp;
	for (;;) {
		CString dstName(m_config->getNeighbor(srcName, dir, t, &tTmp));

		// if nothing in that direction then return NULL. if the
		// destination is the source then we can make no more
		// progress in this direction.  since we haven't found a
		// connected neighbor we return NULL.
		if (dstName.empty()) {
			LOG((CLOG_DEBUG2 "no neighbor on %s of \"%s\"", CConfig::dirName(dir), srcName.c_str()));
			return NULL;
		}

		// look up neighbor cell.  if the screen is connected and
		// ready then we can stop.
		CClientList::const_iterator index = m_clients.find(dstName);
		if (index != m_clients.end()) {
			LOG((CLOG_DEBUG2 "\"%s\" is on %s of \"%s\" at %f", dstName.c_str(), CConfig::dirName(dir), srcName.c_str(), t));
			mapToPixel(index->second, dir, tTmp, x, y);
			return index->second;
		}

		// skip over unconnected screen
		LOG((CLOG_DEBUG2 "ignored \"%s\" on %s of \"%s\"", dstName.c_str(), CConfig::dirName(dir), srcName.c_str()));
		srcName = dstName;

		// use position on skipped screen
		t = tTmp;
	}
}

CBaseClientProxy*
CServer::mapToNeighbor(CBaseClientProxy* src,
				EDirection srcSide, SInt32& x, SInt32& y) const
{
	// note -- must be locked on entry

	assert(src != NULL);

	// get the first neighbor
	CBaseClientProxy* dst = getNeighbor(src, srcSide, x, y);
	if (dst == NULL) {
		return NULL;
	}

	// get the source screen's size
	SInt32 dx, dy, dw, dh;
	CBaseClientProxy* lastGoodScreen = src;
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
			LOG((CLOG_DEBUG2 "skipping over screen %s", getName(dst).c_str()));
			dst = getNeighbor(lastGoodScreen, srcSide, x, y);
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
			LOG((CLOG_DEBUG2 "skipping over screen %s", getName(dst).c_str()));
			dst = getNeighbor(lastGoodScreen, srcSide, x, y);
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
			LOG((CLOG_DEBUG2 "skipping over screen %s", getName(dst).c_str()));
			dst = getNeighbor(lastGoodScreen, srcSide, x, y);
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
			if (y < dh) {
				break;
			}
			LOG((CLOG_DEBUG2 "skipping over screen %s", getName(dst).c_str()));
			dst = getNeighbor(lastGoodScreen, srcSide, x, y);
		}
		assert(lastGoodScreen != NULL);
		y += dy;
		break;

	case kNoDirection:
		assert(0 && "bad direction");
		return NULL;
	}

	// save destination screen
	assert(lastGoodScreen != NULL);
	dst = lastGoodScreen;

	// if entering primary screen then be sure to move in far enough
	// to avoid the jump zone.  if entering a side that doesn't have
	// a neighbor (i.e. an asymmetrical side) then we don't need to
	// move inwards because that side can't provoke a jump.
	avoidJumpZone(dst, srcSide, x, y);

	return dst;
}

void
CServer::avoidJumpZone(CBaseClientProxy* dst,
				EDirection dir, SInt32& x, SInt32& y) const
{
	// we only need to avoid jump zones on the primary screen
	if (dst != m_primaryClient) {
		return;
	}

	const CString dstName(getName(dst));
	SInt32 dx, dy, dw, dh;
	dst->getShape(dx, dy, dw, dh);
	float t = mapToFraction(dst, dir, x, y);
	SInt32 z = getJumpZoneSize(dst);

	// move in far enough to avoid the jump zone.  if entering a side
	// that doesn't have a neighbor (i.e. an asymmetrical side) then we
	// don't need to move inwards because that side can't provoke a jump.
	switch (dir) {
	case kLeft:
		if (!m_config->getNeighbor(dstName, kRight, t, NULL).empty() &&
			x > dx + dw - 1 - z)
			x = dx + dw - 1 - z;
		break;

	case kRight:
		if (!m_config->getNeighbor(dstName, kLeft, t, NULL).empty() &&
			x < dx + z)
			x = dx + z;
		break;

	case kTop:
		if (!m_config->getNeighbor(dstName, kBottom, t, NULL).empty() &&
			y > dy + dh - 1 - z)
			y = dy + dh - 1 - z;
		break;

	case kBottom:
		if (!m_config->getNeighbor(dstName, kTop, t, NULL).empty() &&
			y < dy + z)
			y = dy + z;
		break;

	case kNoDirection:
		assert(0 && "bad direction");
	}
}

bool
CServer::isSwitchOkay(CBaseClientProxy* newScreen,
				EDirection dir, SInt32 x, SInt32 y,
				SInt32 xActive, SInt32 yActive)
{
	LOG((CLOG_DEBUG1 "try to leave \"%s\" on %s", getName(m_active).c_str(), CConfig::dirName(dir)));

	// is there a neighbor?
	if (newScreen == NULL) {
		// there's no neighbor.  we don't want to switch and we don't
		// want to try to switch later.
		LOG((CLOG_DEBUG1 "no neighbor %s", CConfig::dirName(dir)));
		stopSwitch();
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
		if (isNewDirection ||
			!isSwitchTwoTapStarted() || !shouldSwitchTwoTap()) {
			// tapping a different or new edge or second tap not
			// fast enough.  prepare for second tap.
			preventSwitch = true;
			startSwitchTwoTap();
		}
		else {
			// got second tap
			allowSwitch = true;
		}
	}

	// if waiting before a switch then prepare to switch later
	if (!allowSwitch && m_switchWaitDelay > 0.0) {
		if (isNewDirection || !isSwitchWaitStarted()) {
			startSwitchWait(x, y);
		}
		preventSwitch = true;
	}

	// are we in a locked corner?  first check if screen has the option set
	// and, if not, check the global options.
	const CConfig::CScreenOptions* options =
						m_config->getOptions(getName(m_active));
	if (options == NULL || options->count(kOptionScreenSwitchCorners) == 0) {
		options = m_config->getOptions("");
	}
	if (options != NULL && options->count(kOptionScreenSwitchCorners) > 0) {
		// get corner mask and size
		CConfig::CScreenOptions::const_iterator i =
			options->find(kOptionScreenSwitchCorners);
		UInt32 corners = static_cast<UInt32>(i->second);
		i = options->find(kOptionScreenSwitchCornerSize);
		SInt32 size = 0;
		if (i != options->end()) {
			size = i->second;
		}

		// see if we're in a locked corner
		if ((getCorner(m_active, xActive, yActive, size) & corners) != 0) {
			// yep, no switching
			LOG((CLOG_DEBUG1 "locked in corner"));
			preventSwitch = true;
			stopSwitch();
		}
	}

	// ignore if mouse is locked to screen and don't try to switch later
	if (!preventSwitch && isLockedToScreen()) {
		LOG((CLOG_DEBUG1 "locked to screen"));
		preventSwitch = true;
		stopSwitch();
	}

	// check for optional needed modifiers
	KeyModifierMask mods = this->m_primaryClient->getToggleMask( );

	if (!preventSwitch && (
			(this->m_switchNeedsShift && ((mods & KeyModifierShift) != KeyModifierShift)) ||
        	(this->m_switchNeedsControl && ((mods & KeyModifierControl) != KeyModifierControl)) ||
			(this->m_switchNeedsAlt && ((mods & KeyModifierAlt) != KeyModifierAlt))
		)) {
		LOG((CLOG_DEBUG1 "need modifiers to switch"));
		preventSwitch = true;
		stopSwitch();
	} 
	
	return !preventSwitch;
}

void
CServer::noSwitch(SInt32 x, SInt32 y)
{
	armSwitchTwoTap(x, y);
	stopSwitchWait();
}

void
CServer::stopSwitch()
{
	if (m_switchScreen != NULL) {
		m_switchScreen = NULL;
		m_switchDir    = kNoDirection;
		stopSwitchTwoTap();
		stopSwitchWait();
	}
}

void
CServer::startSwitchTwoTap()
{
	m_switchTwoTapEngaged = true;
	m_switchTwoTapArmed   = false;
	m_switchTwoTapTimer.reset();
	LOG((CLOG_DEBUG1 "waiting for second tap"));
}

void
CServer::armSwitchTwoTap(SInt32 x, SInt32 y)
{
	if (m_switchTwoTapEngaged) {
		if (m_switchTwoTapTimer.getTime() > m_switchTwoTapDelay) {
			// second tap took too long.  disengage.
			stopSwitchTwoTap();
		}
		else if (!m_switchTwoTapArmed) {
			// still time for a double tap.  see if we left the tap
			// zone and, if so, arm the two tap.
			SInt32 ax, ay, aw, ah;
			m_active->getShape(ax, ay, aw, ah);
			SInt32 tapZone = m_primaryClient->getJumpZoneSize();
			if (tapZone < m_switchTwoTapZone) {
				tapZone = m_switchTwoTapZone;
			}
			if (x >= ax + tapZone && x < ax + aw - tapZone &&
				y >= ay + tapZone && y < ay + ah - tapZone) {
				// win32 can generate bogus mouse events that appear to
				// move in the opposite direction that the mouse actually
				// moved.  try to ignore that crap here.
				switch (m_switchDir) {
				case kLeft:
					m_switchTwoTapArmed = (m_xDelta > 0 && m_xDelta2 > 0);
					break;

				case kRight:
					m_switchTwoTapArmed = (m_xDelta < 0 && m_xDelta2 < 0);
					break;

				case kTop:
					m_switchTwoTapArmed = (m_yDelta > 0 && m_yDelta2 > 0);
					break;

				case kBottom:
					m_switchTwoTapArmed = (m_yDelta < 0 && m_yDelta2 < 0);
					break;

				default:
					break;
				}
			}
		}
	}
}

void
CServer::stopSwitchTwoTap()
{
	m_switchTwoTapEngaged = false;
	m_switchTwoTapArmed   = false;
}

bool
CServer::isSwitchTwoTapStarted() const
{
	return m_switchTwoTapEngaged;
}

bool
CServer::shouldSwitchTwoTap() const
{
	// this is the second tap if two-tap is armed and this tap
	// came fast enough
	return (m_switchTwoTapArmed &&
			m_switchTwoTapTimer.getTime() <= m_switchTwoTapDelay);
}

void
CServer::startSwitchWait(SInt32 x, SInt32 y)
{
	stopSwitchWait();
	m_switchWaitX     = x;
	m_switchWaitY     = y;
	m_switchWaitTimer = m_events->newOneShotTimer(m_switchWaitDelay, this);
	LOG((CLOG_DEBUG1 "waiting to switch"));
}

void
CServer::stopSwitchWait()
{
	if (m_switchWaitTimer != NULL) {
		m_events->deleteTimer(m_switchWaitTimer);
		m_switchWaitTimer = NULL;
	}
}

bool
CServer::isSwitchWaitStarted() const
{
	return (m_switchWaitTimer != NULL);
}

UInt32
CServer::getCorner(CBaseClientProxy* client,
				SInt32 x, SInt32 y, SInt32 size) const
{
	assert(client != NULL);

	// get client screen shape
	SInt32 ax, ay, aw, ah;
	client->getShape(ax, ay, aw, ah);

	// check for x,y on the left or right
	SInt32 xSide;
	if (x <= ax) {
		xSide = -1;
	}
	else if (x >= ax + aw - 1) {
		xSide = 1;
	}
	else {
		xSide = 0;
	}

	// check for x,y on the top or bottom
	SInt32 ySide;
	if (y <= ay) {
		ySide = -1;
	}
	else if (y >= ay + ah - 1) {
		ySide = 1;
	}
	else {
		ySide = 0;
	}

	// if against the left or right then check if y is within size
	if (xSide != 0) {
		if (y < ay + size) {
			return (xSide < 0) ? kTopLeftMask : kTopRightMask;
		}
		else if (y >= ay + ah - size) {
			return (xSide < 0) ? kBottomLeftMask : kBottomRightMask;
		}
	}

	// if against the left or right then check if y is within size
	if (ySide != 0) {
		if (x < ax + size) {
			return (ySide < 0) ? kTopLeftMask : kBottomLeftMask;
		}
		else if (x >= ax + aw - size) {
			return (ySide < 0) ? kTopRightMask : kBottomRightMask;
		}
	}

	return kNoCornerMask;
}

void
CServer::stopRelativeMoves()
{
	if (m_relativeMoves && m_active != m_primaryClient) {
		// warp to the center of the active client so we know where we are
		SInt32 ax, ay, aw, ah;
		m_active->getShape(ax, ay, aw, ah);
		m_x       = ax + (aw >> 1);
		m_y       = ay + (ah >> 1);
		m_xDelta  = 0;
		m_yDelta  = 0;
		m_xDelta2 = 0;
		m_yDelta2 = 0;
		LOG((CLOG_DEBUG2 "synchronize move on %s by %d,%d", getName(m_active).c_str(), m_x, m_y));
		m_active->mouseMove(m_x, m_y);
	}
}

void
CServer::sendOptions(CBaseClientProxy* client) const
{
	COptionsList optionsList;

	// look up options for client
	const CConfig::CScreenOptions* options =
						m_config->getOptions(getName(client));
	if (options != NULL) {
		// convert options to a more convenient form for sending
		optionsList.reserve(2 * options->size());
		for (CConfig::CScreenOptions::const_iterator index = options->begin();
									index != options->end(); ++index) {
			optionsList.push_back(index->first);
			optionsList.push_back(static_cast<UInt32>(index->second));
		}
	}

	// look up global options
	options = m_config->getOptions("");
	if (options != NULL) {
		// convert options to a more convenient form for sending
		optionsList.reserve(optionsList.size() + 2 * options->size());
		for (CConfig::CScreenOptions::const_iterator index = options->begin();
									index != options->end(); ++index) {
			optionsList.push_back(index->first);
			optionsList.push_back(static_cast<UInt32>(index->second));
		}
	}

	// send the options
	client->resetOptions();
	client->setOptions(optionsList);
}

void
CServer::processOptions()
{
	const CConfig::CScreenOptions* options = m_config->getOptions("");
	if (options == NULL) {
		return;
	}

	m_switchNeedsShift = false;		// it seems if i don't add these
	m_switchNeedsControl = false;	// lines, the 'reload config' option
	m_switchNeedsAlt = false;		// doesnt' work correct.

	bool newRelativeMoves = m_relativeMoves;
	for (CConfig::CScreenOptions::const_iterator index = options->begin();
								index != options->end(); ++index) {
		const OptionID id       = index->first;
		const OptionValue value = index->second;
		if (id == kOptionScreenSwitchDelay) {
			m_switchWaitDelay = 1.0e-3 * static_cast<double>(value);
			if (m_switchWaitDelay < 0.0) {
				m_switchWaitDelay = 0.0;
			}
			stopSwitchWait();
		}
		else if (id == kOptionScreenSwitchTwoTap) {
			m_switchTwoTapDelay = 1.0e-3 * static_cast<double>(value);
			if (m_switchTwoTapDelay < 0.0) {
				m_switchTwoTapDelay = 0.0;
			}
			stopSwitchTwoTap();
		}
		else if (id == kOptionScreenSwitchNeedsControl) {
			m_switchNeedsControl = (value != 0);
		}
		else if (id == kOptionScreenSwitchNeedsShift) {
			m_switchNeedsShift = (value != 0);
		}
		else if (id == kOptionScreenSwitchNeedsAlt) {
			m_switchNeedsAlt = (value != 0);
		}
		else if (id == kOptionRelativeMouseMoves) {
			newRelativeMoves = (value != 0);
		}
	}
	if (m_relativeMoves && !newRelativeMoves) {
		stopRelativeMoves();
	}
	m_relativeMoves = newRelativeMoves;
}

void
CServer::handleShapeChanged(const CEvent&, void* vclient)
{
	// ignore events from unknown clients
	CBaseClientProxy* client = reinterpret_cast<CBaseClientProxy*>(vclient);
	if (m_clientSet.count(client) == 0) {
		return;
	}

	LOG((CLOG_DEBUG "screen \"%s\" shape changed", getName(client).c_str()));

	// update jump coordinate
	SInt32 x, y;
	client->getCursorPos(x, y);
	client->setJumpCursorPos(x, y);

	// update the mouse coordinates
	if (client == m_active) {
		m_x = x;
		m_y = y;
	}

	// handle resolution change to primary screen
	if (client == m_primaryClient) {
		if (client == m_active) {
			onMouseMovePrimary(m_x, m_y);
		}
		else {
			onMouseMoveSecondary(0, 0);
		}
	}
}

void
CServer::handleClipboardGrabbed(const CEvent& event, void* vclient)
{
	// ignore events from unknown clients
	CBaseClientProxy* grabber = reinterpret_cast<CBaseClientProxy*>(vclient);
	if (m_clientSet.count(grabber) == 0) {
		return;
	}
	const IScreen::CClipboardInfo* info =
		reinterpret_cast<const IScreen::CClipboardInfo*>(event.getData());

	// ignore grab if sequence number is old.  always allow primary
	// screen to grab.
	CClipboardInfo& clipboard = m_clipboards[info->m_id];
	if (grabber != m_primaryClient &&
		info->m_sequenceNumber < clipboard.m_clipboardSeqNum) {
		LOG((CLOG_INFO "ignored screen \"%s\" grab of clipboard %d", getName(grabber).c_str(), info->m_id));
		return;
	}

	// mark screen as owning clipboard
	LOG((CLOG_INFO "screen \"%s\" grabbed clipboard %d from \"%s\"", getName(grabber).c_str(), info->m_id, clipboard.m_clipboardOwner.c_str()));
	clipboard.m_clipboardOwner  = getName(grabber);
	clipboard.m_clipboardSeqNum = info->m_sequenceNumber;

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
		CBaseClientProxy* client = index->second;
		if (client == grabber) {
			client->setClipboardDirty(info->m_id, false);
		}
		else {
			client->grabClipboard(info->m_id);
		}
	}
}

void
CServer::handleClipboardChanged(const CEvent& event, void* vclient)
{
	// ignore events from unknown clients
	CBaseClientProxy* sender = reinterpret_cast<CBaseClientProxy*>(vclient);
	if (m_clientSet.count(sender) == 0) {
		return;
	}
	const IScreen::CClipboardInfo* info =
		reinterpret_cast<const IScreen::CClipboardInfo*>(event.getData());
	onClipboardChanged(sender, info->m_id, info->m_sequenceNumber);
}

void
CServer::handleKeyDownEvent(const CEvent& event, void*)
{
	IPlatformScreen::CKeyInfo* info =
		reinterpret_cast<IPlatformScreen::CKeyInfo*>(event.getData());
	onKeyDown(info->m_key, info->m_mask, info->m_button, info->m_screens);
}

void
CServer::handleKeyUpEvent(const CEvent& event, void*)
{
	IPlatformScreen::CKeyInfo* info =
		 reinterpret_cast<IPlatformScreen::CKeyInfo*>(event.getData());
	onKeyUp(info->m_key, info->m_mask, info->m_button, info->m_screens);
}

void
CServer::handleKeyRepeatEvent(const CEvent& event, void*)
{
	IPlatformScreen::CKeyInfo* info =
		reinterpret_cast<IPlatformScreen::CKeyInfo*>(event.getData());
	onKeyRepeat(info->m_key, info->m_mask, info->m_count, info->m_button);
}

void
CServer::handleButtonDownEvent(const CEvent& event, void*)
{
	IPlatformScreen::CButtonInfo* info =
		reinterpret_cast<IPlatformScreen::CButtonInfo*>(event.getData());
	onMouseDown(info->m_button);
}

void
CServer::handleButtonUpEvent(const CEvent& event, void*)
{
	IPlatformScreen::CButtonInfo* info =
		reinterpret_cast<IPlatformScreen::CButtonInfo*>(event.getData());
	onMouseUp(info->m_button);
}

void
CServer::handleMotionPrimaryEvent(const CEvent& event, void*)
{
	IPlatformScreen::CMotionInfo* info =
		reinterpret_cast<IPlatformScreen::CMotionInfo*>(event.getData());
	onMouseMovePrimary(info->m_x, info->m_y);
}

void
CServer::handleMotionSecondaryEvent(const CEvent& event, void*)
{
	IPlatformScreen::CMotionInfo* info =
		reinterpret_cast<IPlatformScreen::CMotionInfo*>(event.getData());
	onMouseMoveSecondary(info->m_x, info->m_y);
}

void
CServer::handleWheelEvent(const CEvent& event, void*)
{
	IPlatformScreen::CWheelInfo* info =
		reinterpret_cast<IPlatformScreen::CWheelInfo*>(event.getData());
	onMouseWheel(info->m_xDelta, info->m_yDelta);
}

void
CServer::handleScreensaverActivatedEvent(const CEvent&, void*)
{
	onScreensaver(true);
}

void
CServer::handleScreensaverDeactivatedEvent(const CEvent&, void*)
{
	onScreensaver(false);
}

void
CServer::handleSwitchWaitTimeout(const CEvent&, void*)
{
	// ignore if mouse is locked to screen
	if (isLockedToScreen()) {
		LOG((CLOG_DEBUG1 "locked to screen"));
		stopSwitch();
		return;
	}

	// switch screen
	switchScreen(m_switchScreen, m_switchWaitX, m_switchWaitY, false);
}

void
CServer::handleClientDisconnected(const CEvent&, void* vclient)
{
	// client has disconnected.  it might be an old client or an
	// active client.  we don't care so just handle it both ways.
	CBaseClientProxy* client = reinterpret_cast<CBaseClientProxy*>(vclient);
	removeActiveClient(client);
	removeOldClient(client);
	delete client;
}

void
CServer::handleClientCloseTimeout(const CEvent&, void* vclient)
{
	// client took too long to disconnect.  just dump it.
	CBaseClientProxy* client = reinterpret_cast<CBaseClientProxy*>(vclient);
	LOG((CLOG_NOTE "forced disconnection of client \"%s\"", getName(client).c_str()));
	removeOldClient(client);
	delete client;
}

void
CServer::handleSwitchToScreenEvent(const CEvent& event, void*)
{
	CSwitchToScreenInfo* info = 
		reinterpret_cast<CSwitchToScreenInfo*>(event.getData());

	CClientList::const_iterator index = m_clients.find(info->m_screen);
	if (index == m_clients.end()) {
		LOG((CLOG_DEBUG1 "screen \"%s\" not active", info->m_screen));
	}
	else {
		jumpToScreen(index->second);
	}
}

void
CServer::handleSwitchInDirectionEvent(const CEvent& event, void*)
{
	CSwitchInDirectionInfo* info = 
		reinterpret_cast<CSwitchInDirectionInfo*>(event.getData());

	// jump to screen in chosen direction from center of this screen
	SInt32 x = m_x, y = m_y;
	CBaseClientProxy* newScreen =
		getNeighbor(m_active, info->m_direction, x, y);
	if (newScreen == NULL) {
		LOG((CLOG_DEBUG1 "no neighbor %s", CConfig::dirName(info->m_direction)));
	}
	else {
		jumpToScreen(newScreen);
	}
}

void
CServer::handleKeyboardBroadcastEvent(const CEvent& event, void*)
{
	CKeyboardBroadcastInfo* info = (CKeyboardBroadcastInfo*)event.getData();

	// choose new state
	bool newState;
	switch (info->m_state) {
	case CKeyboardBroadcastInfo::kOff:
		newState = false;
		break;

	default:
	case CKeyboardBroadcastInfo::kOn:
		newState = true;
		break;

	case CKeyboardBroadcastInfo::kToggle:
		newState = !m_keyboardBroadcasting;
		break;
	}

	// enter new state
	if (newState != m_keyboardBroadcasting ||
		info->m_screens != m_keyboardBroadcastingScreens) {
		m_keyboardBroadcasting        = newState;
		m_keyboardBroadcastingScreens = info->m_screens;
		LOG((CLOG_DEBUG "keyboard broadcasting %s: %s", m_keyboardBroadcasting ? "on" : "off", m_keyboardBroadcastingScreens.c_str()));
	}
}

void
CServer::handleLockCursorToScreenEvent(const CEvent& event, void*)
{
	CLockCursorToScreenInfo* info = (CLockCursorToScreenInfo*)event.getData();

	// choose new state
	bool newState;
	switch (info->m_state) {
	case CLockCursorToScreenInfo::kOff:
		newState = false;
		break;

	default:
	case CLockCursorToScreenInfo::kOn:
		newState = true;
		break;

	case CLockCursorToScreenInfo::kToggle:
		newState = !m_lockedToScreen;
		break;
	}

	// enter new state
	if (newState != m_lockedToScreen) {
		m_lockedToScreen = newState;
		LOG((CLOG_DEBUG "cursor %s current screen", m_lockedToScreen ? "locked to" : "unlocked from"));

		m_primaryClient->reconfigure(getActivePrimarySides());
		if (!isLockedToScreenServer()) {
			stopRelativeMoves();
		}
	}
}

void
CServer::handleFakeInputBeginEvent(const CEvent&, void*)
{
	m_primaryClient->fakeInputBegin();
}

void
CServer::handleFakeInputEndEvent(const CEvent&, void*)
{
	m_primaryClient->fakeInputEnd();
}

void
CServer::handleFileChunkSendingEvent(const CEvent& event, void*)
{
	onFileChunkSending(event.getData());
}

void
CServer::handleFileRecieveCompletedEvent(const CEvent& event, void*)
{
	onFileRecieveCompleted();
}

void
CServer::onClipboardChanged(CBaseClientProxy* sender,
				ClipboardID id, UInt32 seqNum)
{
	CClipboardInfo& clipboard = m_clipboards[id];

	// ignore update if sequence number is old
	if (seqNum < clipboard.m_clipboardSeqNum) {
		LOG((CLOG_INFO "ignored screen \"%s\" update of clipboard %d (missequenced)", getName(sender).c_str(), id));
		return;
	}

	// should be the expected client
	assert(sender == m_clients.find(clipboard.m_clipboardOwner)->second);

	// get data
	sender->getClipboard(id, &clipboard.m_clipboard);

	// ignore if data hasn't changed
	CString data = clipboard.m_clipboard.marshall();
	if (data == clipboard.m_clipboardData) {
		LOG((CLOG_DEBUG "ignored screen \"%s\" update of clipboard %d (unchanged)", clipboard.m_clipboardOwner.c_str(), id));
		return;
	}

	// got new data
	LOG((CLOG_INFO "screen \"%s\" updated clipboard %d", clipboard.m_clipboardOwner.c_str(), id));
	clipboard.m_clipboardData = data;

	// tell all clients except the sender that the clipboard is dirty
	for (CClientList::const_iterator index = m_clients.begin();
								index != m_clients.end(); ++index) {
		CBaseClientProxy* client = index->second;
		client->setClipboardDirty(id, client != sender);
	}

	// send the new clipboard to the active screen
	m_active->setClipboard(id, &clipboard.m_clipboard);
}

void
CServer::onScreensaver(bool activated)
{
	LOG((CLOG_DEBUG "onScreenSaver %s", activated ? "activated" : "deactivated"));

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
			CBaseClientProxy* screen = m_activeSaver;
			SInt32 x, y, w, h;
			screen->getShape(x, y, w, h);
			SInt32 zoneSize = getJumpZoneSize(screen);
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
		CBaseClientProxy* client = index->second;
		client->screensaver(activated);
	}
}

void
CServer::onKeyDown(KeyID id, KeyModifierMask mask, KeyButton button,
				const char* screens)
{
	LOG((CLOG_DEBUG1 "onKeyDown id=%d mask=0x%04x button=0x%04x", id, mask, button));
	assert(m_active != NULL);

	// relay
	if (!m_keyboardBroadcasting && IKeyState::CKeyInfo::isDefault(screens)) {
		m_active->keyDown(id, mask, button);
	}
	else {
		if (!screens && m_keyboardBroadcasting) {
			screens = m_keyboardBroadcastingScreens.c_str();
			if (IKeyState::CKeyInfo::isDefault(screens)) {
				screens = "*";
			}
		}
		for (CClientList::const_iterator index = m_clients.begin();
								index != m_clients.end(); ++index) {
			if (IKeyState::CKeyInfo::contains(screens, index->first)) {
				index->second->keyDown(id, mask, button);
			}
		}
	}
}

void
CServer::onKeyUp(KeyID id, KeyModifierMask mask, KeyButton button,
				const char* screens)
{
	LOG((CLOG_DEBUG1 "onKeyUp id=%d mask=0x%04x button=0x%04x", id, mask, button));
	assert(m_active != NULL);

	// relay
	if (!m_keyboardBroadcasting && IKeyState::CKeyInfo::isDefault(screens)) {
		m_active->keyUp(id, mask, button);
	}
	else {
		if (!screens && m_keyboardBroadcasting) {
			screens = m_keyboardBroadcastingScreens.c_str();
			if (IKeyState::CKeyInfo::isDefault(screens)) {
				screens = "*";
			}
		}
		for (CClientList::const_iterator index = m_clients.begin();
								index != m_clients.end(); ++index) {
			if (IKeyState::CKeyInfo::contains(screens, index->first)) {
				index->second->keyUp(id, mask, button);
			}
		}
	}
}

void
CServer::onKeyRepeat(KeyID id, KeyModifierMask mask,
				SInt32 count, KeyButton button)
{
	LOG((CLOG_DEBUG1 "onKeyRepeat id=%d mask=0x%04x count=%d button=0x%04x", id, mask, count, button));
	assert(m_active != NULL);

	// relay
	m_active->keyRepeat(id, mask, count, button);
}

void
CServer::onMouseDown(ButtonID id)
{
	LOG((CLOG_DEBUG1 "onMouseDown id=%d", id));
	assert(m_active != NULL);

	// relay
	m_active->mouseDown(id);
}

void
CServer::onMouseUp(ButtonID id)
{
	LOG((CLOG_DEBUG1 "onMouseUp id=%d", id));
	assert(m_active != NULL);

	// relay
	m_active->mouseUp(id);

	if (m_ignoreFileTransfer) {
		m_ignoreFileTransfer = false;
		return;
	}
	
	if (m_enableDragDrop && !m_screen->isOnScreen()) {
		CString& file = m_screen->getDraggingFilename();
		if (!file.empty()) {
			LOG((CLOG_DEBUG "send file to client: %s", file.c_str()));
			sendFileToClient(file.c_str());
		}
	}
}

bool
CServer::onMouseMovePrimary(SInt32 x, SInt32 y)
{
	LOG((CLOG_DEBUG4 "onMouseMovePrimary %d,%d", x, y));

	// mouse move on primary (server's) screen
	if (m_active != m_primaryClient) {
		// stale event -- we're actually on a secondary screen
		return false;
	}

	// save last delta
	m_xDelta2 = m_xDelta;
	m_yDelta2 = m_yDelta;

	// save current delta
	m_xDelta  = x - m_x;
	m_yDelta  = y - m_y;

	// save position
	m_x       = x;
	m_y       = y;

	// get screen shape
	SInt32 ax, ay, aw, ah;
	m_active->getShape(ax, ay, aw, ah);
	SInt32 zoneSize = getJumpZoneSize(m_active);

	// clamp position to screen
	SInt32 xc = x, yc = y;
	if (xc < ax + zoneSize) {
		xc = ax;
	}
	else if (xc >= ax + aw - zoneSize) {
		xc = ax + aw - 1;
	}
	if (yc < ay + zoneSize) {
		yc = ay;
	}
	else if (yc >= ay + ah - zoneSize) {
		yc = ay + ah - 1;
	}

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
		// still on local screen
		noSwitch(x, y);
		return false;
	}

	// get jump destination
	CBaseClientProxy* newScreen = mapToNeighbor(m_active, dir, x, y);

	// should we switch or not?
	if (isSwitchOkay(newScreen, dir, x, y, xc, yc)) {
		if (m_enableDragDrop && m_screen->getDraggingStarted() && m_active != newScreen) {
			CString& dragFileList = m_screen->getDraggingFilename();
			size_t size = dragFileList.size() + 1;
			char* fileList = NULL;
			UInt32 fileCount = 1;
			if (dragFileList.empty() == false) {
				fileList = new char[size];
				memcpy(fileList, dragFileList.c_str(), size);
				fileList[size - 1] = '\0';
			}

			// fake a escape key down and up then left mouse button up
			m_screen->keyDown(kKeyEscape, 8192, 1);
			m_screen->keyUp(kKeyEscape, 8192, 1);
			m_screen->mouseUp(kButtonLeft);
			
#if defined(__APPLE__)
			
			// on mac it seems that after faking a LMB up, system would signal back
			// to synergy a mouse up event, which doesn't happen on windows. as a
			// result, synergy would send dragging file to client twice. This variable
			// is used to ignore the first file sending.
			m_ignoreFileTransfer = true;
#endif
			
			if (dragFileList.empty() == false) {
				LOG((CLOG_DEBUG2 "sending drag information to client"));
				LOG((CLOG_DEBUG3 "dragging file list: %s", fileList));
				LOG((CLOG_DEBUG3 "dragging file list string size: %i", size));
				newScreen->draggingInfoSending(fileCount, fileList, size);
			}
		}

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

	// mouse move on secondary (client's) screen
	assert(m_active != NULL);
	if (m_active == m_primaryClient) {
		// stale event -- we're actually on the primary screen
		return;
	}

	// if doing relative motion on secondary screens and we're locked
	// to the screen (which activates relative moves) then send a
	// relative mouse motion.  when we're doing this we pretend as if
	// the mouse isn't actually moving because we're expecting some
	// program on the secondary screen to warp the mouse on us, so we
	// have no idea where it really is.
	if (m_relativeMoves && isLockedToScreenServer()) {
		LOG((CLOG_DEBUG2 "relative move on %s by %d,%d", getName(m_active).c_str(), dx, dy));
		m_active->mouseRelativeMove(dx, dy);
		return;
	}

	// save old position
	const SInt32 xOld = m_x;
	const SInt32 yOld = m_y;

	// save last delta
	m_xDelta2 = m_xDelta;
	m_yDelta2 = m_yDelta;

	// save current delta
	m_xDelta  = dx;
	m_yDelta  = dy;

	// accumulate motion
	m_x      += dx;
	m_y      += dy;

	// get screen shape
	SInt32 ax, ay, aw, ah;
	m_active->getShape(ax, ay, aw, ah);

	// find direction of neighbor and get the neighbor
	bool jump = true;
	CBaseClientProxy* newScreen;
	do {
		// clamp position to screen
		SInt32 xc = m_x, yc = m_y;
		if (xc < ax) {
			xc = ax;
		}
		else if (xc >= ax + aw) {
			xc = ax + aw - 1;
		}
		if (yc < ay) {
			yc = ay;
		}
		else if (yc >= ay + ah) {
			yc = ay + ah - 1;
		}

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
					// still on local screen
					noSwitch(m_x, m_y);
				}
			}

			// skip rest of block
			break;
		}

		// try to switch screen.  get the neighbor.
		newScreen = mapToNeighbor(m_active, dir, m_x, m_y);

		// see if we should switch
		if (!isSwitchOkay(newScreen, dir, m_x, m_y, xc, yc)) {
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
			LOG((CLOG_DEBUG2 "clamp to left of \"%s\"", getName(m_active).c_str()));
		}
		else if (m_x > ax + aw - 1) {
			m_x = ax + aw - 1;
			LOG((CLOG_DEBUG2 "clamp to right of \"%s\"", getName(m_active).c_str()));
		}
		if (m_y < ay) {
			m_y = ay;
			LOG((CLOG_DEBUG2 "clamp to top of \"%s\"", getName(m_active).c_str()));
		}
		else if (m_y > ay + ah - 1) {
			m_y = ay + ah - 1;
			LOG((CLOG_DEBUG2 "clamp to bottom of \"%s\"", getName(m_active).c_str()));
		}

		// warp cursor if it moved.
		if (m_x != xOld || m_y != yOld) {
			LOG((CLOG_DEBUG2 "move on %s to %d,%d", getName(m_active).c_str(), m_x, m_y));
			m_active->mouseMove(m_x, m_y);
		}
	}
}

void
CServer::onMouseWheel(SInt32 xDelta, SInt32 yDelta)
{
	LOG((CLOG_DEBUG1 "onMouseWheel %+d,%+d", xDelta, yDelta));
	assert(m_active != NULL);

	// relay
	m_active->mouseWheel(xDelta, yDelta);
}

void
CServer::onFileChunkSending(const void* data)
{
	CFileChunker::CFileChunk* fileChunk = reinterpret_cast<CFileChunker::CFileChunk*>(const_cast<void*>(data));

	LOG((CLOG_DEBUG1 "onFileChunkSending"));
	assert(m_active != NULL);

	// relay
 	m_active->fileChunkSending(fileChunk->m_chunk[0], &(fileChunk->m_chunk[1]), fileChunk->m_dataSize);
}

void
CServer::onFileRecieveCompleted()
{
	if (isReceivedFileSizeValid()) {
		m_writeToDropDirThread = new CThread(
									   new TMethodJob<CServer>(
															   this, &CServer::writeToDropDirThread));
	}
}

void
CServer::writeToDropDirThread(void*)
{
	LOG((CLOG_DEBUG "starting write to drop dir thread"));

	while (m_screen->getFakeDraggingStarted()) {
		ARCH->sleep(.1f);
	}

	m_fileTransferDes = m_screen->getDropTarget();
	LOG((CLOG_DEBUG "dropping file, files=%i target=%s", m_dragFileList.size(), m_fileTransferDes.c_str()));

	if (!m_fileTransferDes.empty() && m_dragFileList.size() > 0) {
		std::fstream file;
		CString dropTarget = m_fileTransferDes;
#ifdef SYSAPI_WIN32
		dropTarget.append("\\");
#else
		dropTarget.append("/");
#endif
		dropTarget.append(m_dragFileList.at(0));
		file.open(dropTarget.c_str(), std::ios::out | std::ios::binary);
		if (!file.is_open()) {
			// TODO: file open failed
		}
		
		file.write(m_receivedFileData.c_str(), m_receivedFileData.size());
		file.close();
	}
	else {
		LOG((CLOG_ERR "drop file failed: drop target is empty"));
	}
}

bool
CServer::addClient(CBaseClientProxy* client)
{
	CString name = getName(client);
	if (m_clients.count(name) != 0) {
		return false;
	}

	// add event handlers
	m_events->adoptHandler(m_events->forIScreen().shapeChanged(),
							client->getEventTarget(),
							new TMethodEventJob<CServer>(this,
								&CServer::handleShapeChanged, client));
	m_events->adoptHandler(m_events->forIScreen().clipboardGrabbed(),
							client->getEventTarget(),
							new TMethodEventJob<CServer>(this,
								&CServer::handleClipboardGrabbed, client));
	m_events->adoptHandler(m_events->forCClientProxy().clipboardChanged(),
							client->getEventTarget(),
							new TMethodEventJob<CServer>(this,
								&CServer::handleClipboardChanged, client));

	// add to list
	m_clientSet.insert(client);
	m_clients.insert(std::make_pair(name, client));

	// initialize client data
	SInt32 x, y;
	client->getCursorPos(x, y);
	client->setJumpCursorPos(x, y);

	// tell primary client about the active sides
	m_primaryClient->reconfigure(getActivePrimarySides());

	return true;
}

bool
CServer::removeClient(CBaseClientProxy* client)
{
	// return false if not in list
	CClientSet::iterator i = m_clientSet.find(client);
	if (i == m_clientSet.end()) {
		return false;
	}

	// remove event handlers
	m_events->removeHandler(m_events->forIScreen().shapeChanged(),
							client->getEventTarget());
	m_events->removeHandler(m_events->forIScreen().clipboardGrabbed(),
							client->getEventTarget());
	m_events->removeHandler(m_events->forCClientProxy().clipboardChanged(),
							client->getEventTarget());

	// remove from list
	m_clients.erase(getName(client));
	m_clientSet.erase(i);

	return true;
}

void
CServer::closeClient(CBaseClientProxy* client, const char* msg)
{
	assert(client != m_primaryClient);
	assert(msg != NULL);

	// send message to client.  this message should cause the client
	// to disconnect.  we add this client to the closed client list
	// and install a timer to remove the client if it doesn't respond
	// quickly enough.  we also remove the client from the active
	// client list since we're not going to listen to it anymore.
	// note that this method also works on clients that are not in
	// the m_clients list.  adoptClient() may call us with such a
	// client.
	LOG((CLOG_NOTE "disconnecting client \"%s\"", getName(client).c_str()));

	// send message
	// FIXME -- avoid type cast (kinda hard, though)
	((CClientProxy*)client)->close(msg);

	// install timer.  wait timeout seconds for client to close.
	double timeout = 5.0;
	CEventQueueTimer* timer = m_events->newOneShotTimer(timeout, NULL);
	m_events->adoptHandler(CEvent::kTimer, timer,
							new TMethodEventJob<CServer>(this,
								&CServer::handleClientCloseTimeout, client));

	// move client to closing list
	removeClient(client);
	m_oldClients.insert(std::make_pair(client, timer));

	// if this client is the active screen then we have to
	// jump off of it
	forceLeaveClient(client);
}

void
CServer::closeClients(const CConfig& config)
{
	// collect the clients that are connected but are being dropped
	// from the configuration (or who's canonical name is changing).
	typedef std::set<CBaseClientProxy*> CRemovedClients;
	CRemovedClients removed;
	for (CClientList::iterator index = m_clients.begin();
								index != m_clients.end(); ++index) {
		if (!config.isCanonicalName(index->first)) {
			removed.insert(index->second);
		}
	}

	// don't close the primary client
	removed.erase(m_primaryClient);

	// now close them.  we collect the list then close in two steps
	// because closeClient() modifies the collection we iterate over.
	for (CRemovedClients::iterator index = removed.begin();
								index != removed.end(); ++index) {
		closeClient(*index, kMsgCClose);
	}
}

void
CServer::removeActiveClient(CBaseClientProxy* client)
{
	if (removeClient(client)) {
		forceLeaveClient(client);
		m_events->removeHandler(m_events->forCClientProxy().disconnected(), client);
		if (m_clients.size() == 1 && m_oldClients.empty()) {
			m_events->addEvent(CEvent(m_events->forCServer().disconnected(), this));
		}
	}
}

void
CServer::removeOldClient(CBaseClientProxy* client)
{
	COldClients::iterator i = m_oldClients.find(client);
	if (i != m_oldClients.end()) {
		m_events->removeHandler(m_events->forCClientProxy().disconnected(), client);
		m_events->removeHandler(CEvent::kTimer, i->second);
		m_events->deleteTimer(i->second);
		m_oldClients.erase(i);
		if (m_clients.size() == 1 && m_oldClients.empty()) {
			m_events->addEvent(CEvent(m_events->forCServer().disconnected(), this));
		}
	}
}

void
CServer::forceLeaveClient(CBaseClientProxy* client)
{
	CBaseClientProxy* active =
		(m_activeSaver != NULL) ? m_activeSaver : m_active;
	if (active == client) {
		// record new position (center of primary screen)
		m_primaryClient->getCursorCenter(m_x, m_y);

		// stop waiting to switch to this client
		if (active == m_switchScreen) {
			stopSwitch();
		}

		// don't notify active screen since it has probably already
		// disconnected.
		LOG((CLOG_INFO "jump from \"%s\" to \"%s\" at %d,%d", getName(active).c_str(), getName(m_primaryClient).c_str(), m_x, m_y));

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
	if (m_activeSaver == client) {
		m_activeSaver = NULL;
	}

	// tell primary client about the active sides
	m_primaryClient->reconfigure(getActivePrimarySides());
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


//
// CServer::CLockCursorToScreenInfo
//

CServer::CLockCursorToScreenInfo*
CServer::CLockCursorToScreenInfo::alloc(State state)
{
	CLockCursorToScreenInfo* info =
		(CLockCursorToScreenInfo*)malloc(sizeof(CLockCursorToScreenInfo));
	info->m_state = state;
	return info;
}


//
// CServer::CSwitchToScreenInfo
//

CServer::CSwitchToScreenInfo*
CServer::CSwitchToScreenInfo::alloc(const CString& screen)
{
	CSwitchToScreenInfo* info =
		(CSwitchToScreenInfo*)malloc(sizeof(CSwitchToScreenInfo) +
								screen.size());
	strcpy(info->m_screen, screen.c_str());
	return info;
}


//
// CServer::CSwitchInDirectionInfo
//

CServer::CSwitchInDirectionInfo*
CServer::CSwitchInDirectionInfo::alloc(EDirection direction)
{
	CSwitchInDirectionInfo* info =
		(CSwitchInDirectionInfo*)malloc(sizeof(CSwitchInDirectionInfo));
	info->m_direction = direction;
	return info;
}

//
// CServer::CKeyboardBroadcastInfo
//

CServer::CKeyboardBroadcastInfo*
CServer::CKeyboardBroadcastInfo::alloc(State state)
{
	CKeyboardBroadcastInfo* info =
		(CKeyboardBroadcastInfo*)malloc(sizeof(CKeyboardBroadcastInfo));
	info->m_state      = state;
	info->m_screens[0] = '\0';
	return info;
}

CServer::CKeyboardBroadcastInfo*
CServer::CKeyboardBroadcastInfo::alloc(State state, const CString& screens)
{
	CKeyboardBroadcastInfo* info =
		(CKeyboardBroadcastInfo*)malloc(sizeof(CKeyboardBroadcastInfo) +
								screens.size());
	info->m_state = state;
	strcpy(info->m_screens, screens.c_str());
	return info;
}

void
CServer::clearReceivedFileData()
{
	m_receivedFileData.clear();
}

void
CServer::setExpectedFileSize(CString data)
{
	std::istringstream iss(data);
	iss >> m_expectedFileSize;
}

void
CServer::fileChunkReceived(CString data)
{
	m_receivedFileData += data;
}

bool
CServer::isReceivedFileSizeValid()
{
	return m_expectedFileSize == m_receivedFileData.size();
}

void
CServer::sendFileToClient(const char* filename)
{
	m_sendFileThread = new CThread(
		new TMethodJob<CServer>(
			this, &CServer::sendFileThread,
			reinterpret_cast<void*>(const_cast<char*>(filename))));
}

void
CServer::sendFileThread(void* filename)
{
	try {
		char* name  = reinterpret_cast<char*>(filename);
		CFileChunker::sendFileChunks(name, m_events, this);
	}
	catch (std::runtime_error error) {
		LOG((CLOG_ERR "failed sending file chunks: %s", error.what()));
	}

	m_sendFileThread = NULL;
}

void
CServer::dragInfoReceived(UInt32 fileNum, CString content)
{
	// TODO: fix duplicate function from CClient

	if (!m_enableDragDrop) {
		LOG((CLOG_DEBUG "drag drop not enabled, ignoring drag info."));
		return;
	}

	CDragInformation::parseDragInfo(m_dragFileList, fileNum, content);
	LOG((CLOG_DEBUG "drag info received, total drag file number: %i", m_dragFileList.size()));
	
	for (int i = 0; i < m_dragFileList.size(); ++i) {
		LOG((CLOG_DEBUG "dragging file %i name: %s", i + 1, m_dragFileList.at(i).c_str()));
	}
	
	if (m_dragFileList.size() == 1) {
		m_dragFileExt = CDragInformation::getDragFileExtension(m_dragFileList.at(0));
	}
	else if (m_dragFileList.size() > 1) {
		m_dragFileExt.clear();
	}
	else {
		return;
	}
	
	m_screen->startDraggingFiles(m_dragFileExt);
}

void
CServer::draggingInfoSending(UInt32 fileCount, CString& fileList, size_t size)
{
	m_active->draggingInfoSending(fileCount, fileList.c_str(), size);
}
