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

#include "CGameDevice.h"
#include "CThread.h"
#include "CGameDevice.h"
#include "BasicTypes.h"
#include "GameDeviceTypes.h"
#include "IPrimaryScreen.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <vector>

class CMSWindowsScreen;

enum
{
	kGamePollFreqDefault = 100,
	kGamePollFreqMin = 50,
	kGamePollFreqMax = 200,
	kGameCalibrationPeriod = 10000, // 10 seconds
	kGameLagRecordMax = 10,
};

class CMSWindowsXInput : public CGameDevice
{
public:
	CMSWindowsXInput(CMSWindowsScreen* screen, const CGameDeviceInfo& gameDevice);
	virtual ~CMSWindowsXInput();

	void				init(CMSWindowsScreen* screen);
	
	// thread for polling xinput state.
	void				xInputPollThread(void*);

	// thread for checking queued timing requests. 
	void				xInputTimingThread(void*);

	// thread for checking pending feedback state.
	void				xInputFeedbackThread(void*);

	virtual void		gameDeviceTimingResp(UInt16 freq);
	virtual void		gameDeviceFeedback(GameDeviceID id, UInt16 m1, UInt16 m2);
	virtual void		fakeGameDeviceButtons(GameDeviceID id, GameDeviceButton buttons) const;
	virtual void		fakeGameDeviceSticks(GameDeviceID id, SInt16 x1, SInt16 y1, SInt16 x2, SInt16 y2) const;
	virtual void		fakeGameDeviceTriggers(GameDeviceID id, UInt8 t1, UInt8 t2) const;
	virtual void		queueGameDeviceTimingReq() const;

private:
	CMSWindowsScreen*	m_screen;
	const CGameDeviceInfo& m_gameDeviceInfo;
	CThread*			m_xInputPollThread;
	CThread*			m_xInputTimingThread;
	CThread*			m_xInputFeedbackThread;
	WORD				m_gameButtonsLast;
	BYTE				m_gameLeftTriggerLast;
	BYTE				m_gameRightTriggerLast;
	SHORT				m_gameLeftStickXLast;
	SHORT				m_gameLeftStickYLast;
	SHORT				m_gameRightStickXLast;
	SHORT				m_gameRightStickYLast;
	double				m_gameLastTimingSent;
	bool				m_gameTimingWaiting;
	UInt16				m_gameFakeLag;
	UInt16				m_gameFakeLagMin;
	UInt16				m_gamePollFreq;
	SInt8				m_gamePollFreqAdjust;
	UInt16				m_gameTimingStarted;
	UInt16				m_gameTimingFirst;
	UInt16				m_gameFakeLagLast;
	bool				m_gameTimingCalibrated;
	std::vector<UInt16>	m_gameFakeLagRecord;
	HMODULE				m_xinputModule;
};
