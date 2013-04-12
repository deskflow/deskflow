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
 * along with this program.  If notsee <http://www.gnu.org/licenses/>.
 */

#include "CMSWindowsXInput.h"
#include "XScreen.h"
#include "CThread.h"
#include "TMethodJob.h"
#include "CLog.h"
#include "XInputHook.h"
#include "CMSWindowsScreen.h"

#include "XInput.h"

typedef DWORD (WINAPI *XInputGetStateFunc)(DWORD, XINPUT_STATE*);
typedef DWORD (WINAPI *XInputSetStateFunc)(DWORD, XINPUT_VIBRATION*);

CMSWindowsXInput::CMSWindowsXInput(CMSWindowsScreen* screen, const CGameDeviceInfo& gameDeviceInfo) :
m_screen(screen),
m_gameDeviceInfo(gameDeviceInfo),
m_xInputPollThread(NULL),
m_xInputTimingThread(NULL),
m_xInputFeedbackThread(NULL),
m_gameButtonsLast(0),
m_gameLeftTriggerLast(0),
m_gameRightTriggerLast(0),
m_gameLeftStickXLast(0),
m_gameLeftStickYLast(0),
m_gameRightStickXLast(0),
m_gameRightStickYLast(0),
m_gameLastTimingSent(0),
m_gameTimingWaiting(false),
m_gameFakeLag(0),
m_gamePollFreq(kGamePollFreqDefault),
m_gamePollFreqAdjust(0),
m_gameFakeLagMin(kGamePollFreqDefault),
m_gameTimingStarted(false),
m_gameTimingFirst(0),
m_gameFakeLagLast(0),
m_gameTimingCalibrated(false),
m_xinputModule(NULL)
{
	m_xinputModule = LoadLibrary("xinput1_3.dll");
	if (m_xinputModule == NULL)
	{
		throw XScreenXInputFailure("could not load xinput library");
	}

	if (screen->isPrimary())
	{
		// only capture xinput on the server.
		m_xInputPollThread = new CThread(new TMethodJob<CMSWindowsXInput>(
			this, &CMSWindowsXInput::xInputPollThread));
	}
	else
	{
		// check for queued timing requests on client.
		m_xInputTimingThread = new CThread(new TMethodJob<CMSWindowsXInput>(
			this, &CMSWindowsXInput::xInputTimingThread));

		// check for waiting feedback state on client.
		m_xInputFeedbackThread = new CThread(new TMethodJob<CMSWindowsXInput>(
			this, &CMSWindowsXInput::xInputFeedbackThread));
	}
}

CMSWindowsXInput::~CMSWindowsXInput()
{
}

void
CMSWindowsXInput::fakeGameDeviceButtons(GameDeviceID id, GameDeviceButton buttons) const
{
	SetXInputButtons(id, buttons);
}

void
CMSWindowsXInput::fakeGameDeviceSticks(GameDeviceID id, SInt16 x1, SInt16 y1, SInt16 x2, SInt16 y2) const
{
	SetXInputSticks(id, x1, y1, x2, y2);
}

void
CMSWindowsXInput::fakeGameDeviceTriggers(GameDeviceID id, UInt8 t1, UInt8 t2) const
{
	SetXInputTriggers(id, t1, t2);
}

void
CMSWindowsXInput::queueGameDeviceTimingReq() const
{
	QueueXInputTimingReq();
}

void
CMSWindowsXInput::gameDeviceTimingResp(UInt16 freq)
{
	if (!m_gameTimingStarted)
	{
		// record when timing started for calibration period.
		m_gameTimingFirst = (UInt16)(ARCH->time() * 1000);
		m_gameTimingStarted = true;
	}

	m_gameTimingWaiting = false;
	m_gameFakeLagLast = m_gameFakeLag;
	m_gameFakeLag = (UInt16)((ARCH->time() - m_gameLastTimingSent) * 1000);
	m_gameFakeLagRecord.push_back(m_gameFakeLag);

	if (m_gameFakeLag < m_gameFakeLagMin)
	{
		// record the lowest value so that the poll frequency
		// is adjusted to track.
		m_gameFakeLagMin = m_gameFakeLag;
	}
	else if (m_gameFakeLag > (m_gameFakeLagLast * 2))
	{
		// if fake lag has increased significantly since the last
		// timing, then we must have reached the safe minimum.
		m_gameFakeLagMin = m_gameFakeLagLast;
	}

	// only change poll frequency if it's a sensible value.
	if (freq > kGamePollFreqMin && freq < kGamePollFreqMax)
	{
		m_gamePollFreq = freq;
	}

	UInt16 timeSinceStart = ((UInt16)(ARCH->time() * 1000) - m_gameTimingFirst);
	if (!m_gameTimingCalibrated && (timeSinceStart < kGameCalibrationPeriod))
	{
		// during the calibration period, increase polling speed
		// to try and find the lowest lag value.
		m_gamePollFreqAdjust = 1;
		LOG((CLOG_DEBUG2 "calibrating game device poll frequency, start=%d, now=%d, since=%d",
			m_gameTimingFirst, (int)(ARCH->time() * 1000), timeSinceStart));
	}
	else
	{
		// @bug - calibration seems to re-occur after a period of time,
		// though, this would actually be a nice feature (but could be
		// a bit risky -- could cause poor game play)... setting this
		// stops calibration from happening again.
		m_gameTimingCalibrated = true;

		// only adjust poll frequency if outside desired limits.
		m_gamePollFreqAdjust = 0;
		if (m_gameFakeLag > m_gameFakeLagMin * 3)
		{
			m_gamePollFreqAdjust = 1;
		}
		else if (m_gameFakeLag < m_gameFakeLagMin)
		{
			m_gamePollFreqAdjust = -1;
		}
	}

	LOG((CLOG_DEBUG3 "game device timing, lag=%dms, freq=%dms, adjust=%dms, min=%dms",
		m_gameFakeLag, m_gamePollFreq, m_gamePollFreqAdjust, m_gameFakeLagMin));

	if (m_gameFakeLagRecord.size() >= kGameLagRecordMax)
	{
		UInt16 v, min = 65535, max = 0, total = 0;
		std::vector<UInt16>::iterator it;
		for (it = m_gameFakeLagRecord.begin(); it < m_gameFakeLagRecord.end(); ++it)
		{
			v = *it;
			if (v < min)
			{
				min = v;
			}
			if (v > max)
			{
				max = v;
			}
			total += v;
		}

		LOG((CLOG_INFO "game device timing, min=%dms, max=%dms, avg=%dms",
			min, max, (UInt16)(total / m_gameFakeLagRecord.size())));
		m_gameFakeLagRecord.clear();
	}
}

void
CMSWindowsXInput::gameDeviceFeedback(GameDeviceID id, UInt16 m1, UInt16 m2)
{
	XInputSetStateFunc xInputSetStateFunc =
		(XInputSetStateFunc)GetProcAddress(m_xinputModule, "XInputSetState");

	if (xInputSetStateFunc == NULL)
	{
		throw XScreenXInputFailure("could not get function address: XInputSetState");
	}

	XINPUT_VIBRATION vibration;
	ZeroMemory(&vibration, sizeof(XINPUT_VIBRATION));
	vibration.wLeftMotorSpeed = m1;
	vibration.wRightMotorSpeed = m2;
	xInputSetStateFunc(id, &vibration);
}

void
CMSWindowsXInput::xInputPollThread(void*)
{
	LOG((CLOG_DEBUG "xinput poll thread started"));

	XInputGetStateFunc xInputGetStateFunc = 
		(XInputGetStateFunc)GetProcAddress(m_xinputModule, "XInputGetState");

	if (xInputGetStateFunc == NULL)
	{
		throw XScreenXInputFailure("could not get function address: XInputGetState");
	}

	int index = 0;
	XINPUT_STATE state;
	bool end = false;
	while (!end)
	{
		ZeroMemory(&state, sizeof(XINPUT_STATE));
		DWORD result = xInputGetStateFunc(index, &state);

		// timeout the timing request after 10 seconds
		if (m_gameTimingWaiting && (ARCH->time() - m_gameLastTimingSent > 2))
		{
			m_gameTimingWaiting = false;
			LOG((CLOG_DEBUG "game device timing request timed out"));
		}
		
		// xinput controller is connected
		if (result == ERROR_SUCCESS)
		{
			// @todo game device state class
			bool buttonsChanged = state.Gamepad.wButtons != m_gameButtonsLast;
			bool leftTriggerChanged = state.Gamepad.bLeftTrigger != m_gameLeftTriggerLast;
			bool rightTriggerChanged = state.Gamepad.bRightTrigger != m_gameRightTriggerLast;
			bool leftStickXChanged = state.Gamepad.sThumbLX != m_gameLeftStickXLast;
			bool leftStickYChanged = state.Gamepad.sThumbLY != m_gameLeftStickYLast;
			bool rightStickXChanged = state.Gamepad.sThumbRX != m_gameRightStickXLast;
			bool rightStickYChanged = state.Gamepad.sThumbRY != m_gameRightStickYLast;

			m_gameButtonsLast = state.Gamepad.wButtons;
			m_gameLeftTriggerLast = state.Gamepad.bLeftTrigger;
			m_gameRightTriggerLast = state.Gamepad.bRightTrigger;
			m_gameLeftStickXLast = state.Gamepad.sThumbLX;
			m_gameLeftStickYLast = state.Gamepad.sThumbLY;
			m_gameRightStickXLast = state.Gamepad.sThumbRX;
			m_gameRightStickYLast = state.Gamepad.sThumbRY;

			bool eventSent = false;

			if (buttonsChanged)
			{
				LOG((CLOG_DEBUG "xinput buttons changed"));

				// xinput buttons convert exactly to synergy buttons
				m_screen->sendEvent(m_screen->getGameDeviceButtonsEvent(),
					new IPrimaryScreen::CGameDeviceButtonInfo(index, state.Gamepad.wButtons));

				eventSent = true;
			}

			if (leftStickXChanged || leftStickYChanged || rightStickXChanged || rightStickYChanged)
			{
				LOG((CLOG_DEBUG "xinput sticks changed"));

				m_screen->sendEvent(m_screen->getGameDeviceSticksEvent(),
					new IPrimaryScreen::CGameDeviceStickInfo(
					index,
					m_gameLeftStickXLast, m_gameLeftStickYLast,
					m_gameRightStickXLast, m_gameRightStickYLast));

				eventSent = true;
			}

			if (leftTriggerChanged || rightTriggerChanged)
			{
				LOG((CLOG_DEBUG "xinput triggers changed"));

				// @todo seems wrong re-using x/y for a single value...
				m_screen->sendEvent(m_screen->getGameDeviceTriggersEvent(),
					new IPrimaryScreen::CGameDeviceTriggerInfo(
						index,
						state.Gamepad.bLeftTrigger,
						state.Gamepad.bRightTrigger));

				eventSent = true;
			}

			if (/*eventSent && */!m_gameTimingWaiting && (ARCH->time() - m_gameLastTimingSent > .5))
			{
				m_screen->sendEvent(m_screen->getGameDeviceTimingReqEvent(), NULL);
				m_gameLastTimingSent = ARCH->time();
				m_gameTimingWaiting = true;

				LOG((CLOG_DEBUG "game device timing request at %.4f", m_gameLastTimingSent));
			}
		}

		UInt16 sleep = m_gamePollFreq + m_gamePollFreqAdjust;
		LOG((CLOG_DEBUG5 "xinput poll sleeping for %dms", sleep));
		Sleep(sleep);
	}
}

void
CMSWindowsXInput::xInputTimingThread(void*)
{
	LOG((CLOG_DEBUG "xinput timing thread started"));

	bool end = false;
	while (!end)
	{
		// if timing request was queued, a timing response is queued
		// when the xinput status is faked; if it was faked, go tell
		// the server when this happened.
		if (DequeueXInputTimingResp())
		{
			LOG((CLOG_DEBUG "dequeued game device timing response"));
			m_screen->sendEvent(m_screen->getGameDeviceTimingRespEvent(),
				new IPrimaryScreen::CGameDeviceTimingRespInfo(GetXInputFakeFreqMillis()));
		}

		// give the cpu a break.
		Sleep(1);
	}
}

void
CMSWindowsXInput::xInputFeedbackThread(void*)
{
	LOG((CLOG_DEBUG "xinput feedback thread started"));

	int index = 0;
	bool end = false;
	while (!end)
	{
		WORD leftMotor, rightMotor;
		if (DequeueXInputFeedback(&leftMotor, &rightMotor))
		{
			LOG((CLOG_DEBUG "dequeued game device feedback"));
			m_screen->sendEvent(m_screen->getGameDeviceFeedbackEvent(),
				new IPrimaryScreen::CGameDeviceFeedbackInfo(index, leftMotor, rightMotor));
		}

		Sleep(50);
	}
}
