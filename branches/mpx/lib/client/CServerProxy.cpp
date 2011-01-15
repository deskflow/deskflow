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
 */

#include "CServerProxy.h"
#include "CClient.h"
#include "CClipboard.h"
#include "CProtocolUtil.h"
#include "OptionTypes.h"
#include "ProtocolTypes.h"
#include "IStream.h"
#include "CLog.h"
#include "IEventQueue.h"
#include "TMethodEventJob.h"
#include "XBase.h"
#include <memory>
#include <cstring>

//
// CServerProxy
//

CServerProxy::CServerProxy(CClient* client, IStream* stream) :
	m_client(client),
	m_dev(CDeviceManager::getInstance()),
	m_stream(stream),
	m_seqNum(0),
	m_compressMouse(false),
	m_compressMouseRelative(false),
	m_xMouse(0),
	m_yMouse(0),
	m_dxMouse(0),
	m_dyMouse(0),
	m_ignoreMouse(false),
	m_keepAliveAlarm(0.0),
	m_keepAliveAlarmTimer(NULL),
	m_parser(&CServerProxy::parseHandshakeMessage)
{
	assert(m_client != NULL);
	assert(m_stream != NULL);

	// initialize modifier translation table
	for (KeyModifierID kId = 0; kId < kKeyModifierIDLast; ++kId)
		m_modifierTranslationTable[kId] = kId;

	// handle data on stream
	EVENTQUEUE->adoptHandler(IStream::getInputReadyEvent(),
							m_stream->getEventTarget(),
							new TMethodEventJob<CServerProxy>(this,
								&CServerProxy::handleData));

	// send heartbeat
	setKeepAliveRate(kKeepAliveRate);
}

CServerProxy::~CServerProxy()
{
	setKeepAliveRate(-1.0);
	EVENTQUEUE->removeHandler(IStream::getInputReadyEvent(),
							m_stream->getEventTarget());
}

void
CServerProxy::resetKeepAliveAlarm()
{
	if (m_keepAliveAlarmTimer != NULL) {
		EVENTQUEUE->removeHandler(CEvent::kTimer, m_keepAliveAlarmTimer);
		EVENTQUEUE->deleteTimer(m_keepAliveAlarmTimer);
		m_keepAliveAlarmTimer = NULL;
	}
	if (m_keepAliveAlarm > 0.0) {
		m_keepAliveAlarmTimer =
			EVENTQUEUE->newOneShotTimer(m_keepAliveAlarm, NULL);
		EVENTQUEUE->adoptHandler(CEvent::kTimer, m_keepAliveAlarmTimer,
							new TMethodEventJob<CServerProxy>(this,
								&CServerProxy::handleKeepAliveAlarm));
	}
}

void
CServerProxy::setKeepAliveRate(double rate)
{
	m_keepAliveAlarm = rate * kKeepAlivesUntilDeath;
	resetKeepAliveAlarm();
}

void
CServerProxy::handleData(const CEvent&, void*)
{
	// handle messages until there are no more.  first read message code.
	UInt8 code[4];
	UInt32 n = m_stream->read(code, 4);
	while (n != 0) {
		// verify we got an entire code
		if (n != 4) {
			LOG((CLOG_ERR "incomplete message from server: %d bytes", n));
			m_client->disconnect("incomplete message from server");
			return;
		}

		// parse message
		LOG((CLOG_DEBUG2 "msg from server: %c%c%c%c", code[0], code[1], code[2], code[3]));
		switch ((this->*m_parser)(code)) {
		case kOkay:
			break;

		case kUnknown:
			LOG((CLOG_ERR "invalid message from server: %c%c%c%c", code[0], code[1], code[2], code[3]));
			m_client->disconnect("invalid message from server");
			return;

		case kDisconnect:
			return;
		}

		// next message
		n = m_stream->read(code, 4);
	}

	flushCompressedMouse();
}

CServerProxy::EResult
CServerProxy::parseHandshakeMessage(const UInt8* code)
{
	if (memcmp(code, kMsgQInfo, 4) == 0) {
		queryInfo();
	}

	else if (memcmp(code, kMsgCInfoAck, 4) == 0) {
		infoAcknowledgment();
	}

	else if (memcmp(code, kMsgDSetOptions, 4) == 0) {
		setOptions();

		// handshake is complete
		m_parser = &CServerProxy::parseMessage;
		m_client->handshakeComplete();
	}

	else if (memcmp(code, kMsgCResetOptions, 4) == 0) {
		resetOptions();
	}

	else if (memcmp(code, kMsgCKeepAlive, 4) == 0) {
		// echo keep alives and reset alarm
		CProtocolUtil::writef(m_stream, kMsgCKeepAlive);
		resetKeepAliveAlarm();
	}

	else if (memcmp(code, kMsgCNoop, 4) == 0) {
		// accept and discard no-op
	}

	else if (memcmp(code, kMsgCClose, 4) == 0) {
		// server wants us to hangup
		LOG((CLOG_DEBUG1 "recv close"));
		m_client->disconnect(NULL);
		return kDisconnect;
	}

	else if (memcmp(code, kMsgEIncompatible, 4) == 0) {
		SInt32 major, minor;
		CProtocolUtil::readf(m_stream,
						kMsgEIncompatible + 4, &major, &minor);
		LOG((CLOG_ERR "server has incompatible version %d.%d", major, minor));
		m_client->disconnect("server has incompatible version");
		return kDisconnect;
	}

	else if (memcmp(code, kMsgEBusy, 4) == 0) {
		LOG((CLOG_ERR "server already has a connected client with name \"%s\"", m_client->getName().c_str()));
		m_client->disconnect("server already has a connected client with our name");
		return kDisconnect;
	}

	else if (memcmp(code, kMsgEUnknown, 4) == 0) {
		LOG((CLOG_ERR "server refused client with name \"%s\"", m_client->getName().c_str()));
		m_client->disconnect("server refused client with our name");
		return kDisconnect;
	}

	else if (memcmp(code, kMsgEBad, 4) == 0) {
		LOG((CLOG_ERR "server disconnected due to a protocol error"));
		m_client->disconnect("server reported a protocol error");
		return kDisconnect;
	}
	else {
		return kUnknown;
	}

	return kOkay;
}

CServerProxy::EResult
CServerProxy::parseMessage(const UInt8* code)
{
	if (memcmp(code, kMsgDMouseMove, 4) == 0) {
		mouseMove();
	}

	else if (memcmp(code, kMsgDMouseRelMove, 4) == 0) {
		mouseRelativeMove();
	}

	else if (memcmp(code, kMsgDMouseWheel, 4) == 0) {
		mouseWheel();
	}

	else if (memcmp(code, kMsgDKeyDown, 4) == 0) {
		keyDown();
	}

	else if (memcmp(code, kMsgDKeyUp, 4) == 0) {
		keyUp();
	}

	else if (memcmp(code, kMsgDMouseDown, 4) == 0) {
		mouseDown();
	}

	else if (memcmp(code, kMsgDMouseUp, 4) == 0) {
		mouseUp();
	}

	else if (memcmp(code, kMsgDKeyRepeat, 4) == 0) {
		keyRepeat();
	}

	else if (memcmp(code, kMsgCKeepAlive, 4) == 0) {
		// echo keep alives and reset alarm
		CProtocolUtil::writef(m_stream, kMsgCKeepAlive);
		resetKeepAliveAlarm();
	}

	else if (memcmp(code, kMsgCNoop, 4) == 0) {
		// accept and discard no-op
	}

	else if (memcmp(code, kMsgCEnter, 4) == 0) {
		enter();
	}

	else if (memcmp(code, kMsgCLeave, 4) == 0) {
		leave();
	}

	else if (memcmp(code, kMsgCClipboard, 4) == 0) {
		grabClipboard();
	}

	else if (memcmp(code, kMsgCScreenSaver, 4) == 0) {
		screensaver();
	}

	else if (memcmp(code, kMsgQInfo, 4) == 0) {
		queryInfo();
	}

	else if (memcmp(code, kMsgCInfoAck, 4) == 0) {
		infoAcknowledgment();
	}

	else if (memcmp(code, kMsgDClipboard, 4) == 0) {
		setClipboard();
	}

	else if (memcmp(code, kMsgCResetOptions, 4) == 0) {
		resetOptions();
	}

	else if (memcmp(code, kMsgDSetOptions, 4) == 0) {
		setOptions();
	}

	else if (memcmp(code, kMsgCClose, 4) == 0) {
		// server wants us to hangup
		LOG((CLOG_DEBUG1 "recv close"));
		m_client->disconnect(NULL);
		return kDisconnect;
	}
	else if (memcmp(code, kMsgEBad, 4) == 0) {
		LOG((CLOG_ERR "server disconnected due to a protocol error"));
		m_client->disconnect("server reported a protocol error");
		return kDisconnect;
	}
	else {
		return kUnknown;
	}

	// send a reply.  this is intended to work around a delay when
	// running a linux server and an OS X (any BSD?) client.  the
	// client waits to send an ACK (if the system control flag
	// net.inet.tcp.delayed_ack is 1) in hopes of piggybacking it
	// on a data packet.  we provide that packet here.  i don't
	// know why a delayed ACK should cause the server to wait since
	// TCP_NODELAY is enabled.
	CProtocolUtil::writef(m_stream, kMsgCNoop);

	return kOkay;
}

void
CServerProxy::handleKeepAliveAlarm(const CEvent&, void*)
{
	LOG((CLOG_NOTE "server is dead"));
	m_client->disconnect("server is not responding");
}

void
CServerProxy::onInfoChanged()
{
	// ignore mouse motion until we receive acknowledgment of our info
	// change message.
	m_ignoreMouse = true;

	// send info update
	queryInfo();
}

bool
CServerProxy::onGrabClipboard(ClipboardID cId)
{
	LOG((CLOG_DEBUG1 "sending clipboard %d changed", cId));
	CProtocolUtil::writef(m_stream, kMsgCClipboard, cId, m_seqNum);
	return true;
}

void
CServerProxy::onClipboardChanged(ClipboardID cId, const IClipboard* clipboard)
{
	CString data = IClipboard::marshall(clipboard);
	LOG((CLOG_DEBUG1 "sending clipboard %d seqnum=%d, size=%d", cId, m_seqNum, data.size()));
	CProtocolUtil::writef(m_stream, kMsgDClipboard, cId, m_seqNum, &data);
}

void
CServerProxy::flushCompressedMouse()
{
	if (m_compressMouse) {
		m_compressMouse = false;
//FIXXME		m_client->mouseMove(m_xMouse, m_yMouse);
	}
	if (m_compressMouseRelative) {
		m_compressMouseRelative = false;
//FIXXME		m_client->mouseRelativeMove(m_dxMouse, m_dyMouse);
		m_dxMouse = 0;
		m_dyMouse = 0;
	}
}

void
CServerProxy::sendInfo(const CClientInfo& info)
{
	LOG((CLOG_DEBUG1 "sending info shape=%d,%d %dx%d", info.m_x, info.m_y, info.m_w, info.m_h));
	CProtocolUtil::writef(m_stream, kMsgDInfo,
								info.m_x, info.m_y,
								info.m_w, info.m_h, 0,
								info.m_mx, info.m_my, info.m_id);
}

KeyID
CServerProxy::translateKey(KeyID kId) const
{
	static const KeyID s_translationTable[kKeyModifierIDLast][2] = {
		{ kKeyNone,      kKeyNone },
		{ kKeyShift_L,   kKeyShift_R },
		{ kKeyControl_L, kKeyControl_R },
		{ kKeyAlt_L,     kKeyAlt_R },
		{ kKeyMeta_L,    kKeyMeta_R },
		{ kKeySuper_L,   kKeySuper_R }
	};

	KeyModifierID kId2 = kKeyModifierIDNull;
	UInt32 side      = 0;
	switch (kId) {
	case kKeyShift_L:
		kId2  = kKeyModifierIDShift;
		side = 0;
		break;

	case kKeyShift_R:
		kId2  = kKeyModifierIDShift;
		side = 1;
		break;

	case kKeyControl_L:
		kId2  = kKeyModifierIDControl;
		side = 0;
		break;

	case kKeyControl_R:
		kId2  = kKeyModifierIDControl;
		side = 1;
		break;

	case kKeyAlt_L:
		kId2  = kKeyModifierIDAlt;
		side = 0;
		break;

	case kKeyAlt_R:
		kId2  = kKeyModifierIDAlt;
		side = 1;
		break;

	case kKeyMeta_L:
		kId2  = kKeyModifierIDMeta;
		side = 0;
		break;

	case kKeyMeta_R:
		kId2  = kKeyModifierIDMeta;
		side = 1;
		break;

	case kKeySuper_L:
		kId2  = kKeyModifierIDSuper;
		side = 0;
		break;

	case kKeySuper_R:
		kId2  = kKeyModifierIDSuper;
		side = 1;
		break;
	}

	if (kId2 != kKeyModifierIDNull) {
		return s_translationTable[m_modifierTranslationTable[kId2]][side];
	}
	else {
		return kId;
	}
}

KeyModifierMask
CServerProxy::translateModifierMask(KeyModifierMask mask) const
{
	static const KeyModifierMask s_masks[kKeyModifierIDLast] = {
		0x0000,
		KeyModifierShift,
		KeyModifierControl,
		KeyModifierAlt,
		KeyModifierMeta,
		KeyModifierSuper
	};

	KeyModifierMask newMask = mask & ~(KeyModifierShift |
										KeyModifierControl |
										KeyModifierAlt |
										KeyModifierMeta |
										KeyModifierSuper);
	if ((mask & KeyModifierShift) != 0) {
		newMask |= s_masks[m_modifierTranslationTable[kKeyModifierIDShift]];
	}
	if ((mask & KeyModifierControl) != 0) {
		newMask |= s_masks[m_modifierTranslationTable[kKeyModifierIDControl]];
	}
	if ((mask & KeyModifierAlt) != 0) {
		newMask |= s_masks[m_modifierTranslationTable[kKeyModifierIDAlt]];
	}
	if ((mask & KeyModifierMeta) != 0) {
		newMask |= s_masks[m_modifierTranslationTable[kKeyModifierIDMeta]];
	}
	if ((mask & KeyModifierSuper) != 0) {
		newMask |= s_masks[m_modifierTranslationTable[kKeyModifierIDSuper]];
	}
	return newMask;
}

void
CServerProxy::enter()
{
	// parse
	SInt16 x, y;
	UInt8 kId = 0;
	UInt8 pId = 0;
	UInt16 mask;
	UInt32 seqNum;
	CProtocolUtil::readf(m_stream, kMsgCEnter + 4, &x, &y, &seqNum, &mask, &kId, &pId);
	LOG((CLOG_DEBUG1 "recv enter(k:%d,p:%d), %d,%d %d %04x", kId, pId, x, y, seqNum, mask));

	// discard old compressed mouse motion, if any
	m_compressMouse         = false;
	m_compressMouseRelative = false;
	m_dxMouse               = 0;
	m_dyMouse               = 0;
	m_seqNum                = seqNum;

	// forward
	m_client->enter(x, y, seqNum, static_cast<KeyModifierMask>(mask), false, kId, pId);
}

void
CServerProxy::leave()
{
	UInt8 id = 0;
	// parse
	CProtocolUtil::readf(m_stream, kMsgCLeave + 4, &id);
	LOG((CLOG_DEBUG1 "recv leave, %d", id));

	// send last mouse motion
	flushCompressedMouse();

	// forward
	m_client->leave(id);
}

void
CServerProxy::setClipboard()
{
	// parse
	ClipboardID cId;
	UInt32 seqNum;
	CString data;
	CProtocolUtil::readf(m_stream, kMsgDClipboard + 4, &cId, &seqNum, &data);
	LOG((CLOG_DEBUG "recv clipboard %d size=%d", cId, data.size()));

	// validate
	if (cId >= kClipboardEnd) {
		return;
	}

	// forward
	CClipboard clipboard;
	clipboard.unmarshall(data, 0);
	m_client->setClipboard(cId, &clipboard);
}

void
CServerProxy::grabClipboard()
{
	// parse
	ClipboardID cId;
	UInt32 seqNum;
	CProtocolUtil::readf(m_stream, kMsgCClipboard + 4, &cId, &seqNum);
	LOG((CLOG_DEBUG "recv grab clipboard %d", cId));

	// validate
	if (cId >= kClipboardEnd) {
		return;
	}

	// forward
	m_client->grabClipboard(cId);
}

void
CServerProxy::keyDown()
{
	// get mouse up to date
	flushCompressedMouse();

	// parse
	UInt16 kId, mask, button;
	UInt8 id;
	CProtocolUtil::readf(m_stream, kMsgDKeyDown + 4, &kId, &mask, &button, &id);
	LOG((CLOG_DEBUG1 "recv key down dev=%d, kId=0x%08x, mask=0x%04x, button=0x%04x"
	, id, kId, mask, button));

	// translate
	KeyID kId2             = translateKey(static_cast<KeyID>(kId));
	KeyModifierMask mask2 = translateModifierMask(static_cast<KeyModifierMask>(mask));
	if (kId2   != static_cast<KeyID>(kId) ||
		mask2 != static_cast<KeyModifierMask>(mask))
		LOG((CLOG_DEBUG1 "key down translated to kId=0x%08x, mask=0x%04x", kId2, mask2));

	// forward
	m_client->keyDown(kId2, mask2, button, id);
}

void
CServerProxy::keyRepeat()
{
	// get mouse up to date
	flushCompressedMouse();

	// parse
	UInt16 kId, mask, count, button;
	UInt8 id;
	CProtocolUtil::readf(m_stream, kMsgDKeyRepeat + 4,
								&kId, &mask, &count, &button, &id);
	LOG((CLOG_DEBUG1 "recv key repeat dev=%d,kId=0x%08x, mask=0x%04x, count=%d, button=0x%04x"
	, id, kId, mask, count, button));

	// translate
	KeyID kId2             = translateKey(static_cast<KeyID>(kId));
	KeyModifierMask mask2 = translateModifierMask(
								static_cast<KeyModifierMask>(mask));
	if (kId2   != static_cast<KeyID>(kId) ||
		mask2 != static_cast<KeyModifierMask>(mask))
		LOG((CLOG_DEBUG1 "key repeat translated to kId=0x%08x, mask=0x%04x", kId2, mask2));

	// forward
	m_client->keyRepeat(kId2, mask2, count, button, id);
}

void
CServerProxy::keyUp()
{
	// get mouse up to date
	flushCompressedMouse();

	// parse
	UInt16 kId, mask, button;
	UInt8 id;
	CProtocolUtil::readf(m_stream, kMsgDKeyUp + 4, &kId, &mask, &button, &id);
	LOG((CLOG_DEBUG1 "recv key up dev=%d, kId=0x%08x, mask=0x%04x, button=0x%04x"
	, id, kId, mask, button));

	// translate
	KeyID kId2             = translateKey(static_cast<KeyID>(kId));
	KeyModifierMask mask2 = translateModifierMask(
								static_cast<KeyModifierMask>(mask));
	if (kId2   != static_cast<KeyID>(kId) ||
		mask2 != static_cast<KeyModifierMask>(mask))
		LOG((CLOG_DEBUG1 "key up translated to kId=0x%08x, mask=0x%04x", kId2, mask2));

	// forward
	m_client->keyUp(kId2, mask2, button, id);
}

void
CServerProxy::mouseDown()
{
	// get mouse up to date
	flushCompressedMouse();

	// parse
	SInt8 button;
	UInt8 id;
	CProtocolUtil::readf(m_stream, kMsgDMouseDown + 4, &button, &id);
	LOG((CLOG_DEBUG1 "recv mouse down dev=%d, button=%d", id, button));

	// forward
	m_client->mouseDown(static_cast<ButtonID>(button), id);
}

void
CServerProxy::mouseUp()
{
	// get mouse up to date
	flushCompressedMouse();

	// parse
	SInt8 button;
	UInt8 id;
	CProtocolUtil::readf(m_stream, kMsgDMouseUp + 4, &button, &id);
	LOG((CLOG_DEBUG1 "recv mouse up dev=%d, id=%d", id, button));

	// forward
	m_client->mouseUp(static_cast<ButtonID>(button), id);
}

void
CServerProxy::mouseMove()
{
	// parse
	bool ignore;
	SInt16 x, y;
	UInt8 id;
	CProtocolUtil::readf(m_stream, kMsgDMouseMove + 4, &x, &y, &id);

	// note if we should ignore the move
	ignore = m_ignoreMouse;

	// compress mouse motion events if more input follows
	if (!ignore && !m_compressMouse && m_stream->isReady()) {
		m_compressMouse = true;
	}

	// if compressing then ignore the motion but record it
	if (m_compressMouse) {
		m_compressMouseRelative = false;
		ignore    = true;
		m_xMouse  = x;
		m_yMouse  = y;
		m_dxMouse = 0;
		m_dyMouse = 0;
	}
	LOG((CLOG_DEBUG2 "recv mouse move dev=%d, %d,%d", id, x, y));

	// forward
	if (!ignore) {
		m_client->mouseMove(x, y,id);
	}
}

void
CServerProxy::mouseRelativeMove()
{
	// parse
	bool ignore;
	SInt16 dx, dy;
	UInt8 id;
	CProtocolUtil::readf(m_stream, kMsgDMouseRelMove + 4, &dx, &dy, &id);

	// note if we should ignore the move
	ignore = m_ignoreMouse;

	// compress mouse motion events if more input follows
	if (!ignore && !m_compressMouseRelative && m_stream->isReady()) {
		m_compressMouseRelative = true;
	}

	// if compressing then ignore the motion but record it
	if (m_compressMouseRelative) {
		ignore     = true;
		m_dxMouse += dx;
		m_dyMouse += dy;
	}
	LOG((CLOG_DEBUG2 "recv mouse relative move dev=%d, %d,%d", dx, dy, id));

	// forward
	if (!ignore) {
	    m_client->mouseRelativeMove(dx, dy, id);
	}
}

void
CServerProxy::mouseWheel()
{
	// get mouse up to date
	flushCompressedMouse();

	// parse
	SInt16 xDelta, yDelta;
	UInt8 id;
	CProtocolUtil::readf(m_stream, kMsgDMouseWheel + 4, &xDelta, &yDelta, &id);
	LOG((CLOG_DEBUG2 "recv mouse wheel dev=%d, %+d,%+d", id, xDelta, yDelta));

	// forward
	m_client->mouseWheel(xDelta, yDelta, id);
}

void
CServerProxy::screensaver()
{
	// parse
	SInt8 on;
	CProtocolUtil::readf(m_stream, kMsgCScreenSaver + 4, &on);
	LOG((CLOG_DEBUG1 "recv screen saver on=%d", on));

	// forward
	m_client->screensaver(on != 0);
}

void
CServerProxy::resetOptions()
{
	// parse
	LOG((CLOG_DEBUG1 "recv reset options"));

	// forward
	m_client->resetOptions();

	// reset keep alive
	setKeepAliveRate(kKeepAliveRate);

	// reset modifier translation table
	for (KeyModifierID kId = 0; kId < kKeyModifierIDLast; ++kId) {
		m_modifierTranslationTable[kId] = kId;
	}
}

void
CServerProxy::setOptions()
{
	// parse
	COptionsList options;
	CProtocolUtil::readf(m_stream, kMsgDSetOptions + 4, &options);
	LOG((CLOG_DEBUG1 "recv set options size=%d", options.size()));

	// forward
	m_client->setOptions(options);

	// update modifier table
	for (UInt32 i = 0, n = (UInt32)options.size(); i < n; i += 2) {
		KeyModifierID kId = kKeyModifierIDNull;
		if (options[i] == kOptionModifierMapForShift) {
			kId = kKeyModifierIDShift;
		}
		else if (options[i] == kOptionModifierMapForControl) {
			kId = kKeyModifierIDControl;
		}
		else if (options[i] == kOptionModifierMapForAlt) {
			kId = kKeyModifierIDAlt;
		}
		else if (options[i] == kOptionModifierMapForMeta) {
			kId = kKeyModifierIDMeta;
		}
		else if (options[i] == kOptionModifierMapForSuper) {
			kId = kKeyModifierIDSuper;
		}
		else if (options[i] == kOptionHeartbeat) {
			// update keep alive
			setKeepAliveRate(1.0e-3 * static_cast<double>(options[i + 1]));
		}
		if (kId != kKeyModifierIDNull) {
			m_modifierTranslationTable[kId] =
				static_cast<KeyModifierID>(options[i + 1]);
			LOG((CLOG_DEBUG1 "modifier %d mapped to %d", kId, m_modifierTranslationTable[kId]));
		}
	}
}

void
CServerProxy::queryInfo()
{
	CClientInfo info;
	//FIXXME queryinfo id
	//UInt8 id = 2;
	m_client->getShape(info.m_x, info.m_y, info.m_w, info.m_h);
	//m_client->getCursorPos(info.m_mx, info.m_my, id);
	sendInfo(info);
}

void
CServerProxy::infoAcknowledgment()
{
	LOG((CLOG_DEBUG1 "recv info acknowledgment"));
	m_ignoreMouse = false;
}
