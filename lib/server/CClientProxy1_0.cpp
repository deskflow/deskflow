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

#include "CClientProxy1_0.h"
#include "CServer.h"
#include "CClipboard.h"
#include "CProtocolUtil.h"
#include "XSynergy.h"
#include "IStream.h"
#include "CLock.h"
#include "CLog.h"
#include "IEventQueue.h"
#include "TMethodEventJob.h"
#include <cstring>

//
// CClientProxy1_0
//

CClientProxy1_0::CClientProxy1_0(IServer* server,
				const CString& name, IStream* stream) :
	CClientProxy(server, name, stream),
	m_heartbeatAlarm(kHeartRate * kHeartBeatsUntilDeath),
	m_heartbeatTimer(NULL)
{
	for (UInt32 i = 0; i < kClipboardEnd; ++i) {
		m_clipboardDirty[i] = true;
	}

	// install event handlers
	EVENTQUEUE->adoptHandler(IStream::getInputReadyEvent(),
							stream->getEventTarget(),
							new TMethodEventJob<CClientProxy1_0>(this,
								&CClientProxy1_0::handleData, NULL));
	EVENTQUEUE->adoptHandler(IStream::getOutputErrorEvent(),
							stream->getEventTarget(),
							new TMethodEventJob<CClientProxy1_0>(this,
								&CClientProxy1_0::handleWriteError, NULL));
	EVENTQUEUE->adoptHandler(IStream::getInputShutdownEvent(),
							stream->getEventTarget(),
							new TMethodEventJob<CClientProxy1_0>(this,
								&CClientProxy1_0::handleDisconnect, NULL));
	EVENTQUEUE->adoptHandler(IStream::getOutputShutdownEvent(),
							stream->getEventTarget(),
							new TMethodEventJob<CClientProxy1_0>(this,
								&CClientProxy1_0::handleWriteError, NULL));
	EVENTQUEUE->adoptHandler(CEvent::kTimer, this,
							new TMethodEventJob<CClientProxy1_0>(this,
								&CClientProxy1_0::handleFlatline, NULL));

	// FIXME -- open() replacement must install initial heartbeat timer
}

CClientProxy1_0::~CClientProxy1_0()
{
	removeHandlers();
}

void
CClientProxy1_0::disconnect()
{
	CLock lock(getMutex());
	removeHandlers();
	// FIXME -- send disconnect event (server should be listening for this)
	getStream()->flush();
	getStream()->close();
}

void
CClientProxy1_0::removeHandlers()
{
	// uninstall event handlers
	EVENTQUEUE->removeHandler(IStream::getInputReadyEvent(),
							getStream()->getEventTarget());
	EVENTQUEUE->removeHandler(IStream::getOutputErrorEvent(),
							getStream()->getEventTarget());
	EVENTQUEUE->removeHandler(IStream::getInputShutdownEvent(),
							getStream()->getEventTarget());
	EVENTQUEUE->removeHandler(IStream::getOutputShutdownEvent(),
							getStream()->getEventTarget());
	EVENTQUEUE->removeHandler(CEvent::kTimer, this);

	// remove timer
	removeHeartbeatTimer();
}

void
CClientProxy1_0::addHeartbeatTimer()
{
	CLock lock(getMutex());
	if (m_heartbeatAlarm > 0.0) {
		m_heartbeatTimer = EVENTQUEUE->newOneShotTimer(m_heartbeatAlarm, this);
	}
}

void
CClientProxy1_0::removeHeartbeatTimer()
{
	CLock lock(getMutex());
	if (m_heartbeatTimer != NULL) {
		EVENTQUEUE->deleteTimer(m_heartbeatTimer);
		m_heartbeatTimer = NULL;
	}
}

void
CClientProxy1_0::handleData(const CEvent&, void*)
{
	// handle messages until there are no more.  first read message code.
	UInt8 code[4];
	UInt32 n = getStream()->read(code, 4);
	while (n != 0) {
		// verify we got an entire code
		if (n != 4) {
			LOG((CLOG_ERR "incomplete message from \"%s\": %d bytes", getName().c_str(), n));
			disconnect();
			return;
		}

		// parse message
		LOG((CLOG_DEBUG2 "msg from \"%s\": %c%c%c%c", getName().c_str(), code[0], code[1], code[2], code[3]));
		if (!parseMessage(code)) {
			LOG((CLOG_ERR "invalid message from client \"%s\"", getName().c_str()));
			disconnect();
			return;
		}

		// next message
		n = getStream()->read(code, 4);
	}

	// restart heartbeat timer
	removeHeartbeatTimer();
	addHeartbeatTimer();
}

bool
CClientProxy1_0::parseMessage(const UInt8* code)
{
	if (memcmp(code, kMsgDInfo, 4) == 0) {
		return recvInfo(true);
	}
	else if (memcmp(code, kMsgCNoop, 4) == 0) {
		// discard no-ops
		LOG((CLOG_DEBUG2 "no-op from", getName().c_str()));
		return true;
	}
	else if (memcmp(code, kMsgCClipboard, 4) == 0) {
		return recvGrabClipboard();
	}
	else if (memcmp(code, kMsgDClipboard, 4) == 0) {
		return recvClipboard();
	}
	return false;
}

void
CClientProxy1_0::handleDisconnect(const CEvent&, void*)
{
	LOG((CLOG_NOTE "client \"%s\" disconnected", getName().c_str()));
	disconnect();
}

void
CClientProxy1_0::handleWriteError(const CEvent&, void*)
{
	LOG((CLOG_ERR "error writing to client \"%s\"", getName().c_str()));
	disconnect();
}

void
CClientProxy1_0::handleFlatline(const CEvent&, void*)
{
	// didn't get a heartbeat fast enough.  assume client is dead.
	LOG((CLOG_NOTE "client \"%s\" is dead", getName().c_str()));
	disconnect();
}

// FIXME -- replace this
void
CClientProxy1_0::open()
{
	// send request
	LOG((CLOG_DEBUG1 "querying client \"%s\" info", getName().c_str()));
	CProtocolUtil::writef(getStream(), kMsgQInfo);
	getStream()->flush();

	// wait for and verify reply
	UInt8 code[4];
	for (;;) {
		UInt32 n = getStream()->read(code, 4);
		if (n == 4) {
			if (memcmp(code, kMsgCNoop, 4) == 0) {
				// discard heartbeats
				continue;
			}
			if (memcmp(code, kMsgDInfo, 4) == 0) {
				break;
			}
		}
		throw XBadClient();
	}

	// handle reply
	recvInfo(false);
}

// FIXME -- replace this
void
CClientProxy1_0::close()
{
	LOG((CLOG_DEBUG1 "send close to \"%s\"", getName().c_str()));
	CProtocolUtil::writef(getStream(), kMsgCClose);

	// force the close to be sent before we return
	getStream()->flush();
}

void
CClientProxy1_0::enter(SInt32 xAbs, SInt32 yAbs,
				UInt32 seqNum, KeyModifierMask mask, bool)
{
	LOG((CLOG_DEBUG1 "send enter to \"%s\", %d,%d %d %04x", getName().c_str(), xAbs, yAbs, seqNum, mask));
	CProtocolUtil::writef(getStream(), kMsgCEnter,
								xAbs, yAbs, seqNum, mask);
}

bool
CClientProxy1_0::leave()
{
	LOG((CLOG_DEBUG1 "send leave to \"%s\"", getName().c_str()));
	CProtocolUtil::writef(getStream(), kMsgCLeave);

	// we can never prevent the user from leaving
	return true;
}

void
CClientProxy1_0::setClipboard(ClipboardID id, const CString& data)
{
	// ignore if this clipboard is already clean
	CLock lock(getMutex());
	if (m_clipboardDirty[id]) {
		// this clipboard is now clean
		m_clipboardDirty[id] = false;

		LOG((CLOG_DEBUG "send clipboard %d to \"%s\" size=%d", id, getName().c_str(), data.size()));
		CProtocolUtil::writef(getStream(), kMsgDClipboard, id, 0, &data);
	}
}

void
CClientProxy1_0::grabClipboard(ClipboardID id)
{
	LOG((CLOG_DEBUG "send grab clipboard %d to \"%s\"", id, getName().c_str()));
	CProtocolUtil::writef(getStream(), kMsgCClipboard, id, 0);

	// this clipboard is now dirty
	CLock lock(getMutex());
	m_clipboardDirty[id] = true;
}

void
CClientProxy1_0::setClipboardDirty(ClipboardID id, bool dirty)
{
	CLock lock(getMutex());
	m_clipboardDirty[id] = dirty;
}

void
CClientProxy1_0::keyDown(KeyID key, KeyModifierMask mask, KeyButton)
{
	LOG((CLOG_DEBUG1 "send key down to \"%s\" id=%d, mask=0x%04x", getName().c_str(), key, mask));
	CProtocolUtil::writef(getStream(), kMsgDKeyDown1_0, key, mask);
}

void
CClientProxy1_0::keyRepeat(KeyID key, KeyModifierMask mask,
				SInt32 count, KeyButton)
{
	LOG((CLOG_DEBUG1 "send key repeat to \"%s\" id=%d, mask=0x%04x, count=%d", getName().c_str(), key, mask, count));
	CProtocolUtil::writef(getStream(), kMsgDKeyRepeat1_0, key, mask, count);
}

void
CClientProxy1_0::keyUp(KeyID key, KeyModifierMask mask, KeyButton)
{
	LOG((CLOG_DEBUG1 "send key up to \"%s\" id=%d, mask=0x%04x", getName().c_str(), key, mask));
	CProtocolUtil::writef(getStream(), kMsgDKeyUp1_0, key, mask);
}

void
CClientProxy1_0::mouseDown(ButtonID button)
{
	LOG((CLOG_DEBUG1 "send mouse down to \"%s\" id=%d", getName().c_str(), button));
	CProtocolUtil::writef(getStream(), kMsgDMouseDown, button);
}

void
CClientProxy1_0::mouseUp(ButtonID button)
{
	LOG((CLOG_DEBUG1 "send mouse up to \"%s\" id=%d", getName().c_str(), button));
	CProtocolUtil::writef(getStream(), kMsgDMouseUp, button);
}

void
CClientProxy1_0::mouseMove(SInt32 xAbs, SInt32 yAbs)
{
	LOG((CLOG_DEBUG2 "send mouse move to \"%s\" %d,%d", getName().c_str(), xAbs, yAbs));
	CProtocolUtil::writef(getStream(), kMsgDMouseMove, xAbs, yAbs);
}

void
CClientProxy1_0::mouseWheel(SInt32 delta)
{
	LOG((CLOG_DEBUG2 "send mouse wheel to \"%s\" %+d", getName().c_str(), delta));
	CProtocolUtil::writef(getStream(), kMsgDMouseWheel, delta);
}

void
CClientProxy1_0::screensaver(bool on)
{
	LOG((CLOG_DEBUG1 "send screen saver to \"%s\" on=%d", getName().c_str(), on ? 1 : 0));
	CProtocolUtil::writef(getStream(), kMsgCScreenSaver, on ? 1 : 0);
}

void
CClientProxy1_0::resetOptions()
{
	LOG((CLOG_DEBUG1 "send reset options to \"%s\"", getName().c_str()));
	CProtocolUtil::writef(getStream(), kMsgCResetOptions);

	// reset heart rate and death
	CLock lock(getMutex());
	m_heartbeatAlarm = kHeartRate * kHeartBeatsUntilDeath;
	removeHeartbeatTimer();
	addHeartbeatTimer();
}

void
CClientProxy1_0::setOptions(const COptionsList& options)
{
	LOG((CLOG_DEBUG1 "send set options to \"%s\" size=%d", getName().c_str(), options.size()));
	CProtocolUtil::writef(getStream(), kMsgDSetOptions, &options);

	// check options
	CLock lock(getMutex());
	for (UInt32 i = 0, n = options.size(); i < n; i += 2) {
		if (options[i] == kOptionHeartbeat) {
			double rate = 1.0e-3 * static_cast<double>(options[i + 1]);
			if (rate <= 0.0) {
				rate = -1.0;
			}
			m_heartbeatAlarm = rate * kHeartBeatsUntilDeath;
			removeHeartbeatTimer();
			addHeartbeatTimer();
		}
	}
}

SInt32
CClientProxy1_0::getJumpZoneSize() const
{
	CLock lock(getMutex());
	return m_info.m_zoneSize;
}

void
CClientProxy1_0::getShape(SInt32& x, SInt32& y, SInt32& w, SInt32& h) const
{
	CLock lock(getMutex());
	x = m_info.m_x;
	y = m_info.m_y;
	w = m_info.m_w;
	h = m_info.m_h;
}

void
CClientProxy1_0::getCursorPos(SInt32&, SInt32&) const
{
	assert(0 && "shouldn't be called");
}

void
CClientProxy1_0::getCursorCenter(SInt32& x, SInt32& y) const
{
	CLock lock(getMutex());
	x = m_info.m_mx;
	y = m_info.m_my;
}

bool
CClientProxy1_0::recvInfo(bool notify)
{
	{
		CLock lock(getMutex());

		// parse the message
		SInt16 x, y, w, h, zoneSize, mx, my;
		if (!CProtocolUtil::readf(getStream(), kMsgDInfo + 4,
								&x, &y, &w, &h, &zoneSize, &mx, &my)) {
			return false;
		}
		LOG((CLOG_DEBUG "received client \"%s\" info shape=%d,%d %dx%d, zone=%d, pos=%d,%d", getName().c_str(), x, y, w, h, zoneSize, mx, my));

		// validate
		if (w <= 0 || h <= 0 || zoneSize < 0) {
			return false;
		}
		if (mx < x || my < y || mx >= x + w || my >= y + h) {
			return false;
		}

		// save
		m_info.m_x        = x;
		m_info.m_y        = y;
		m_info.m_w        = w;
		m_info.m_h        = h;
		m_info.m_zoneSize = zoneSize;
		m_info.m_mx       = mx;
		m_info.m_my       = my;
	}

	// tell server of change
	if (notify) {
		getServer()->onInfoChanged(getName(), m_info);
	}

	// acknowledge receipt
	LOG((CLOG_DEBUG1 "send info ack to \"%s\"", getName().c_str()));
	CProtocolUtil::writef(getStream(), kMsgCInfoAck);
	return true;
}

bool
CClientProxy1_0::recvClipboard()
{
	// parse message
	ClipboardID id;
	UInt32 seqNum;
	CString data;
	if (!CProtocolUtil::readf(getStream(),
							kMsgDClipboard + 4, &id, &seqNum, &data)) {
		return false;
	}
	LOG((CLOG_DEBUG "received client \"%s\" clipboard %d seqnum=%d, size=%d", getName().c_str(), id, seqNum, data.size()));

	// validate
	if (id >= kClipboardEnd) {
		return false;
	}

	// send update.  this calls us back to reset our clipboard dirty flag
	// so don't hold a lock during the call.
	getServer()->onClipboardChanged(id, seqNum, data);
	return true;
}

bool
CClientProxy1_0::recvGrabClipboard()
{
	// parse message
	ClipboardID id;
	UInt32 seqNum;
	if (!CProtocolUtil::readf(getStream(), kMsgCClipboard + 4, &id, &seqNum)) {
		return false;
	}
	LOG((CLOG_DEBUG "received client \"%s\" grabbed clipboard %d seqnum=%d", getName().c_str(), id, seqNum));

	// validate
	if (id >= kClipboardEnd) {
		return false;
	}

	// send update.  this calls us back to reset our clipboard dirty flag
	// so don't hold a lock during the call.
	getServer()->onGrabClipboard(getName(), id, seqNum);
	return true;
}
