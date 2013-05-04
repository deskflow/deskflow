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

#pragma once

#include "GameDeviceTypes.h"

class CGameDeviceInfo
{
public:

	enum EGameMode {
		kGameModeNone,
		kGameModeXInput,
		kGameModeJoyInfoEx
	};

	enum EGamePoll {
		kGamePollDynamic,
		kGamePollStatic
	};

	CGameDeviceInfo();
	virtual ~CGameDeviceInfo();
	EGameMode m_mode;
	EGamePoll m_poll;
	int m_pollFreq;
};

class CGameDevice
{
public:
	CGameDevice();
	virtual ~CGameDevice();

	virtual void gameDeviceTimingResp(UInt16 freq) = 0;
	virtual void gameDeviceFeedback(GameDeviceID id, UInt16 m1, UInt16 m2) = 0;
	virtual void fakeGameDeviceButtons(GameDeviceID id, GameDeviceButton buttons) const = 0;
	virtual void fakeGameDeviceSticks(GameDeviceID id, SInt16 x1, SInt16 y1, SInt16 x2, SInt16 y2) const = 0;
	virtual void fakeGameDeviceTriggers(GameDeviceID id, UInt8 t1, UInt8 t2) const = 0;
	virtual void queueGameDeviceTimingReq() const = 0;
};
