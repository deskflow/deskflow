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

#include "CClientProxy1_1.h"
#include "CProtocolUtil.h"
#include "CLog.h"
#include <cstring>

//
// CClientProxy1_1
//

CClientProxy1_1::CClientProxy1_1(const CString& name, synergy::IStream* stream, IEventQueue* eventQueue) :
	CClientProxy1_0(name, stream, eventQueue)
{
	// do nothing
}

CClientProxy1_1::~CClientProxy1_1()
{
	// do nothing
}

void
CClientProxy1_1::keyDown(KeyID key, KeyModifierMask mask, KeyButton button)
{
	LOG((CLOG_DEBUG1 "send key down to \"%s\" id=%d, mask=0x%04x, button=0x%04x", getName().c_str(), key, mask, button));
	CProtocolUtil::writef(getStream(), kMsgDKeyDown, key, mask, button);
}

void
CClientProxy1_1::keyRepeat(KeyID key, KeyModifierMask mask,
				SInt32 count, KeyButton button)
{
	LOG((CLOG_DEBUG1 "send key repeat to \"%s\" id=%d, mask=0x%04x, count=%d, button=0x%04x", getName().c_str(), key, mask, count, button));
	CProtocolUtil::writef(getStream(), kMsgDKeyRepeat, key, mask, count, button);
}

void
CClientProxy1_1::keyUp(KeyID key, KeyModifierMask mask, KeyButton button)
{
	LOG((CLOG_DEBUG1 "send key up to \"%s\" id=%d, mask=0x%04x, button=0x%04x", getName().c_str(), key, mask, button));
	CProtocolUtil::writef(getStream(), kMsgDKeyUp, key, mask, button);
}
