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

#include "CServerProxy.h"
#include "CProtocolUtil.h"
#include "IClient.h"
#include "OptionTypes.h"
#include "ProtocolTypes.h"
#include "IInputStream.h"
#include "IOutputStream.h"
#include "CLock.h"
#include "CLog.h"
#include "CStopwatch.h"
#include "XBase.h"
#include <memory>

//
// CServerProxy
//

CServerProxy::CServerProxy(IClient* client,
				IInputStream* adoptedInput, IOutputStream* adoptedOutput) :
	m_client(client),
	m_input(adoptedInput),
	m_output(adoptedOutput),
	m_seqNum(0),
	m_heartRate(kHeartRate)
{
	assert(m_client != NULL);
	assert(m_input  != NULL);
	assert(m_output != NULL);

	// initialize modifier translation table
	for (KeyModifierID id = 0; id < kKeyModifierIDLast; ++id)
		m_modifierTranslationTable[id] = id;
}

CServerProxy::~CServerProxy()
{
	delete m_input;
	delete m_output;
}

bool
CServerProxy::mainLoop()
{
	bool failedToConnect = false;
	try {
		// no compressed mouse motion yet
		m_compressMouse = false;

		// not ignoring mouse motions
		m_ignoreMouse   = false;

		// reset sequence number
		m_seqNum        = 0;

		// handle messages from server
		CStopwatch heartbeat;
		for (;;) {
			// if no input is pending then flush compressed mouse motion
			if (getInputStream()->getSize() == 0) {
				flushCompressedMouse();
			}

			// wait for a message
			LOG((CLOG_DEBUG2 "waiting for message"));
			UInt8 code[4];
			UInt32 n = getInputStream()->read(code, 4, m_heartRate);

			// check if server hungup
			if (n == 0) {
				LOG((CLOG_NOTE "server disconnected"));
				break;
			}

			// check for time out
			if (n == (UInt32)-1 ||
				(m_heartRate >= 0.0 && heartbeat.getTime() > m_heartRate)) {
				// send heartbeat
				CLock lock(&m_mutex);
				CProtocolUtil::writef(getOutputStream(), kMsgCNoop);
				heartbeat.reset();
				if (n == (UInt32)-1) {
					// no message to process
					continue;
				}
			}

			// verify we got an entire code
			if (n != 4) {
				// client sent an incomplete message
				LOG((CLOG_ERR "incomplete message from server"));
				break;
			}

			// parse message
			LOG((CLOG_DEBUG2 "msg from server: %c%c%c%c", code[0], code[1], code[2], code[3]));
			if (memcmp(code, kMsgDMouseMove, 4) == 0) {
				mouseMove();
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
				break;
			}

			else if (memcmp(code, kMsgEIncompatible, 4) == 0) {
				SInt32 major, minor;
				CProtocolUtil::readf(getInputStream(),
								kMsgEIncompatible + 4, &major, &minor);
				LOG((CLOG_ERR "server has incompatible version %d.%d", major, minor));
				failedToConnect = true;
				break;
			}

			else if (memcmp(code, kMsgEBusy, 4) == 0) {
				LOG((CLOG_ERR "server already has a connected client with name \"%s\"", getName().c_str()));
				failedToConnect = true;
				break;
			}

			else if (memcmp(code, kMsgEUnknown, 4) == 0) {
				LOG((CLOG_ERR "server refused client with name \"%s\"", getName().c_str()));
				failedToConnect = true;
				break;
			}

			else if (memcmp(code, kMsgEBad, 4) == 0) {
				LOG((CLOG_ERR "server disconnected due to a protocol error"));
				failedToConnect = true;
				break;
			}

			else {
				// unknown message
				LOG((CLOG_ERR "unknown message from server"));
				LOG((CLOG_ERR "unknown message: %d %d %d %d [%c%c%c%c]", code[0], code[1], code[2], code[3], code[0], code[1], code[2], code[3]));
				failedToConnect = true;
				break;
			}
		}
	}
	catch (XBase& e) {
		LOG((CLOG_ERR "error: %s", e.what()));
	}
	catch (...) {
		throw;
	}

	return !failedToConnect;
}

IClient*
CServerProxy::getClient() const
{
	return m_client;
}

CString
CServerProxy::getName() const
{
	return m_client->getName();
}

IInputStream*
CServerProxy::getInputStream() const
{
	return m_input;
}

IOutputStream*
CServerProxy::getOutputStream() const
{
	return m_output;
}

void
CServerProxy::onError()
{
	// ignore
}

void
CServerProxy::onInfoChanged(const CClientInfo& info)
{
	// ignore mouse motion until we receive acknowledgment of our info
	// change message.
	CLock lock(&m_mutex);
	m_ignoreMouse = true;

	// send info update
	sendInfo(info);
}

bool
CServerProxy::onGrabClipboard(ClipboardID id)
{
	LOG((CLOG_DEBUG1 "sending clipboard %d changed", id));
	CLock lock(&m_mutex);
	CProtocolUtil::writef(getOutputStream(), kMsgCClipboard, id, m_seqNum);
	return true;
}

void
CServerProxy::onClipboardChanged(ClipboardID id, const CString& data)
{
	CLock lock(&m_mutex);
	LOG((CLOG_DEBUG1 "sending clipboard %d seqnum=%d, size=%d", id, m_seqNum, data.size()));
	CProtocolUtil::writef(getOutputStream(), kMsgDClipboard, id, m_seqNum, &data);
}

void
CServerProxy::flushCompressedMouse()
{
	bool send = false;
	SInt32 x = 0, y = 0;
	{
		CLock lock(&m_mutex);
		if (m_compressMouse) {
			m_compressMouse = false;
			x               = m_xMouse;
			y               = m_yMouse;
			send            = true;
		}
	}

	if (send) {
		getClient()->mouseMove(x, y);
	}
}

void
CServerProxy::sendInfo(const CClientInfo& info)
{
	// note -- m_mutex should be locked on entry
	LOG((CLOG_DEBUG1 "sending info shape=%d,%d %dx%d zone=%d pos=%d,%d", info.m_x, info.m_y, info.m_w, info.m_h, info.m_zoneSize, info.m_mx, info.m_my));
	CProtocolUtil::writef(getOutputStream(), kMsgDInfo,
								info.m_x, info.m_y,
								info.m_w, info.m_h,
								info.m_zoneSize,
								info.m_mx, info.m_my);
}

KeyID
CServerProxy::translateKey(KeyID id) const
{
	static const KeyID s_translationTable[kKeyModifierIDLast][2] = {
		{ kKeyNone,      kKeyNone },
		{ kKeyShift_L,   kKeyShift_R },
		{ kKeyControl_L, kKeyControl_R },
		{ kKeyAlt_L,     kKeyAlt_R },
		{ kKeyMeta_L,    kKeyMeta_R },
		{ kKeySuper_L,   kKeySuper_R }
	};

	KeyModifierID id2 = kKeyModifierIDNull;
	UInt32 side      = 0;
	switch (id) {
	case kKeyShift_L:
		id2  = kKeyModifierIDShift;
		side = 0;
		break;

	case kKeyShift_R:
		id2  = kKeyModifierIDShift;
		side = 1;
		break;

	case kKeyControl_L:
		id2  = kKeyModifierIDControl;
		side = 0;
		break;

	case kKeyControl_R:
		id2  = kKeyModifierIDControl;
		side = 1;
		break;

	case kKeyAlt_L:
		id2  = kKeyModifierIDAlt;
		side = 0;
		break;

	case kKeyAlt_R:
		id2  = kKeyModifierIDAlt;
		side = 1;
		break;

	case kKeyMeta_L:
		id2  = kKeyModifierIDMeta;
		side = 0;
		break;

	case kKeyMeta_R:
		id2  = kKeyModifierIDMeta;
		side = 1;
		break;

	case kKeySuper_L:
		id2  = kKeyModifierIDSuper;
		side = 0;
		break;

	case kKeySuper_R:
		id2  = kKeyModifierIDSuper;
		side = 1;
		break;
	}

	if (id2 != kKeyModifierIDNull) {
		return s_translationTable[m_modifierTranslationTable[id2]][side];
	}
	else {
		return id;
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
	UInt16 mask;
	UInt32 seqNum;
	CProtocolUtil::readf(getInputStream(),
								kMsgCEnter + 4, &x, &y, &seqNum, &mask);
	LOG((CLOG_DEBUG1 "recv enter, %d,%d %d %04x", x, y, seqNum, mask));

	// discard old compressed mouse motion, if any
	{
		CLock lock(&m_mutex);
		m_compressMouse = false;
		m_seqNum        = seqNum;
	}

	// forward
	getClient()->enter(x, y, seqNum, static_cast<KeyModifierMask>(mask), false);
}

void
CServerProxy::leave()
{
	// parse
	LOG((CLOG_DEBUG1 "recv leave"));

	// send last mouse motion
	flushCompressedMouse();

	// forward
	getClient()->leave();
}

void
CServerProxy::setClipboard()
{
	// parse
	ClipboardID id;
	UInt32 seqNum;
	CString data;
	CProtocolUtil::readf(getInputStream(),
								kMsgDClipboard + 4, &id, &seqNum, &data);
	LOG((CLOG_DEBUG "recv clipboard %d size=%d", id, data.size()));

	// validate
	if (id >= kClipboardEnd) {
		return;
	}

	// forward
	getClient()->setClipboard(id, data);
}

void
CServerProxy::grabClipboard()
{
	// parse
	ClipboardID id;
	UInt32 seqNum;
	CProtocolUtil::readf(getInputStream(), kMsgCClipboard + 4, &id, &seqNum);
	LOG((CLOG_DEBUG "recv grab clipboard %d", id));

	// validate
	if (id >= kClipboardEnd) {
		return;
	}

	// forward
	getClient()->grabClipboard(id);
}

void
CServerProxy::keyDown()
{
	// get mouse up to date
	flushCompressedMouse();

	// parse
	UInt16 id, mask, button;
	CProtocolUtil::readf(getInputStream(), kMsgDKeyDown + 4,
								&id, &mask, &button);
	LOG((CLOG_DEBUG1 "recv key down id=%d, mask=0x%04x, button=0x%04x", id, mask, button));

	// translate
	KeyID id2             = translateKey(static_cast<KeyID>(id));
	KeyModifierMask mask2 = translateModifierMask(
								static_cast<KeyModifierMask>(mask));
	if (id2   != static_cast<KeyID>(id) ||
		mask2 != static_cast<KeyModifierMask>(mask))
		LOG((CLOG_DEBUG1 "key down translated to id=%d, mask=0x%04x", id2, mask2));

	// forward
	getClient()->keyDown(id2, mask2, button);
}

void
CServerProxy::keyRepeat()
{
	// get mouse up to date
	flushCompressedMouse();

	// parse
	UInt16 id, mask, count, button;
	CProtocolUtil::readf(getInputStream(), kMsgDKeyRepeat + 4,
								&id, &mask, &count, &button);
	LOG((CLOG_DEBUG1 "recv key repeat id=%d, mask=0x%04x, count=%d, button=0x%04x", id, mask, count, button));

	// translate
	KeyID id2             = translateKey(static_cast<KeyID>(id));
	KeyModifierMask mask2 = translateModifierMask(
								static_cast<KeyModifierMask>(mask));
	if (id2   != static_cast<KeyID>(id) ||
		mask2 != static_cast<KeyModifierMask>(mask))
		LOG((CLOG_DEBUG1 "key repeat translated to id=%d, mask=0x%04x", id2, mask2));

	// forward
	getClient()->keyRepeat(id2, mask2, count, button);
}

void
CServerProxy::keyUp()
{
	// get mouse up to date
	flushCompressedMouse();

	// parse
	UInt16 id, mask, button;
	CProtocolUtil::readf(getInputStream(), kMsgDKeyUp + 4, &id, &mask, &button);
	LOG((CLOG_DEBUG1 "recv key up id=%d, mask=0x%04x, button=0x%04x", id, mask, button));

	// translate
	KeyID id2             = translateKey(static_cast<KeyID>(id));
	KeyModifierMask mask2 = translateModifierMask(
								static_cast<KeyModifierMask>(mask));
	if (id2   != static_cast<KeyID>(id) ||
		mask2 != static_cast<KeyModifierMask>(mask))
		LOG((CLOG_DEBUG1 "key up translated to id=%d, mask=0x%04x", id2, mask2));

	// forward
	getClient()->keyUp(id2, mask2, button);
}

void
CServerProxy::mouseDown()
{
	// get mouse up to date
	flushCompressedMouse();

	// parse
	SInt8 id;
	CProtocolUtil::readf(getInputStream(), kMsgDMouseDown + 4, &id);
	LOG((CLOG_DEBUG1 "recv mouse down id=%d", id));

	// forward
	getClient()->mouseDown(static_cast<ButtonID>(id));
}

void
CServerProxy::mouseUp()
{
	// get mouse up to date
	flushCompressedMouse();

	// parse
	SInt8 id;
	CProtocolUtil::readf(getInputStream(), kMsgDMouseUp + 4, &id);
	LOG((CLOG_DEBUG1 "recv mouse up id=%d", id));

	// forward
	getClient()->mouseUp(static_cast<ButtonID>(id));
}

void
CServerProxy::mouseMove()
{
	// parse
	bool ignore;
	SInt16 x, y;
	CProtocolUtil::readf(getInputStream(), kMsgDMouseMove + 4, &x, &y);

	{
		// note if we should ignore the move
		CLock lock(&m_mutex);
		ignore = m_ignoreMouse;

		// compress mouse motion events if more input follows
		if (!ignore && !m_compressMouse && getInputStream()->getSize() > 0) {
			m_compressMouse = true;
		}

		// if compressing then ignore the motion but record it
		if (m_compressMouse) {
			ignore   = true;
			m_xMouse = x;
			m_yMouse = y;
		}
	}
	LOG((CLOG_DEBUG2 "recv mouse move %d,%d", x, y));

	// forward
	if (!ignore) {
		getClient()->mouseMove(x, y);
	}
}

void
CServerProxy::mouseWheel()
{
	// get mouse up to date
	flushCompressedMouse();

	// parse
	SInt16 delta;
	CProtocolUtil::readf(getInputStream(), kMsgDMouseWheel + 4, &delta);
	LOG((CLOG_DEBUG2 "recv mouse wheel %+d", delta));

	// forward
	getClient()->mouseWheel(delta);
}

void
CServerProxy::screensaver()
{
	// parse
	SInt8 on;
	CProtocolUtil::readf(getInputStream(), kMsgCScreenSaver + 4, &on);
	LOG((CLOG_DEBUG1 "recv screen saver on=%d", on));

	// forward
	getClient()->screensaver(on != 0);
}

void
CServerProxy::resetOptions()
{
	// parse
	LOG((CLOG_DEBUG1 "recv reset options"));

	// forward
	getClient()->resetOptions();

	CLock lock(&m_mutex);

	// reset heart rate
	m_heartRate = kHeartRate;

	// reset modifier translation table
	for (KeyModifierID id = 0; id < kKeyModifierIDLast; ++id) {
		m_modifierTranslationTable[id] = id;
	}

	// send heartbeat if necessary
	if (m_heartRate >= 0.0) {
		CProtocolUtil::writef(getOutputStream(), kMsgCNoop);
	}
}

void
CServerProxy::setOptions()
{
	// parse
	COptionsList options;
	CProtocolUtil::readf(getInputStream(), kMsgDSetOptions + 4, &options);
	LOG((CLOG_DEBUG1 "recv set options size=%d", options.size()));

	// forward
	getClient()->setOptions(options);

	CLock lock(&m_mutex);

	// update modifier table
	for (UInt32 i = 0, n = options.size(); i < n; i += 2) {
		KeyModifierID id = kKeyModifierIDNull;
		if (options[i] == kOptionModifierMapForShift) {
			id = kKeyModifierIDShift;
		}
		else if (options[i] == kOptionModifierMapForControl) {
			id = kKeyModifierIDControl;
		}
		else if (options[i] == kOptionModifierMapForAlt) {
			id = kKeyModifierIDAlt;
		}
		else if (options[i] == kOptionModifierMapForMeta) {
			id = kKeyModifierIDMeta;
		}
		else if (options[i] == kOptionModifierMapForSuper) {
			id = kKeyModifierIDSuper;
		}
		else if (options[i] == kOptionHeartbeat) {
			// update heart rate
			m_heartRate = 1.0e-3 * static_cast<double>(options[i + 1]);

			// send heartbeat if necessary
			if (m_heartRate >= 0.0) {
				CProtocolUtil::writef(getOutputStream(), kMsgCNoop);
			}
		}
		if (id != kKeyModifierIDNull) {
			m_modifierTranslationTable[id] =
				static_cast<KeyModifierID>(options[i + 1]);
			LOG((CLOG_DEBUG1 "modifier %d mapped to %d", id, m_modifierTranslationTable[id]));
		}
	}
}

void
CServerProxy::queryInfo()
{
	// get current info
	CClientInfo info;
	getClient()->getShape(info.m_x, info.m_y, info.m_w, info.m_h);
	getClient()->getCursorPos(info.m_mx, info.m_my);
	info.m_zoneSize = getClient()->getJumpZoneSize();

	// send it
	CLock lock(&m_mutex);
	sendInfo(info);
}

void
CServerProxy::infoAcknowledgment()
{
	// parse
	LOG((CLOG_DEBUG1 "recv info acknowledgment"));

	// now allow mouse motion
	CLock lock(&m_mutex);
	m_ignoreMouse = false;
}
