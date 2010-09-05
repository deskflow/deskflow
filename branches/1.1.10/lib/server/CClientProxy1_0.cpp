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
#include "IInputStream.h"
#include "IOutputStream.h"
#include "CLock.h"
#include "CThread.h"
#include "CLog.h"
#include "CStopwatch.h"
#include <cstring>

//
// CClientProxy1_0
//

CClientProxy1_0::CClientProxy1_0(IServer* server, const CString& name,
				IInputStream* input, IOutputStream* output) :
	CClientProxy(server, name, input, output),
	m_heartRate(kHeartRate),
	m_heartDeath(kHeartRate * kHeartBeatsUntilDeath)
{
	for (UInt32 i = 0; i < kClipboardEnd; ++i) {
		m_clipboardDirty[i] = true;
	}
}

CClientProxy1_0::~CClientProxy1_0()
{
	// do nothing
}

void
CClientProxy1_0::open()
{
	// send request
	LOG((CLOG_DEBUG1 "querying client \"%s\" info", getName().c_str()));
	CProtocolUtil::writef(getOutputStream(), kMsgQInfo);
	getOutputStream()->flush();

	// wait for and verify reply
	UInt8 code[4];
	for (;;) {
		UInt32 n = getInputStream()->read(code, 4, -1.0);
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

void
CClientProxy1_0::mainLoop()
{
	// handle messages until the client hangs up or stops sending heartbeats
	CStopwatch heartTimer;
	for (;;) {
		CThread::testCancel();

		// wait for a message
		UInt8 code[4];
		UInt32 n = getInputStream()->read(code, 4, m_heartRate);
		CThread::testCancel();

		// check if client hungup
		if (n == 0) {
			LOG((CLOG_NOTE "client \"%s\" disconnected", getName().c_str()));
			return;
		}

		// check if client has stopped sending heartbeats
		if (n == (UInt32)-1) {
			if (m_heartDeath >= 0.0 && heartTimer.getTime() > m_heartDeath) {
				LOG((CLOG_NOTE "client \"%s\" is dead", getName().c_str()));
				return;
			}
			continue;
		}

		// got a message so reset heartbeat monitor
		heartTimer.reset();

		// verify we got an entire code
		if (n != 4) {
			LOG((CLOG_ERR "incomplete message from \"%s\": %d bytes", getName().c_str(), n));

			// client sent an incomplete message
			throw XBadClient();
		}

		// parse message
		LOG((CLOG_DEBUG2 "msg from \"%s\": %c%c%c%c", getName().c_str(), code[0], code[1], code[2], code[3]));
		if (memcmp(code, kMsgDInfo, 4) == 0) {
			recvInfo(true);
		}
		else if (memcmp(code, kMsgCNoop, 4) == 0) {
			// discard no-ops
			LOG((CLOG_DEBUG2 "no-op from", getName().c_str()));
			continue;
		}
		else if (memcmp(code, kMsgCClipboard, 4) == 0) {
			recvGrabClipboard();
		}
		else if (memcmp(code, kMsgDClipboard, 4) == 0) {
			recvClipboard();
		}
		// note -- more message handlers go here
		else {
			LOG((CLOG_ERR "invalid message from client \"%s\"", getName().c_str()));

			// unknown message
			throw XBadClient();
		}
	}
}

void
CClientProxy1_0::close()
{
	LOG((CLOG_DEBUG1 "send close to \"%s\"", getName().c_str()));
	CProtocolUtil::writef(getOutputStream(), kMsgCClose);

	// force the close to be sent before we return
	getOutputStream()->flush();
}

void
CClientProxy1_0::enter(SInt32 xAbs, SInt32 yAbs,
				UInt32 seqNum, KeyModifierMask mask, bool)
{
	LOG((CLOG_DEBUG1 "send enter to \"%s\", %d,%d %d %04x", getName().c_str(), xAbs, yAbs, seqNum, mask));
	CProtocolUtil::writef(getOutputStream(), kMsgCEnter,
								xAbs, yAbs, seqNum, mask);
}

bool
CClientProxy1_0::leave()
{
	LOG((CLOG_DEBUG1 "send leave to \"%s\"", getName().c_str()));
	CProtocolUtil::writef(getOutputStream(), kMsgCLeave);

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
		CProtocolUtil::writef(getOutputStream(), kMsgDClipboard, id, 0, &data);
	}
}

void
CClientProxy1_0::grabClipboard(ClipboardID id)
{
	LOG((CLOG_DEBUG "send grab clipboard %d to \"%s\"", id, getName().c_str()));
	CProtocolUtil::writef(getOutputStream(), kMsgCClipboard, id, 0);

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
	CProtocolUtil::writef(getOutputStream(), kMsgDKeyDown1_0, key, mask);
}

void
CClientProxy1_0::keyRepeat(KeyID key, KeyModifierMask mask,
				SInt32 count, KeyButton)
{
	LOG((CLOG_DEBUG1 "send key repeat to \"%s\" id=%d, mask=0x%04x, count=%d", getName().c_str(), key, mask, count));
	CProtocolUtil::writef(getOutputStream(), kMsgDKeyRepeat1_0, key, mask, count);
}

void
CClientProxy1_0::keyUp(KeyID key, KeyModifierMask mask, KeyButton)
{
	LOG((CLOG_DEBUG1 "send key up to \"%s\" id=%d, mask=0x%04x", getName().c_str(), key, mask));
	CProtocolUtil::writef(getOutputStream(), kMsgDKeyUp1_0, key, mask);
}

void
CClientProxy1_0::mouseDown(ButtonID button)
{
	LOG((CLOG_DEBUG1 "send mouse down to \"%s\" id=%d", getName().c_str(), button));
	CProtocolUtil::writef(getOutputStream(), kMsgDMouseDown, button);
}

void
CClientProxy1_0::mouseUp(ButtonID button)
{
	LOG((CLOG_DEBUG1 "send mouse up to \"%s\" id=%d", getName().c_str(), button));
	CProtocolUtil::writef(getOutputStream(), kMsgDMouseUp, button);
}

void
CClientProxy1_0::mouseMove(SInt32 xAbs, SInt32 yAbs)
{
	LOG((CLOG_DEBUG2 "send mouse move to \"%s\" %d,%d", getName().c_str(), xAbs, yAbs));
	CProtocolUtil::writef(getOutputStream(), kMsgDMouseMove, xAbs, yAbs);
}

void
CClientProxy1_0::mouseWheel(SInt32 delta)
{
	LOG((CLOG_DEBUG2 "send mouse wheel to \"%s\" %+d", getName().c_str(), delta));
	CProtocolUtil::writef(getOutputStream(), kMsgDMouseWheel, delta);
}

void
CClientProxy1_0::screensaver(bool on)
{
	LOG((CLOG_DEBUG1 "send screen saver to \"%s\" on=%d", getName().c_str(), on ? 1 : 0));
	CProtocolUtil::writef(getOutputStream(), kMsgCScreenSaver, on ? 1 : 0);
}

void
CClientProxy1_0::resetOptions()
{
	LOG((CLOG_DEBUG1 "send reset options to \"%s\"", getName().c_str()));
	CProtocolUtil::writef(getOutputStream(), kMsgCResetOptions);

	// reset heart rate and death
	CLock lock(getMutex());
	m_heartRate  = kHeartRate;
	m_heartDeath = kHeartRate * kHeartBeatsUntilDeath;
}

void
CClientProxy1_0::setOptions(const COptionsList& options)
{
	LOG((CLOG_DEBUG1 "send set options to \"%s\" size=%d", getName().c_str(), options.size()));
	CProtocolUtil::writef(getOutputStream(), kMsgDSetOptions, &options);

	// check options
	CLock lock(getMutex());
	for (UInt32 i = 0, n = options.size(); i < n; i += 2) {
		if (options[i] == kOptionHeartbeat) {
			m_heartRate = 1.0e-3 * static_cast<double>(options[i + 1]);
			if (m_heartRate <= 0.0) {
				m_heartRate = -1.0;
			}
			m_heartDeath = m_heartRate * kHeartBeatsUntilDeath;
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

void
CClientProxy1_0::recvInfo(bool notify)
{
	{
		CLock lock(getMutex());

		// parse the message
		SInt16 x, y, w, h, zoneSize, mx, my;
		CProtocolUtil::readf(getInputStream(), kMsgDInfo + 4,
								&x, &y, &w, &h, &zoneSize, &mx, &my);
		LOG((CLOG_DEBUG "received client \"%s\" info shape=%d,%d %dx%d, zone=%d, pos=%d,%d", getName().c_str(), x, y, w, h, zoneSize, mx, my));

		// validate
		if (w <= 0 || h <= 0 || zoneSize < 0) {
			throw XBadClient();
		}
		if (mx < x || my < y || mx >= x + w || my >= y + h) {
			throw XBadClient();
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
	CProtocolUtil::writef(getOutputStream(), kMsgCInfoAck);
}

void
CClientProxy1_0::recvClipboard()
{
	// parse message
	ClipboardID id;
	UInt32 seqNum;
	CString data;
	CProtocolUtil::readf(getInputStream(), kMsgDClipboard + 4, &id, &seqNum, &data);
	LOG((CLOG_DEBUG "received client \"%s\" clipboard %d seqnum=%d, size=%d", getName().c_str(), id, seqNum, data.size()));

	// validate
	if (id >= kClipboardEnd) {
		throw XBadClient();
	}

	// send update.  this calls us back to reset our clipboard dirty flag
	// so don't hold a lock during the call.
	getServer()->onClipboardChanged(id, seqNum, data);
}

void
CClientProxy1_0::recvGrabClipboard()
{
	// parse message
	ClipboardID id;
	UInt32 seqNum;
	CProtocolUtil::readf(getInputStream(), kMsgCClipboard + 4, &id, &seqNum);
	LOG((CLOG_DEBUG "received client \"%s\" grabbed clipboard %d seqnum=%d", getName().c_str(), id, seqNum));

	// validate
	if (id >= kClipboardEnd) {
		throw XBadClient();
	}

	// send update.  this calls us back to reset our clipboard dirty flag
	// so don't hold a lock during the call.
	getServer()->onGrabClipboard(getName(), id, seqNum);
}
