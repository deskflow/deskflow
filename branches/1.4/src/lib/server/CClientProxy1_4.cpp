/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2011 Chris Schoeneman, Nick Bolton, Sorin Sbarnea
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

//
// CClientProxy1_4
//

CClientProxy1_4::CClientProxy1_4(const CString& name, IStream* stream) :
	CClientProxy1_3(name, stream)
{
}

CClientProxy1_4::~CClientProxy1_4()
{
}

void
CClientProxy1_4::gamepadButtonDown(GamepadButtonID id)
{
	LOG((CLOG_DEBUG2 "send gamepad button down to \"%s\" id=%+d", getName().c_str()));
	CProtocolUtil::writef(getStream(), kMsgDGamepadDown, id);
}

void
CClientProxy1_4::gamepadButtonUp(GamepadButtonID id)
{
	LOG((CLOG_DEBUG2 "send gamepad button up to \"%s\" id=%+d", getName().c_str()));
	CProtocolUtil::writef(getStream(), kMsgDGamepadUp, id);
}

void
CClientProxy1_4::gamepadAnalog(GamepadAnalogID id, SInt16 x, SInt16 y)
{
	LOG((CLOG_DEBUG2 "send gamepad analog to \"%s\" id=%+d %+d,%+d", getName().c_str(), id, x, y));
	CProtocolUtil::writef(getStream(), kMsgDGamepadAnalog, id, x, y);
}
