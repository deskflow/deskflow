/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2002 Chris Schoeneman, Nick Bolton, Sorin Sbarnea
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

#include "CClientProxy1_0.h"
#include "CProtocolUtil.h"
#include "XSynergy.h"
#include "IStream.h"
#include "CLog.h"
#include "IEventQueue.h"
#include "TMethodEventJob.h"
#include <cstring>

//
// CClientProxy1_0
//

CClientProxy1_0::CClientProxy1_0(const CString& name, IStream* stream) :
	CClientProxy(name, stream),
	m_heartbeatTimer(NULL),
	m_parser(&CClientProxy1_0::parseHandshakeMessage)
{
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

	setHeartbeatRate(kHeartRate, kHeartRate * kHeartBeatsUntilDeath);

	LOG((CLOG_DEBUG1 "querying client \"%s\" info", getName().c_str()));
	CProtocolUtil::writef(getStream(), kMsgQInfo);
}

CClientProxy1_0::~CClientProxy1_0()
{
	removeHandlers();
}

void
CClientProxy1_0::disconnect()
{
	removeHandlers();
	getStream()->close();
	EVENTQUEUE->addEvent(CEvent(getDisconnectedEvent(), getEventTarget()));
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
	if (m_heartbeatAlarm > 0.0) {
		m_heartbeatTimer = EVENTQUEUE->newOneShotTimer(m_heartbeatAlarm, this);
	}
}

void
CClientProxy1_0::removeHeartbeatTimer()
{
	if (m_heartbeatTimer != NULL) {
		EVENTQUEUE->deleteTimer(m_heartbeatTimer);
		m_heartbeatTimer = NULL;
	}
}

void
CClientProxy1_0::resetHeartbeatTimer()
{
	// reset the alarm
	removeHeartbeatTimer();
	addHeartbeatTimer();
}

void
CClientProxy1_0::resetHeartbeatRate()
{
	setHeartbeatRate(kHeartRate, kHeartRate * kHeartBeatsUntilDeath);
}

void
CClientProxy1_0::setHeartbeatRate(double, double alarm)
{
	m_heartbeatAlarm = alarm;
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
		if (!(this->*m_parser)(code)) {
			LOG((CLOG_ERR "invalid message from client \"%s\": %c%c%c%c", getName().c_str(), code[0], code[1], code[2], code[3]));
			disconnect();
			return;
		}

		// next message
		n = getStream()->read(code, 4);
	}

	// restart heartbeat timer
	resetHeartbeatTimer();
}

bool
CClientProxy1_0::parseHandshakeMessage(const UInt8* code)
{
	if (memcmp(code, kMsgCNoop, 4) == 0) {
		// discard no-ops
		LOG((CLOG_DEBUG2 "no-op from", getName().c_str()));
		return true;
	}
	else if (memcmp(code, kMsgDInfo, 4) == 0) {
		// future messages get parsed by parseMessage
		m_parser = &CClientProxy1_0::parseMessage;
		if (recvInfo()) {
			EVENTQUEUE->addEvent(CEvent(getReadyEvent(), getEventTarget()));
			addHeartbeatTimer();
			return true;
		}
	}
	return false;
}

bool
CClientProxy1_0::parseMessage(const UInt8* code)
{
	if (memcmp(code, kMsgDInfo, 4) == 0) {
		if (recvInfo()) {
			EVENTQUEUE->addEvent(
							CEvent(getShapeChangedEvent(), getEventTarget()));
			return true;
		}
		return false;
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
	LOG((CLOG_NOTE "client \"%s\" has disconnected", getName().c_str()));
	disconnect();
}

void
CClientProxy1_0::handleWriteError(const CEvent&, void*)
{
	LOG((CLOG_WARN "error writing to client \"%s\"", getName().c_str()));
	disconnect();
}

void
CClientProxy1_0::handleFlatline(const CEvent&, void*)
{
	// didn't get a heartbeat fast enough.  assume client is dead.
	LOG((CLOG_NOTE "client \"%s\" is dead", getName().c_str()));
	disconnect();
}

bool
CClientProxy1_0::getClipboard(ClipboardID id, IClipboard* clipboard) const
{
	CClipboard::copy(clipboard, &m_clipboard[id].m_clipboard);
	return true;
}

void
CClientProxy1_0::getShape(SInt32& x, SInt32& y, SInt32& w, SInt32& h) const
{
	x = m_info.m_x;
	y = m_info.m_y;
	w = m_info.m_w;
	h = m_info.m_h;
}

void
CClientProxy1_0::getCursorPos(SInt32& x, SInt32& y) const
{
	// note -- this returns the cursor pos from when we last got client info
	x = m_info.m_mx;
	y = m_info.m_my;
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
CClientProxy1_0::setClipboard(ClipboardID id, const IClipboard* clipboard)
{
	// ignore if this clipboard is already clean
	if (m_clipboard[id].m_dirty) {
		// this clipboard is now clean
		m_clipboard[id].m_dirty = false;
		CClipboard::copy(&m_clipboard[id].m_clipboard, clipboard);

		CString data = m_clipboard[id].m_clipboard.marshall();
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
	m_clipboard[id].m_dirty = true;
}

void
CClientProxy1_0::setClipboardDirty(ClipboardID id, bool dirty)
{
	m_clipboard[id].m_dirty = dirty;
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
CClientProxy1_0::mouseRelativeMove(SInt32, SInt32)
{
	// ignore -- not supported in protocol 1.0
}

void
CClientProxy1_0::mouseWheel(SInt32, SInt32 yDelta)
{
	// clients prior to 1.3 only support the y axis
	LOG((CLOG_DEBUG2 "send mouse wheel to \"%s\" %+d", getName().c_str(), yDelta));
	CProtocolUtil::writef(getStream(), kMsgDMouseWheel1_0, yDelta);
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
	resetHeartbeatRate();
	removeHeartbeatTimer();
	addHeartbeatTimer();
}

void
CClientProxy1_0::setOptions(const COptionsList& options)
{
	LOG((CLOG_DEBUG1 "send set options to \"%s\" size=%d", getName().c_str(), options.size()));
	CProtocolUtil::writef(getStream(), kMsgDSetOptions, &options);

	// check options
	for (UInt32 i = 0, n = (UInt32)options.size(); i < n; i += 2) {
		if (options[i] == kOptionHeartbeat) {
			double rate = 1.0e-3 * static_cast<double>(options[i + 1]);
			if (rate <= 0.0) {
				rate = -1.0;
			}
			setHeartbeatRate(rate, rate * kHeartBeatsUntilDeath);
			removeHeartbeatTimer();
			addHeartbeatTimer();
		}
	}
}

bool
CClientProxy1_0::recvInfo()
{
	// parse the message
	SInt16 x, y, w, h, dummy1, mx, my;
	if (!CProtocolUtil::readf(getStream(), kMsgDInfo + 4,
							&x, &y, &w, &h, &dummy1, &mx, &my)) {
		return false;
	}
	LOG((CLOG_DEBUG "received client \"%s\" info shape=%d,%d %dx%d at %d,%d", getName().c_str(), x, y, w, h, mx, my));

	// validate
	if (w <= 0 || h <= 0) {
		return false;
	}
	if (mx < x || mx >= x + w || my < y || my >= y + h) {
		mx = x + w / 2;
		my = y + h / 2;
	}

	// save
	m_info.m_x  = x;
	m_info.m_y  = y;
	m_info.m_w  = w;
	m_info.m_h  = h;
	m_info.m_mx = mx;
	m_info.m_my = my;

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

	// save clipboard
	m_clipboard[id].m_clipboard.unmarshall(data, 0);
	m_clipboard[id].m_sequenceNumber = seqNum;

	// notify
	CClipboardInfo* info   = new CClipboardInfo;
	info->m_id             = id;
	info->m_sequenceNumber = seqNum;
	EVENTQUEUE->addEvent(CEvent(getClipboardChangedEvent(),
							getEventTarget(), info));

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

	// notify
	CClipboardInfo* info   = new CClipboardInfo;
	info->m_id             = id;
	info->m_sequenceNumber = seqNum;
	EVENTQUEUE->addEvent(CEvent(getClipboardGrabbedEvent(),
							getEventTarget(), info));

	return true;
}


//
// CClientProxy1_0::CClientClipboard
//

CClientProxy1_0::CClientClipboard::CClientClipboard() :
	m_clipboard(),
	m_sequenceNumber(0),
	m_dirty(true)
{
	// do nothing
}
