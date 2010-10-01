/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) <year>  <name of author>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

*/

#include "CClientProxy1_4.h"
#include "CProtocolUtil.h"
#include "XSynergy.h"
#include "IStream.h"
#include "CLog.h"
#include "IEventQueue.h"
#include "TMethodEventJob.h"
#include <cstring>

CClientProxy1_4::CClientProxy1_4(const CString& name, IStream* stream) :
	CClientProxy1_3(name, stream),
	m_dev(CDeviceManager::getInstance())
{
    LOG((CLOG_DEBUG "Proxy1_4 created"));
}

void CClientProxy1_4::keyDown(KeyID key, KeyModifierMask mask, KeyButton button, UInt8 id)
{
	LOG((CLOG_DEBUG1 "send key down if dev=%d to \"%s\" id=%d, mask=0x%04x, button=0x%04x"
	, id, getName().c_str(), key, mask, button));
	CProtocolUtil::writef(getStream(), kMsgDKeyDown, key, mask, button, id);
}

void CClientProxy1_4::keyRepeat(KeyID key, KeyModifierMask mask , SInt32 count, KeyButton button, UInt8 id)
{
	LOG((CLOG_DEBUG1 "send key repeat of dev=%id to \"%s\" id=%d, mask=0x%04x, count=%d, button=0x%04x"
	, id, getName().c_str(), key, mask, count, button));
	CProtocolUtil::writef(getStream(), kMsgDKeyRepeat, key, mask, count, button, id);
}

void CClientProxy1_4::keyUp(KeyID key, KeyModifierMask mask, KeyButton button, UInt8 id)
{
	LOG((CLOG_DEBUG1 "send key up of dev=%d to \"%s\" id=%d, mask=0x%04x, button=0x%04x"
	, id, getName().c_str(), key, mask, button));
	CProtocolUtil::writef(getStream(), kMsgDKeyUp, key, mask, button, id);
}

void CClientProxy1_4::getCursorPos(SInt32& x, SInt32& y, UInt8 id) const
{
	// FIXXXME CClientProxy1_0::getCursorPos: CClientInfo ?
	// note -- this returns the cursor pos from when we last got client info
	
	//FIXXXME this returns the position of the local cursor. should be the remote one
	m_dev->getLastCursorPos(x, y, id);
	LOG((CLOG_DEBUG1 "returning ClientInfo: of dev=%d, x(%d),y(%d)", id, x, y));
}

void CClientProxy1_4::enter(SInt32 xAbs, SInt32 yAbs, UInt32 seqNum, KeyModifierMask mask, bool forScreensaver, UInt8 kId, UInt8 pId)
{
      LOG((CLOG_DEBUG1 "Proxy1_4: send enter of dev=(%d,%d) to \"%s\", %d,%d %d %04x"
      , pId, kId, getName().c_str(), xAbs, yAbs, seqNum, mask));
      CProtocolUtil::writef(getStream(), kMsgCEnter, xAbs, yAbs, seqNum, mask, kId, pId);
}

bool CClientProxy1_4::leave(UInt8 id)
{
	LOG((CLOG_DEBUG1 "send leave of dev=%d to \"%s\"", id, getName().c_str()));
	CProtocolUtil::writef(getStream(), kMsgCLeave, id);
	// we can never prevent the user from leaving
	return true;
}

void CClientProxy1_4::mouseDown(ButtonID button, UInt8 id)
{
	LOG((CLOG_DEBUG1 "send mouse down of dev=%d to \"%s\" id=%d"
	, id, getName().c_str(), button));
	CProtocolUtil::writef(getStream(), kMsgDMouseDown, button, id);
}

void CClientProxy1_4::mouseUp(ButtonID button, UInt8 id)
{
	LOG((CLOG_DEBUG1 "send mouse up of dev=%d to \"%s\" id=%d"
	, id, getName().c_str(), button));
	CProtocolUtil::writef(getStream(), kMsgDMouseUp, button, id);
}

void CClientProxy1_4::mouseMove(SInt32 xAbs, SInt32 yAbs, UInt8 id)
{
	LOG((CLOG_DEBUG2 "send mouse move of dev=%d to \"%s\" %d,%d"
	, id, getName().c_str(), xAbs, yAbs));
	CProtocolUtil::writef(getStream(), kMsgDMouseMove, xAbs, yAbs, id);
}

void CClientProxy1_4::mouseRelativeMove(SInt32 xRel, SInt32 yRel, UInt8 id)
{
	LOG((CLOG_DEBUG2 "send mouse relative move of dev=%d to \"%s\" %d,%d"
	, id, getName().c_str(), xRel, yRel));
	CProtocolUtil::writef(getStream(), kMsgDMouseRelMove, xRel, yRel, id);
}

void CClientProxy1_4::mouseWheel(SInt32 xDelta, SInt32 yDelta, UInt8 id)
{
	LOG((CLOG_DEBUG2 "send mouse wheel of dev=%d to \"%s\" %+d,%+d"
	, id, getName().c_str(), xDelta, yDelta));
	CProtocolUtil::writef(getStream(), kMsgDMouseWheel, xDelta, yDelta, id);
}

