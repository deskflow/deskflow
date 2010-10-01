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
#include <cstring>
#include <cstdlib>

//
// CServer
//

CEvent::Type			CServer::s_errorEvent         = CEvent::kUnknown;
CEvent::Type			CServer::s_connectedEvent     = CEvent::kUnknown;
CEvent::Type			CServer::s_disconnectedEvent  = CEvent::kUnknown;
CEvent::Type			CServer::s_switchToScreen     = CEvent::kUnknown;
CEvent::Type			CServer::s_switchInDirection  = CEvent::kUnknown;
CEvent::Type			CServer::s_keyboardBroadcast  = CEvent::kUnknown;
CEvent::Type			CServer::s_lockCursorToScreen = CEvent::kUnknown;

CServer::CServer(const CConfig& config, CPrimaryClient* primaryClient) :
	m_primaryClient(primaryClient),
	m_active(primaryClient),
	m_dev(CDeviceManager::getInstance()),
	m_seqNum(0),
	m_config(),
	m_inputFilter(m_config.getInputFilter()),
	m_activeSaver(NULL),
	m_switchDir(kNoDirection),
	m_switchScreen(NULL),
	m_switchWaitDelay(0.0),
	m_switchWaitTimer(NULL),
	m_switchTwoTapDelay(0.0),
	m_switchTwoTapEngaged(false),
	m_switchTwoTapArmed(false),
	m_switchTwoTapZone(3),
	m_keyboardBroadcasting(false),
	m_lockedToScreen(false)
{
	// must have a primary client and it must have a canonical name
	assert(m_primaryClient != NULL);
	assert(config.isScreen(primaryClient->getName()));

	LOG((CLOG_DEBUG "m_primaryClient name: %s", m_primaryClient->getName().c_str()));
	LOG((CLOG_DEBUG "CServer::primaryClient address %p", primaryClient));
	LOG((CLOG_DEBUG "CServer::m_primaryClient address %p", m_primaryClient));
	LOG((CLOG_DEBUG "CServer::m_active address %p", m_active));
	
	m_dev->setPrimaryClient(primaryClient);
	
	SInt32 x, y;
	std::list<UInt8> ptrIDs;
	std::list<UInt8>::iterator i;
	m_dev->getAllPointerIDs(ptrIDs);
	for(i=ptrIDs.begin(); i != ptrIDs.end(); ++i)
	{
	    m_dev->setActiveClient(primaryClient, *i);
	    m_dev->setActiveClient(primaryClient, m_dev->getAttachment(*i));
	}
	
	CString primaryName = getName(primaryClient);

	// clear clipboards
	for (ClipboardID cId = 0; cId < kClipboardEnd; ++cId) {
		CClipboardInfo& clipboard   = m_clipboards[cId];
		clipboard.m_clipboardOwner  = primaryName;
		clipboard.m_clipboardSeqNum = m_seqNum;
		if (clipboard.m_clipboard.open(0)) {
			clipboard.m_clipboard.empty();
			clipboard.m_clipboard.close();
		}
		clipboard.m_clipboardData   = clipboard.m_clipboard.marshall();
	}

	// install event handlers
	EVENTQUEUE->adoptHandler(CEvent::kTimer, this,
							new TMethodEventJob<CServer>(this,
								&CServer::handleSwitchWaitTimeout));
	EVENTQUEUE->adoptHandler(IPlatformScreen::getKeyDownEvent(),
							m_inputFilter,
							new TMethodEventJob<CServer>(this,
								&CServer::handleKeyDownEvent));
	EVENTQUEUE->adoptHandler(IPlatformScreen::getKeyUpEvent(),
							m_inputFilter,
							new TMethodEventJob<CServer>(this,
								&CServer::handleKeyUpEvent));
	EVENTQUEUE->adoptHandler(IPlatformScreen::getKeyRepeatEvent(),
							m_inputFilter,
							new TMethodEventJob<CServer>(this,
								&CServer::handleKeyRepeatEvent));
	EVENTQUEUE->adoptHandler(IPlatformScreen::getButtonDownEvent(),
							m_inputFilter,
							new TMethodEventJob<CServer>(this,
								&CServer::handleButtonDownEvent));
	EVENTQUEUE->adoptHandler(IPlatformScreen::getButtonUpEvent(),
							m_inputFilter,
							new TMethodEventJob<CServer>(this,
								&CServer::handleButtonUpEvent));
	EVENTQUEUE->adoptHandler(IPlatformScreen::getMotionOnPrimaryEvent(),
							m_primaryClient->getEventTarget(),
							new TMethodEventJob<CServer>(this,
								&CServer::handleMotionPrimaryEvent));
	EVENTQUEUE->adoptHandler(IPlatformScreen::getMotionOnSecondaryEvent(),
							m_primaryClient->getEventTarget(),
							new TMethodEventJob<CServer>(this,
								&CServer::handleMotionSecondaryEvent));
	EVENTQUEUE->adoptHandler(IPlatformScreen::getWheelEvent(),
							m_primaryClient->getEventTarget(),
							new TMethodEventJob<CServer>(this,
								&CServer::handleWheelEvent));
	EVENTQUEUE->adoptHandler(IPlatformScreen::getScreensaverActivatedEvent(),
							m_primaryClient->getEventTarget(),
							new TMethodEventJob<CServer>(this,
								&CServer::handleScreensaverActivatedEvent));
	EVENTQUEUE->adoptHandler(IPlatformScreen::getScreensaverDeactivatedEvent(),
							m_primaryClient->getEventTarget(),
							new TMethodEventJob<CServer>(this,
								&CServer::handleScreensaverDeactivatedEvent));
	EVENTQUEUE->adoptHandler(getSwitchToScreenEvent(),
							m_inputFilter,
							new TMethodEventJob<CServer>(this,
								&CServer::handleSwitchToScreenEvent));
	EVENTQUEUE->adoptHandler(getSwitchInDirectionEvent(),
							m_inputFilter,
							new TMethodEventJob<CServer>(this,
								&CServer::handleSwitchInDirectionEvent));
	EVENTQUEUE->adoptHandler(getKeyboardBroadcastEvent(),
							m_inputFilter,
							new TMethodEventJob<CServer>(this,
								&CServer::handleKeyboardBroadcastEvent));
	EVENTQUEUE->adoptHandler(getLockCursorToScreenEvent(),
							m_inputFilter,
							new TMethodEventJob<CServer>(this,
								&CServer::handleLockCursorToScreenEvent));
	EVENTQUEUE->adoptHandler(IPlatformScreen::getFakeInputBeginEvent(),
							m_inputFilter,
							new TMethodEventJob<CServer>(this,
								&CServer::handleFakeInputBeginEvent));
	EVENTQUEUE->adoptHandler(IPlatformScreen::getFakeInputEndEvent(),
							m_inputFilter,
							new TMethodEventJob<CServer>(this,
								&CServer::handleFakeInputEndEvent));

	// add connection
	addClient(m_primaryClient);

	// set initial configuration
	setConfig(config);

	// enable primary client
	m_primaryClient->enable();
	m_inputFilter->setPrimaryClient(m_primaryClient);
}

CServer::~CServer()
{
	// remove event handlers and timers
	EVENTQUEUE->removeHandler(IPlatformScreen::getKeyDownEvent(),
							m_inputFilter);
	EVENTQUEUE->removeHandler(IPlatformScreen::getKeyUpEvent(),
							m_inputFilter);
	EVENTQUEUE->removeHandler(IPlatformScreen::getKeyRepeatEvent(),
							m_inputFilter);
	EVENTQUEUE->removeHandler(IPlatformScreen::getButtonDownEvent(),
							m_inputFilter);
	EVENTQUEUE->removeHandler(IPlatformScreen::getButtonUpEvent(),
							m_inputFilter);
	EVENTQUEUE->removeHandler(IPlatformScreen::getMotionOnPrimaryEvent(),
							m_primaryClient->getEventTarget());
	EVENTQUEUE->removeHandler(IPlatformScreen::getMotionOnSecondaryEvent(),
							m_primaryClient->getEventTarget());
	EVENTQUEUE->removeHandler(IPlatformScreen::getWheelEvent(),
							m_primaryClient->getEventTarget());
	EVENTQUEUE->removeHandler(IPlatformScreen::getScreensaverActivatedEvent(),
							m_primaryClient->getEventTarget());
	EVENTQUEUE->removeHandler(IPlatformScreen::getScreensaverDeactivatedEvent(),
							m_primaryClient->getEventTarget());
	EVENTQUEUE->removeHandler(IPlatformScreen::getFakeInputBeginEvent(),
							m_inputFilter);
	EVENTQUEUE->removeHandler(IPlatformScreen::getFakeInputEndEvent(),
							m_inputFilter);
	EVENTQUEUE->removeHandler(CEvent::kTimer, this);
	stopSwitch();

	// force immediate disconnection of secondary clients
	disconnect();
	for (COldClients::iterator index = m_oldClients.begin();
							index != m_oldClients.begin(); ++index) {
		CBaseClientProxy* client = index->first;
		EVENTQUEUE->deleteTimer(index->second);
		EVENTQUEUE->removeHandler(CEvent::kTimer, client);
		EVENTQUEUE->removeHandler(CClientProxy::getDisconnectedEvent(), client);
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
// FIXXME setConfig id
UInt8 id = 3;
	// close clients that are connected but being dropped from the
	// configuration.
	closeClients(config);

	// cut over
	m_config = config;
	processOptions();

	// add ScrollLock as a hotkey to lock to the screen.  this was a
	// built-in feature in earlier releases and is now supported via
	// the user configurable hotkey mechanism.  if the user has already
	// registered ScrollLock for something else then that will win but
	// we will unfortunately generate a warning.  if the user has
	// configured a CLockCursorToScreenAction then we don't add
	// ScrollLock as a hotkey.
	if (!m_config.hasLockToScreenAction()) {
		IPlatformScreen::CKeyInfo* key =
			IPlatformScreen::CKeyInfo::alloc(kKeyScrollLock, 0, 0, 0, id);
		CInputFilter::CRule rule(new CInputFilter::CKeystrokeCondition(key));
		rule.adoptAction(new CInputFilter::CLockCursorToScreenAction, true);
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
	EVENTQUEUE->adoptHandler(CClientProxy::getDisconnectedEvent(), client,
							new TMethodEventJob<CServer>(this,
								&CServer::handleClientDisconnected, client));

	// name must be in our configuration
	if (!m_config.isScreen(client->getName())) {
		LOG((CLOG_WARN "a client with name \"%s\" is not in the map", client->getName().c_str()));
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
		CServer::CScreenConnectedInfo::alloc(getName(client));
	EVENTQUEUE->addEvent(CEvent(CServer::getConnectedEvent(),
								m_primaryClient->getEventTarget(), info));
}

void
CServer::disconnect()
{
	// close all secondary clients
	if (m_clients.size() > 1 || !m_oldClients.empty()) {
		CConfig emptyConfig;
		closeClients(emptyConfig);
	}
	else {
		EVENTQUEUE->addEvent(CEvent(getDisconnectedEvent(), this));
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

CEvent::Type
CServer::getErrorEvent()
{
	return CEvent::registerTypeOnce(s_errorEvent,
							"CServer::error");
}

CEvent::Type
CServer::getConnectedEvent()
{
	return CEvent::registerTypeOnce(s_connectedEvent,
							"CServer::connected");
}

CEvent::Type
CServer::getDisconnectedEvent()
{
	return CEvent::registerTypeOnce(s_disconnectedEvent,
							"CServer::disconnected");
}

CEvent::Type
CServer::getSwitchToScreenEvent()
{
	return CEvent::registerTypeOnce(s_switchToScreen,
							"CServer::switchToScreen");
}

CEvent::Type
CServer::getSwitchInDirectionEvent()
{
	return CEvent::registerTypeOnce(s_switchInDirection,
							"CServer::switchInDirection");
}

CEvent::Type
CServer::getKeyboardBroadcastEvent()
{
	return CEvent::registerTypeOnce(s_keyboardBroadcast,
							"CServer:keyboardBroadcast");
}

CEvent::Type
CServer::getLockCursorToScreenEvent()
{
	return CEvent::registerTypeOnce(s_lockCursorToScreen,
							"CServer::lockCursorToScreen");
}

CString
CServer::getName(const CBaseClientProxy* client) const
{
	CString name = m_config.getCanonicalName(client->getName());
	if (name.empty()) {
		name = client->getName();
	}
	return name;
}

UInt32
CServer::getActivePrimarySides() const
{
	UInt32 sides = 0;
	// FIXXXME getActivePrimarySides: I think it would be ok to do this for just any ID ?
	std::list<UInt8> ptrIDs;
	std::list<UInt8>::iterator i;
	m_dev->getAllPointerIDs(ptrIDs);
	for(i=ptrIDs.begin(); i != ptrIDs.end(); ++i)
	{
	    if (!isLockedToScreenServer(*i)) {
	        sides = 0;
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
	}
	return sides;
}

bool
CServer::isLockedToScreenServer(UInt8 id) const
{
	// locked if scroll-lock is toggled on
	return m_dev->isLockedToScreen(id);
}

bool
CServer::isLockedToScreen(UInt8 id) const
{
	// locked if we say we're locked
	if (isLockedToScreenServer(id)) {
		LOG((CLOG_DEBUG "locked to screen"));
		return true;
	}

	// locked if primary says we're locked
	if (m_primaryClient->isLockedToScreen(id)) {
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
				SInt32 x, SInt32 y, UInt8 id, bool forScreensaver)
{
	assert(dst != NULL);
#ifndef NDEBUG
	{
		SInt32 dx, dy, dw, dh;
		dst->getShape(dx, dy, dw, dh);
		assert(x >= dx && y >= dy && x < dx + dw && y < dy + dh);
	}
#endif
	CBaseClientProxy *l_active = m_dev->getActiveClient(id);
	assert(l_active != NULL);

	LOG((CLOG_INFO "switch by dev(%d) from \"%s\" to \"%s\" at %d,%d", id, getName(m_active).c_str(), getName(dst).c_str(), x, y));

	// stop waiting to switch
	stopSwitch();

	// record new position
//	m_x       = x;
//	m_y       = y;
// FIXXXME switchscreen: should we safe this?
	m_dev->setLastCursorPos(x, y, id);
	m_dev->setCursorDelta(0, 0, id);
	m_dev->setCursorDelta2(0, 0, id);
// 	m_xDelta  = 0;
// 	m_yDelta  = 0;
// 	m_xDelta2 = 0;
// 	m_yDelta2 = 0;

	// wrapping means leaving the active screen and entering it again.
	// since that's a waste of time we skip that and just warp the
	// mouse.
	if (l_active != dst) {
		// leave active screen
		// FIXXXME switchscreen: leave ?
 		if (!l_active->leave(id)) {
 			// cannot leave screen
 			LOG((CLOG_WARN "can't leave screen"));
 			return;
 		}

		// update the primary client's clipboards if we're leaving the
		// primary screen.
		if (l_active == m_primaryClient) {
			for (ClipboardID cId = 0; cId < kClipboardEnd; ++cId) {
				CClipboardInfo& clipboard = m_clipboards[cId];
				if (clipboard.m_clipboardOwner == getName(m_primaryClient)) {
					onClipboardChanged(m_primaryClient,
						cId, clipboard.m_clipboardSeqNum);
				}
			}
		}

		// cut over
		m_dev->setActiveClient(dst, id);
		m_dev->setActiveClient(dst, m_dev->getAttachment(id));
                l_active = m_dev->getActiveClient(id);
		
		// increment enter sequence number
		++m_seqNum;

		// enter new screen
		l_active->enter(x, y, m_seqNum, m_primaryClient->getToggleMask(id),
				forScreensaver, m_dev->getAttachment(id), id);

		// send the clipboard data to new active screen
		for (ClipboardID cId = 0; cId < kClipboardEnd; ++cId) {
			m_active->setClipboard(cId, &m_clipboards[cId].m_clipboard);
		}
	}
	else {
		l_active->mouseMove(x, y, id);
	}
}

void
CServer::jumpToScreen(CBaseClientProxy* newScreen, UInt8 id)
{
	assert(newScreen != NULL);
	SInt32 l_x, l_y;

	m_dev->getLastCursorPos(l_x, l_y, id);
	// FIXXXME addClient: need to do jump coords in array form per client
	// record the current cursor position on the active screen
	m_dev->setJumpCursorPos(m_active->getName(), l_x, l_y, id);

	// get the last cursor position on the target screen
	SInt32 x, y;
	m_dev->getJumpCursorPos(newScreen->getName(), x, y, id);
	
	switchScreen(newScreen, x, y, id, false);
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

	return m_config.hasNeighbor(getName(client), dir);
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
		CString dstName(m_config.getNeighbor(srcName, dir, t, &tTmp));

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
				EDirection srcSide, SInt32& x, SInt32& y, UInt8 id) const
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
		if (!m_config.getNeighbor(dstName, kRight, t, NULL).empty() &&
			x > dx + dw - 1 - z)
			x = dx + dw - 1 - z;
		break;

	case kRight:
		if (!m_config.getNeighbor(dstName, kLeft, t, NULL).empty() &&
			x < dx + z)
			x = dx + z;
		break;

	case kTop:
		if (!m_config.getNeighbor(dstName, kBottom, t, NULL).empty() &&
			y > dy + dh - 1 - z)
			y = dy + dh - 1 - z;
		break;

	case kBottom:
		if (!m_config.getNeighbor(dstName, kTop, t, NULL).empty() &&
			y < dy + z)
			y = dy + z;
		break;

	case kNoDirection:
		assert(0 && "bad direction");
	}
}

bool
CServer::isSwitchOkay(CBaseClientProxy* newScreen,
				EDirection dir, SInt32 x, SInt32 y, UInt8 id, 
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
						m_config.getOptions(getName(m_active));
	if (options == NULL || options->count(kOptionScreenSwitchCorners) == 0) {
		options = m_config.getOptions("");
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
	if (!preventSwitch && isLockedToScreen(id)) {
		LOG((CLOG_DEBUG1 "locked to screen"));
		preventSwitch = true;
		stopSwitch();
	}

	return !preventSwitch;
}

void
CServer::noSwitch(SInt32 x, SInt32 y, UInt8 id)
{
	armSwitchTwoTap(x, y, id);
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
CServer::armSwitchTwoTap(SInt32 x, SInt32 y, UInt8 id)
{
    SInt32 l_xDelta, l_yDelta, l_xDelta2, l_yDelta2;
    m_dev->getCursorDelta(l_xDelta, l_yDelta, id);
    m_dev->getCursorDelta2(l_xDelta2, l_yDelta2, id);
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
					m_switchTwoTapArmed = (l_xDelta > 0 && l_xDelta2 > 0);
					break;

				case kRight:
					m_switchTwoTapArmed = (l_xDelta < 0 && l_xDelta2 < 0);
					break;

				case kTop:
					m_switchTwoTapArmed = (l_yDelta > 0 && l_yDelta2 > 0);
					break;

				case kBottom:
					m_switchTwoTapArmed = (l_yDelta < 0 && l_yDelta2 < 0);
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
	m_switchWaitTimer = EVENTQUEUE->newOneShotTimer(m_switchWaitDelay, this);
	LOG((CLOG_DEBUG1 "waiting to switch"));
}

void
CServer::stopSwitchWait()
{
	if (m_switchWaitTimer != NULL) {
		EVENTQUEUE->deleteTimer(m_switchWaitTimer);
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
CServer::stopRelativeMoves(UInt8 id)
{
	CBaseClientProxy *l_active = m_dev->getActiveClient(id);
	if (m_dev->isRelativeMovesSet(id) && l_active != m_primaryClient) {
		// warp to the center of the active client so we know where we are
		SInt32 ax, ay, aw, ah, l_x, l_y;
		l_active->getShape(ax, ay, aw, ah);
		m_dev->getLastCursorPos(l_x, l_y, id);
		
		l_x       = ax + (aw >> 1);
		l_y       = ay + (ah >> 1);
		m_dev->setLastCursorPos(l_x, l_y, id);
		m_dev->setCursorDelta(0, 0, id);
		m_dev->setCursorDelta2(0, 0, id);
// 		m_xDelta  = 0;
// 		m_yDelta  = 0;
// 		m_xDelta2 = 0;
// 		m_yDelta2 = 0;
		LOG((CLOG_DEBUG2 "synchronize move on %s by %d,%d", getName(l_active).c_str(), l_x, l_y));
		l_active->mouseMove(l_x, l_y, id);
	}
}

void
CServer::sendOptions(CBaseClientProxy* client) const
{
	COptionsList optionsList;

	// look up options for client
	const CConfig::CScreenOptions* options =
						m_config.getOptions(getName(client));
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
	options = m_config.getOptions("");
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
  	// FIXXME CServer::processOptions: id
	UInt8 id = 2;
	
	const CConfig::CScreenOptions* options = m_config.getOptions("");
	if (options == NULL) {
		return;
	}

	bool newRelativeMoves = m_dev->isRelativeMovesSet(id);
	for (CConfig::CScreenOptions::const_iterator index = options->begin();
								index != options->end(); ++index) {
		const OptionID oId       = index->first;
		const OptionValue value = index->second;
		if (oId == kOptionScreenSwitchDelay) {
			m_switchWaitDelay = 1.0e-3 * static_cast<double>(value);
			if (m_switchWaitDelay < 0.0) {
				m_switchWaitDelay = 0.0;
			}
			stopSwitchWait();
		}
		else if (oId == kOptionScreenSwitchTwoTap) {
			m_switchTwoTapDelay = 1.0e-3 * static_cast<double>(value);
			if (m_switchTwoTapDelay < 0.0) {
				m_switchTwoTapDelay = 0.0;
			}
			stopSwitchTwoTap();
		}
		else if (oId == kOptionRelativeMouseMoves) {
			newRelativeMoves = (value != 0);
		}
	}


	if (m_dev->isRelativeMovesSet(id) && !newRelativeMoves) {
		stopRelativeMoves(id);
	}
	m_dev->setRelativeMoves(newRelativeMoves, id);
}

void
CServer::handleShapeChanged(const CEvent&, void* vclient)
{
	// ignore events from unknown clients
	CBaseClientProxy* client = reinterpret_cast<CBaseClientProxy*>(vclient);
	if (m_clientSet.count(client) == 0) {
		return;
	}
	LOG((CLOG_INFO "screen \"%s\" shape changed", getName(client).c_str()));

	// update jump coordinate
	SInt32 x, y;
	std::list<UInt8> ptrIDs;
	std::list<UInt8>::iterator i;
	m_dev->getAllPointerIDs(ptrIDs);
	for(i=ptrIDs.begin(); i != ptrIDs.end(); ++i)
	{
	    CBaseClientProxy *l_active = m_dev->getActiveClient(*i);
	    client->getCursorPos(x, y, *i);  
	    LOG((CLOG_DEBUG "handleShapeChanged dev(%d) x(%d),y(%d)", *i, x, y));
	    m_dev->setJumpCursorPos(client->getName(), x, y, *i);

	    // update the mouse coordinates
	    if (client == l_active) {
		LOG((CLOG_DEBUG2 "updating mouse coordinates"));
		m_dev->setLastCursorPos(x, y, *i);
	    }

	    // handle resolution change to primary screen
	    if (client == m_primaryClient) {
		if (client == l_active) {
			onMouseMovePrimary(x, y, *i);
		}
		else {
		    onMouseMoveSecondary(0, 0, *i);
		}
	    }
	    LOG((CLOG_DEBUG2 "Done handleShapeChanged"));
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
	CClipboardInfo& clipboard = m_clipboards[info->m_cId];
	if (grabber != m_primaryClient &&
		info->m_sequenceNumber < clipboard.m_clipboardSeqNum) {
		LOG((CLOG_INFO "ignored screen \"%s\" grab of clipboard %d", getName(grabber).c_str(), info->m_cId));
		return;
	}

	// mark screen as owning clipboard
	LOG((CLOG_INFO "screen \"%s\" grabbed clipboard %d from \"%s\"", getName(grabber).c_str(), info->m_cId, clipboard.m_clipboardOwner.c_str()));
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
			client->setClipboardDirty(info->m_cId, false);
		}
		else {
			client->grabClipboard(info->m_cId);
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
	onClipboardChanged(sender, info->m_cId, info->m_sequenceNumber);
}

void
CServer::handleKeyDownEvent(const CEvent& event, void*)
{
	IPlatformScreen::CKeyInfo* info =
		reinterpret_cast<IPlatformScreen::CKeyInfo*>(event.getData());
	onKeyDown(info->m_key, info->m_mask, info->m_button, info->m_screens, info->m_id);
}

void
CServer::handleKeyUpEvent(const CEvent& event, void*)
{
	IPlatformScreen::CKeyInfo* info =
		 reinterpret_cast<IPlatformScreen::CKeyInfo*>(event.getData());
	onKeyUp(info->m_key, info->m_mask, info->m_button, info->m_screens, info->m_id);
}

void
CServer::handleKeyRepeatEvent(const CEvent& event, void*)
{
	IPlatformScreen::CKeyInfo* info =
		reinterpret_cast<IPlatformScreen::CKeyInfo*>(event.getData());
	onKeyRepeat(info->m_key, info->m_mask, info->m_count, info->m_button, info->m_id);
}

void
CServer::handleButtonDownEvent(const CEvent& event, void*)
{
	IPlatformScreen::CButtonInfo* info =
		reinterpret_cast<IPlatformScreen::CButtonInfo*>(event.getData());
	onMouseDown(info->m_button, info->m_id);
}

void
CServer::handleButtonUpEvent(const CEvent& event, void*)
{
	IPlatformScreen::CButtonInfo* info =
		reinterpret_cast<IPlatformScreen::CButtonInfo*>(event.getData());
	onMouseUp(info->m_button, info->m_id);
}

void
CServer::handleMotionPrimaryEvent(const CEvent& event, void*)
{
	IPlatformScreen::CMotionInfo* info =
		reinterpret_cast<IPlatformScreen::CMotionInfo*>(event.getData());
	onMouseMovePrimary(info->m_x, info->m_y, info->m_id);
}

void
CServer::handleMotionSecondaryEvent(const CEvent& event, void*)
{
	IPlatformScreen::CMotionInfo* info =
		reinterpret_cast<IPlatformScreen::CMotionInfo*>(event.getData());
	onMouseMoveSecondary(info->m_x, info->m_y, info->m_id);
}

void
CServer::handleWheelEvent(const CEvent& event, void*)
{
	IPlatformScreen::CWheelInfo* info =
		reinterpret_cast<IPlatformScreen::CWheelInfo*>(event.getData());
	onMouseWheel(info->m_xDelta, info->m_yDelta, info->m_id);
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
	// FIXXME CServer::handleSwitchWaitTimeout: id
	UInt8 id = 2;
	if (isLockedToScreen(id)) {
		LOG((CLOG_DEBUG1 "locked to screen"));
		stopSwitch();
		return;
	}

	// switch screen
	switchScreen(m_switchScreen, m_switchWaitX, m_switchWaitY, id, false);
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
	// FIXXME handleSwitchToScreenEvent: id
	UInt8 id = 2;
	if (index == m_clients.end()) {
		LOG((CLOG_DEBUG1 "screen \"%s\" not active", info->m_screen));
	}
	else {
		jumpToScreen(index->second, id);
	}
}

void
CServer::handleSwitchInDirectionEvent(const CEvent& event, void*)
{
	CSwitchInDirectionInfo* info = 
		reinterpret_cast<CSwitchInDirectionInfo*>(event.getData());

	// jump to screen in chosen direction from center of this screen
	// FIXXME handleSwitchInDirectionEvent: id
	SInt32 x, y;
	UInt8 id = 2;
	CBaseClientProxy *l_active = m_dev->getActiveClient(id);
	m_dev->getLastCursorPos(x, y, id);
	CBaseClientProxy* newScreen =
		getNeighbor(l_active, info->m_direction, x, y);
	if (newScreen == NULL) {
		LOG((CLOG_DEBUG1 "no neighbor %s", CConfig::dirName(info->m_direction)));
	}
	else {
		jumpToScreen(newScreen, id);
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

	// FIXXME CServer::handleLockCursorToScreenEvent: id
	// FIXXXME CServer::handleLockCursorToScreenEvent: adjust CLockCursorToScreenInfo data
	UInt8 id = 2;
	
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
		if (!isLockedToScreenServer(id)) {
			stopRelativeMoves(id);
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
CServer::onClipboardChanged(CBaseClientProxy* sender,
				ClipboardID cId, UInt32 seqNum)
{
	CClipboardInfo& clipboard = m_clipboards[cId];
	// FIXXME CServer::onScreensaver: id
	UInt8 id = 2;	
	CBaseClientProxy *l_active = m_dev->getActiveClient(id);
	
	// ignore update if sequence number is old
	if (seqNum < clipboard.m_clipboardSeqNum) {
		LOG((CLOG_INFO "ignored screen \"%s\" update of clipboard %d (missequenced)", getName(sender).c_str(), cId));
		return;
	}

	// should be the expected client
	assert(sender == m_clients.find(clipboard.m_clipboardOwner)->second);

	// get data
	sender->getClipboard(cId, &clipboard.m_clipboard);

	// ignore if data hasn't changed
	CString data = clipboard.m_clipboard.marshall();
	if (data == clipboard.m_clipboardData) {
		LOG((CLOG_DEBUG "ignored screen \"%s\" update of clipboard %d (unchanged)", clipboard.m_clipboardOwner.c_str(), cId));
		return;
	}

	// got new data
	LOG((CLOG_INFO "screen \"%s\" updated clipboard %d", clipboard.m_clipboardOwner.c_str(), cId));
	clipboard.m_clipboardData = data;

	// tell all clients except the sender that the clipboard is dirty
	for (CClientList::const_iterator index = m_clients.begin();
								index != m_clients.end(); ++index) {
		CBaseClientProxy* client = index->second;
		client->setClipboardDirty(cId, client != sender);
	}

	// send the new clipboard to the active screen
	l_active->setClipboard(cId, &clipboard.m_clipboard);
}

void
CServer::onScreensaver(bool activated)
{
	LOG((CLOG_DEBUG "onScreenSaver %s", activated ? "activated" : "deactivated"));

	// FIXXME CServer::onScreensaver: id
	UInt8 id = 2;
	CBaseClientProxy *l_active = m_dev->getActiveClient(id);
	m_dev->getLastCursorPos(m_xSaver, m_ySaver, id);

	if (activated) {
		// save current screen and position
		m_activeSaver = l_active;

		// jump to primary screen
		if (l_active != m_primaryClient) {
			switchScreen(m_primaryClient, 0, 0, id, true);
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
			switchScreen(screen, m_xSaver, m_ySaver, id, false);
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
CServer::onKeyDown(KeyID kId, KeyModifierMask mask, KeyButton button,
				const char* screens, UInt8 id)
{
	LOG((CLOG_DEBUG1 "onKeyDown dev=%d kId=%d mask=0x%04x button=0x%04x", id, kId, mask, button));
	CBaseClientProxy *l_active = m_dev->getActiveClient(id);
	assert(l_active != NULL);

	// relay
	if (!m_keyboardBroadcasting ||
			(screens && IKeyState::CKeyInfo::isDefault(screens))) {
		l_active->keyDown(kId, mask, button, id);
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
				index->second->keyDown(kId, mask, button, id);
			}
		}
	}
}

void
CServer::onKeyUp(KeyID kId, KeyModifierMask mask, KeyButton button,
				const char* screens, UInt8 id)
{
	LOG((CLOG_DEBUG1 "onKeyUp dev=%d kId=%d mask=0x%04x button=0x%04x", id, kId, mask, button));
	CBaseClientProxy *l_active = m_dev->getActiveClient(id);
	assert(l_active != NULL);

	// relay
	if (!m_keyboardBroadcasting ||
			(screens && IKeyState::CKeyInfo::isDefault(screens))) {
		l_active->keyUp(kId, mask, button, id);
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
				index->second->keyUp(kId, mask, button, id);
			}
		}
	}
}

void
CServer::onKeyRepeat(KeyID kId, KeyModifierMask mask,
				SInt32 count, KeyButton button, UInt8 id)
{
	LOG((CLOG_DEBUG1 "onKeyRepeat dev=%d kId=%d mask=0x%04x count=%d button=0x%04x", id, kId, mask, count, button));
	CBaseClientProxy *l_active = m_dev->getActiveClient(id);
	assert(l_active != NULL);

	// relay
	l_active->keyRepeat(kId, mask, count, button, id);
}

void
CServer::onMouseDown(ButtonID bId, UInt8 id)
{
	LOG((CLOG_DEBUG1 "onMouseDown button id=%d", bId));
	CBaseClientProxy *l_active = m_dev->getActiveClient(id);
	assert(l_active != NULL);

	// relay
	l_active->mouseDown(bId, id);
}

void
CServer::onMouseUp(ButtonID bId, UInt8 id)
{
	LOG((CLOG_DEBUG1 "onMouseUp button id=%d", bId));
	CBaseClientProxy *l_active = m_dev->getActiveClient(id);
	assert(l_active != NULL);

	// relay
	l_active->mouseUp(bId, id);
}

bool
CServer::onMouseMovePrimary(SInt32 x, SInt32 y, UInt8 id)
{
	LOG((CLOG_DEBUG2 "onMouseMovePrimary id(%d) x(%d),y(%d)", id, x, y));
	CBaseClientProxy *l_active = m_dev->getActiveClient(id);

	// mouse move on primary (server's) screen
	if (l_active != m_primaryClient) {
		// stale event -- we're actually on a secondary screen
		return false;
	}

	SInt32 l_x, l_y, tmpDelta_x, tmpDelta_y;
	m_dev->getLastCursorPos(l_x, l_y, id);

	// save last delta
	m_dev->getCursorDelta(tmpDelta_x, tmpDelta_y, id);
	m_dev->setCursorDelta2(tmpDelta_x,tmpDelta_y, id);
// 	m_xDelta2 = m_xDelta;
// 	m_yDelta2 = m_yDelta;

	// save current delta
	m_dev->setCursorDelta(x - l_x, y - l_y, id);
// 	m_xDelta  = x - l_x;
// 	m_yDelta  = y - l_y;

	// save position
	m_dev->setLastCursorPos(x, y, id);

	// get screen shape
	SInt32 ax, ay, aw, ah;
	l_active->getShape(ax, ay, aw, ah);
	SInt32 zoneSize = getJumpZoneSize(l_active);

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
		noSwitch(x, y, id);
		return false;
	}

	// get jump destination
	CBaseClientProxy* newScreen = mapToNeighbor(m_active, dir, x, y, id);

	// should we switch or not?
	if (isSwitchOkay(newScreen, dir, x, y, id, xc, yc)) {
		// switch screen
		switchScreen(newScreen, x, y, id, false);
		return true;
	}
	else {
		return false;
	}
}

void
CServer::onMouseMoveSecondary(SInt32 dx, SInt32 dy, UInt8 id)
{
	LOG((CLOG_DEBUG2 "onMouseMoveSecondary id(%d) x(%d),y(%d)", id, dx, dy));
	CBaseClientProxy *l_active = m_dev->getActiveClient(id);

	if(!l_active)
	  return;
	  
	// mouse move on secondary (client's) screen
	//assert(l_active != NULL);
	if (l_active == m_primaryClient) {
		// stale event -- we're actually on the primary screen
		return;
	}
	LOG((CLOG_DEBUG2 "we move on secondary"));

	// if doing relative motion on secondary screens and we're locked
	// to the screen (which activates relative moves) then send a
	// relative mouse motion.  when we're doing this we pretend as if
	// the mouse isn't actually moving because we're expecting some
	// program on the secondary screen to warp the mouse on us, so we
	// have no idea where it really is.
	if (m_dev->isRelativeMovesSet(id) && isLockedToScreenServer(id)) {
		LOG((CLOG_DEBUG2 "relative move by dev(%d) on %s by %d,%d", id, getName(l_active).c_str(), dx, dy));
		l_active->mouseRelativeMove(dx, dy, id);
		return;
	}

	SInt32 l_x, l_y, tmpDelta_x, tmpDelta_y;
	m_dev->getLastCursorPos(l_x, l_y, id);
	// save old position
	const SInt32 xOld = l_x;
	const SInt32 yOld = l_y;

	// save last delta
	m_dev->getCursorDelta(tmpDelta_x, tmpDelta_y, id);
	m_dev->setCursorDelta2(tmpDelta_x,tmpDelta_y, id);
// 	m_xDelta2 = m_xDelta;
// 	m_yDelta2 = m_yDelta;

	// save current delta
	m_dev->setCursorDelta(dx, dy, id);
// 	m_xDelta  = dx;
// 	m_yDelta  = dy;

	// accumulate motion
	l_x      += dx;
	l_y      += dy;
	m_dev->setLastCursorPos(l_x, l_y, id);

	// get screen shape
	SInt32 ax, ay, aw, ah;
	LOG((CLOG_DEBUG2 "l_active(%d)->getShape", id));
	l_active->getShape(ax, ay, aw, ah);
	LOG((CLOG_DEBUG2 "got shape: x:%d,y:%d,h:%d,w:%d", ax, ay, aw, ah));

	// find direction of neighbor and get the neighbor
	bool jump = true;
	CBaseClientProxy* newScreen;
	do {
		// clamp position to screen
		SInt32 xc = l_x, yc = l_y;
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
		if (l_x < ax) {
			dir = kLeft;
		}
		else if (l_x > ax + aw - 1) {
			dir = kRight;
		}
		else if (l_y < ay) {
			dir = kTop;
		}
		else if (l_y > ay + ah - 1) {
			dir = kBottom;
		}
		else {
			// we haven't left the screen
			newScreen = l_active;
			jump      = false;

			// if waiting and mouse is not on the border we're waiting
			// on then stop waiting.  also if it's not on the border
			// then arm the double tap.
			if (m_switchScreen != NULL) {
				bool clearWait;
				SInt32 zoneSize = m_primaryClient->getJumpZoneSize();
				switch (m_switchDir) {
				case kLeft:
					clearWait = (l_x >= ax + zoneSize);
					break;

				case kRight:
					clearWait = (l_x <= ax + aw - 1 - zoneSize);
					break;

				case kTop:
					clearWait = (l_y >= ay + zoneSize);
					break;

				case kBottom:
					clearWait = (l_y <= ay + ah - 1 + zoneSize);
					break;

				default:
					clearWait = false;
					break;
				}
				if (clearWait) {
					// still on local screen
					noSwitch(l_x, l_y, id);
				}
			}

			// skip rest of block
			break;
		}
		LOG((CLOG_DEBUG2 "mapping to neighbour"));
		// try to switch screen.  get the neighbor.
		newScreen = mapToNeighbor(l_active, dir, l_x, l_y, id);

		// see if we should switch
		if (!isSwitchOkay(newScreen, dir, l_x, l_y, id, xc, yc)) {
			newScreen = l_active;
			jump      = false;
		}
	} while (false);

	if (jump) {
		// switch screens
		switchScreen(newScreen, l_x, l_y, id, false);
	}
	else {
		// same screen.  clamp mouse to edge.
		l_x = xOld + dx;
		l_y = yOld + dy;
		m_dev->setLastCursorPos(l_x, l_y, id);

		if (l_x < ax) {
			l_x = ax;
			LOG((CLOG_DEBUG2 "clamp to left of \"%s\"", getName(l_active).c_str()));
			m_dev->setLastCursorPos(l_x, l_y, id);

		}
		else if (l_x > ax + aw - 1) {
			l_x = ax + aw - 1;
			LOG((CLOG_DEBUG2 "clamp to right of \"%s\"", getName(l_active).c_str()));
			m_dev->setLastCursorPos(l_x, l_y, id);
		}
		if (l_y < ay) {
			l_y = ay;
			LOG((CLOG_DEBUG2 "clamp to top of \"%s\"", getName(l_active).c_str()));
			m_dev->setLastCursorPos(l_x, l_y, id);
		}
		else if (l_y > ay + ah - 1) {
			l_y = ay + ah - 1;
			LOG((CLOG_DEBUG2 "clamp to bottom of \"%s\"", getName(l_active).c_str()));
			m_dev->setLastCursorPos(l_x, l_y, id);
		}

		// warp cursor if it moved.
		if (l_x != xOld || l_y != yOld) {
			LOG((CLOG_DEBUG2 "move on %s to %d,%d", getName(l_active).c_str(), l_x, l_y));
			l_active->mouseMove(l_x, l_y, id);
		}
	}
}

void
CServer::onMouseWheel(SInt32 xDelta, SInt32 yDelta, UInt8 id)
{
	LOG((CLOG_DEBUG1 "onMouseWheel %+d,%+d", xDelta, yDelta));
	CBaseClientProxy *l_active = m_dev->getActiveClient(id);
	assert(l_active != NULL);

	// relay
	l_active->mouseWheel(xDelta, yDelta, id);
}

bool
CServer::addClient(CBaseClientProxy* client)
{
	CString name = getName(client);
	if (m_clients.count(name) != 0) {
		return false;
	}
	// FIXXME CServer::addClient: id
	UInt8 id = 2;

	// add event handlers
	EVENTQUEUE->adoptHandler(IScreen::getShapeChangedEvent(),
							client->getEventTarget(),
							new TMethodEventJob<CServer>(this,
								&CServer::handleShapeChanged, client));
	EVENTQUEUE->adoptHandler(IScreen::getClipboardGrabbedEvent(),
							client->getEventTarget(),
							new TMethodEventJob<CServer>(this,
								&CServer::handleClipboardGrabbed, client));
	EVENTQUEUE->adoptHandler(CClientProxy::getClipboardChangedEvent(),
							client->getEventTarget(),
							new TMethodEventJob<CServer>(this,
								&CServer::handleClipboardChanged, client));

	// add to list
	m_clientSet.insert(client);
	m_clients.insert(std::make_pair(name, client));

	// initialize client data
	SInt32 x, y;
	client->getCursorPos(x, y, id);
	// FIXXXME addClient: need to do jump coords in array form per client
	m_dev->setJumpCursorPos(client->getName(), x, y, id);

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
	EVENTQUEUE->removeHandler(IScreen::getShapeChangedEvent(),
							client->getEventTarget());
	EVENTQUEUE->removeHandler(IScreen::getClipboardGrabbedEvent(),
							client->getEventTarget());
	EVENTQUEUE->removeHandler(CClientProxy::getClipboardChangedEvent(),
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
	CEventQueueTimer* timer = EVENTQUEUE->newOneShotTimer(timeout, NULL);
	EVENTQUEUE->adoptHandler(CEvent::kTimer, timer,
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
		EVENTQUEUE->removeHandler(CClientProxy::getDisconnectedEvent(), client);
		if (m_clients.size() == 1 && m_oldClients.empty()) {
			EVENTQUEUE->addEvent(CEvent(getDisconnectedEvent(), this));
		}
	}
}

void
CServer::removeOldClient(CBaseClientProxy* client)
{
	COldClients::iterator i = m_oldClients.find(client);
	if (i != m_oldClients.end()) {
		EVENTQUEUE->removeHandler(CClientProxy::getDisconnectedEvent(), client);
		EVENTQUEUE->removeHandler(CEvent::kTimer, i->second);
		EVENTQUEUE->deleteTimer(i->second);
		m_oldClients.erase(i);
		if (m_clients.size() == 1 && m_oldClients.empty()) {
			EVENTQUEUE->addEvent(CEvent(getDisconnectedEvent(), this));
		}
	}
}

void
CServer::forceLeaveClient(CBaseClientProxy* client)
{
	CBaseClientProxy* active =
		(m_activeSaver != NULL) ? m_activeSaver : m_active;
	//FIXXME CServer::forceLeaveClient: id
	UInt8 id = 2;	
	SInt32 l_x, l_y;
	CBaseClientProxy *l_active = m_dev->getActiveClient(id);
	
	m_dev->getLastCursorPos(l_x, l_y, id);
	
	if (active == client) {
		// record new position (center of primary screen)
		m_primaryClient->getCursorCenter(l_x, l_y);
		m_dev->setLastCursorPos(l_x, l_y, id);

		// stop waiting to switch to this client
		if (active == m_switchScreen) {
			stopSwitch();
		}

		// don't notify active screen since it has probably already
		// disconnected.
		LOG((CLOG_INFO "jump by dev(%d) from \"%s\" to \"%s\" at %d,%d", id, getName(active).c_str(), getName(m_primaryClient).c_str(), l_x, l_y));

		// cut over
		m_dev->setActiveClient(m_primaryClient, id);
		m_dev->setActiveClient(m_primaryClient, m_dev->getAttachment(id));

		// enter new screen (unless we already have because of the
		// screen saver)
		if (m_activeSaver == NULL) {
			m_primaryClient->enter(l_x, l_y, m_seqNum, m_primaryClient->getToggleMask(id), false, m_dev->getAttachment(id), id);
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
// CServer::CScreenConnectedInfo
//

CServer::CScreenConnectedInfo*
CServer::CScreenConnectedInfo::alloc(const CString& screen)
{
	CScreenConnectedInfo* info =
		(CScreenConnectedInfo*)malloc(sizeof(CScreenConnectedInfo) +
								screen.size());
	strcpy(info->m_screen, screen.c_str());
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
