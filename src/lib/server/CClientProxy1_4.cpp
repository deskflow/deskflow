/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Bolton Software Ltd.
 * Copyright (C) 2011 Chris Schoeneman
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

#include "CClientProxy1_4.h"
#include "CProtocolUtil.h"
#include "CLog.h"
#include "IEventQueue.h"
#include "TMethodEventJob.h"
#include <cstring>
#include <memory>
#include "CServer.h"
#include "CCryptoStream.h"

//
// CClientProxy1_4
//

CClientProxy1_4::CClientProxy1_4(const CString& name, synergy::IStream* stream, CServer* server, IEventQueue* eventQueue) :
	CClientProxy1_3(name, stream, eventQueue), m_server(server)
{
	assert(m_server != NULL);
}

CClientProxy1_4::~CClientProxy1_4()
{
}

void
CClientProxy1_4::gameDeviceButtons(GameDeviceID id, GameDeviceButton buttons)
{
	LOG((CLOG_DEBUG2 "send game device buttons to \"%s\" id=%d buttons=%d", getName().c_str(), id, buttons));
	CProtocolUtil::writef(getStream(), kMsgDGameButtons, id, buttons);
}

void
CClientProxy1_4::gameDeviceSticks(GameDeviceID id, SInt16 x1, SInt16 y1, SInt16 x2, SInt16 y2)
{
	LOG((CLOG_DEBUG2 "send game device sticks to \"%s\" id=%d s1=%+d,%+d s2=%+d,%+d", getName().c_str(), id, x1, y1, x2, y2));
	CProtocolUtil::writef(getStream(), kMsgDGameSticks, id, x1, y1, x2, y2);
}

void
CClientProxy1_4::gameDeviceTriggers(GameDeviceID id, UInt8 t1, UInt8 t2)
{
	LOG((CLOG_DEBUG2 "send game device triggers to \"%s\" id=%d t1=%d t2=%d", getName().c_str(), id, t1, t2));
	CProtocolUtil::writef(getStream(), kMsgDGameTriggers, id, t1, t2);
}

void
CClientProxy1_4::gameDeviceTimingReq()
{
	LOG((CLOG_DEBUG2 "send game device timing request to \"%s\"", getName().c_str()));
	CProtocolUtil::writef(getStream(), kMsgCGameTimingReq);
}

void
CClientProxy1_4::keyDown(KeyID key, KeyModifierMask mask, KeyButton button)
{
	cryptoIv();
	CClientProxy1_3::keyDown(key, mask, button);
}

void
CClientProxy1_4::keyRepeat(KeyID key, KeyModifierMask mask, SInt32 count, KeyButton button)
{
	cryptoIv();
	CClientProxy1_3::keyRepeat(key, mask, count, button);
}

void
CClientProxy1_4::keyUp(KeyID key, KeyModifierMask mask, KeyButton button)
{
	cryptoIv();
	CClientProxy1_3::keyUp(key, mask, button);
}

void
CClientProxy1_4::cryptoIv()
{
	CCryptoStream* cryptoStream = dynamic_cast<CCryptoStream*>(getStream());
	if (cryptoStream == NULL) {
		return;
	}

	byte iv[CRYPTO_IV_SIZE];
	cryptoStream->newIv(iv);
	CString data(reinterpret_cast<const char*>(iv), CRYPTO_IV_SIZE);

	LOG((CLOG_DEBUG2 "send crypto iv change to \"%s\"", getName().c_str()));
	CProtocolUtil::writef(getStream(), kMsgDCryptoIv, &data);
	
	// change IV only after we've sent the current IV, otherwise
	// the client won't be able to decrypt the new IV.
	cryptoStream->setEncryptIv(iv);
}

bool
CClientProxy1_4::parseMessage(const UInt8* code)
{
	// process message
	if (memcmp(code, kMsgCGameTimingResp, 4) == 0) {
		gameDeviceTimingResp();
	}
	
	else if (memcmp(code, kMsgDGameFeedback, 4) == 0) {
		gameDeviceFeedback();
	}

	else {
		return CClientProxy1_3::parseMessage(code);
	}

	return true;
}

void
CClientProxy1_4::gameDeviceFeedback()
{
	// parse
	GameDeviceID id;
	UInt16 m1, m2;
	CProtocolUtil::readf(getStream(), kMsgDGameFeedback + 4, &id, &m1, &m2);
	LOG((CLOG_DEBUG2 "recv game device feedback id=%d m1=%d m2=%d", id, m1, m2));

	// forward
	m_server->gameDeviceFeedback(id, m1, m2);
}

void
CClientProxy1_4::gameDeviceTimingResp()
{
	// parse
	UInt16 freq;
	CProtocolUtil::readf(getStream(), kMsgCGameTimingResp + 4, &freq);
	LOG((CLOG_DEBUG2 "recv game device timing response freq=%dms", freq));

	// forward
	m_server->gameDeviceTimingResp(freq);
}
