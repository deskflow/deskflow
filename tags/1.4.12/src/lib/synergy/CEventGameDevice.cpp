/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Bolton Software Ltd.
 * Copyright (C) 2012 Nick Bolton
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

#include "CEventGameDevice.h"

CEventGameDevice::CEventGameDevice(void* eventTarget) :
	m_eventTarget(eventTarget)
{
}

CEventGameDevice::~CEventGameDevice()
{
}

void
CEventGameDevice::gameDeviceTimingResp(UInt16 freq)
{
}

void
CEventGameDevice::gameDeviceFeedback(GameDeviceID id, UInt16 m1, UInt16 m2)
{
}

void
CEventGameDevice::fakeGameDeviceButtons(GameDeviceID id, GameDeviceButton buttons) const
{
}

void
CEventGameDevice::fakeGameDeviceSticks(GameDeviceID id, SInt16 x1, SInt16 y1, SInt16 x2, SInt16 y2) const
{
}

void
CEventGameDevice::fakeGameDeviceTriggers(GameDeviceID id, UInt8 t1, UInt8 t2) const
{
}

void
CEventGameDevice::queueGameDeviceTimingReq() const
{
}
