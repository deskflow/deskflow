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

#define WIN32_LEAN_AND_MEAN
#define SYNERGY_EXPORT_XINPUT_HOOKS
#define REQUIRED_XINPUT_DLL "xinput1_3.dll"
#define HOOK_TIMEOUT 10000 // 10 sec

#include <Windows.h>
#include <XInput.h>
#include "XInputHook.h"
#include "HookDLL.h"
#include <iostream>

HINSTANCE dll;
char name[256];

#pragma data_seg(".SHARED")

HHOOK s_hook = NULL;

// @todo use a struct for multiple gamepad support
WORD s_buttons = 0;
SHORT s_leftStickX = 0;
SHORT s_leftStickY = 0;
SHORT s_rightStickX = 0;
SHORT s_rightStickY = 0;
BYTE s_leftTrigger = 0;
BYTE s_rightTrigger = 0;
BOOL s_timingReqQueued = FALSE;
BOOL s_timingRespQueued = FALSE;
DWORD s_lastFakeMillis = 0;
WORD s_fakeFreqMillis = 0;
DWORD s_packetNumber = 0;
WORD s_leftMotor = 0;
WORD s_rightMotor = 0;
BOOL s_feedbackQueued = FALSE;

#pragma data_seg()

#pragma comment(linker, "/SECTION:.SHARED,RWS")

#include <sstream>
std::stringstream _xInputHookLogStream;
#define LOG(s) \
	_xInputHookLogStream.str(""); \
	_xInputHookLogStream << "Synergy XInputHook: " << s << endl; \
	OutputDebugString( _xInputHookLogStream.str().c_str())

using namespace std;

SDLLHook s_xInputHook =
{
	XINPUT_DLL,
	false, NULL,
	{
		{ (char*)(0x80000002), HookXInputGetState },
		{ (char*)(0x80000003), HookXInputSetState },
		{ (char*)(0x80000004), HookXInputGetCapabilities },
	}
};

BOOL APIENTRY
DllMain(HINSTANCE module, DWORD reason, LPVOID reserved)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		dll = module;

		// disable unwanted thread notifications to reduce overhead
		DisableThreadLibraryCalls(dll);

		GetModuleFileName(GetModuleHandle(NULL), name, sizeof(name));

		// don't hook synergys (this needs to detect real input)
		if (string(name).find("synergy") == string::npos)
		{
			LOG("checking '" << name << "' for " << XINPUT_DLL);
			HookAPICalls(&s_xInputHook);
		}
	}

	return TRUE;
}

void
SetXInputButtons(DWORD userIndex, WORD buttons)
{
	s_buttons = buttons;

	s_packetNumber++;

	LOG("SetXInputButtons: idx=" << userIndex << ", btns=" << buttons);
}

void
SetXInputSticks(DWORD userIndex, SHORT lx, SHORT ly, SHORT rx, SHORT ry)
{
	s_leftStickX = lx;
	s_leftStickY = ly;
	s_rightStickX = rx;
	s_rightStickY = ry;

	s_packetNumber++;

	LOG("SetXInputSticks:" <<
		" l=" << s_leftStickX << "," << s_leftStickY <<
		" r=" << s_rightStickX << "," << s_rightStickY);
}

void
SetXInputTriggers(DWORD userIndex, BYTE left, BYTE right)
{
	s_leftTrigger = left;
	s_rightTrigger = right;

	s_packetNumber++;

	LOG("SetXInputTriggers: " <<
		"l=" << (int)left << " r=" << (int)right);
}

void
QueueXInputTimingReq()
{
	s_timingReqQueued = TRUE;
}

BOOL
DequeueXInputTimingResp()
{
	BOOL result = s_timingRespQueued;
	s_timingRespQueued = FALSE;
	return result;
}

BOOL
DequeueXInputFeedback(WORD* leftMotor, WORD* rightMotor)
{
	if (s_feedbackQueued)
	{
		*leftMotor = s_leftMotor;
		*rightMotor = s_rightMotor;
		s_feedbackQueued = FALSE;
		return TRUE;
	}
	return FALSE;
}

WORD
GetXInputFakeFreqMillis()
{
	return s_fakeFreqMillis;
}

DWORD WINAPI
HookXInputGetState(DWORD userIndex, XINPUT_STATE* state)
{
	// @todo multiple device support
	if (userIndex != 0)
	{
		return ERROR_DEVICE_NOT_CONNECTED;
	}

	DWORD now = GetTickCount();
	s_fakeFreqMillis = (WORD)(now - s_lastFakeMillis);
	s_lastFakeMillis = now;

	state->dwPacketNumber = s_packetNumber;
	state->Gamepad.wButtons = s_buttons;
	state->Gamepad.bLeftTrigger = s_leftTrigger;
	state->Gamepad.bRightTrigger = s_rightTrigger;
	state->Gamepad.sThumbLX = s_leftStickX;
	state->Gamepad.sThumbLY = s_leftStickY;
	state->Gamepad.sThumbRX = s_rightStickX;
	state->Gamepad.sThumbRY = s_rightStickY;

	LOG("HookXInputGetState"
		<< ", idx=" << userIndex
		<< ", pkt=" << state->dwPacketNumber
		<< ", btn=" << state->Gamepad.wButtons
		<< ", t1=" << (int)state->Gamepad.bLeftTrigger
		<< ", t2=" << (int)state->Gamepad.bRightTrigger
		<< ", s1=" << state->Gamepad.sThumbLX << "," << state->Gamepad.sThumbLY
		<< ", s2=" << state->Gamepad.sThumbRX << "," << state->Gamepad.sThumbRY);

	if (s_timingReqQueued)
	{
		s_timingRespQueued = TRUE;
		s_timingReqQueued = FALSE;
		LOG("timing response queued");
	}

	return ERROR_SUCCESS;
}

DWORD WINAPI
HookXInputSetState(DWORD userIndex, XINPUT_VIBRATION* vibration)
{
	// @todo multiple device support
	if (userIndex != 0)
	{
		return ERROR_DEVICE_NOT_CONNECTED;
	}

	// only change values and queue feedback change if
	// feedback has actually changed.
	if ((s_leftMotor != vibration->wLeftMotorSpeed) ||
		(s_rightMotor != vibration->wRightMotorSpeed))
	{
		s_leftMotor = vibration->wLeftMotorSpeed;
		s_rightMotor = vibration->wRightMotorSpeed;
		s_feedbackQueued = TRUE;

		LOG("HookXInputSetState"
			", idx=" << userIndex <<
			", lm=" << s_leftMotor <<
			", rm=" << s_rightMotor);
	}

	return ERROR_SUCCESS;
}

DWORD WINAPI
HookXInputGetCapabilities(DWORD userIndex, DWORD flags, XINPUT_CAPABILITIES* capabilities)
{
	// @todo multiple device support
	if (userIndex != 0)
	{
		return ERROR_DEVICE_NOT_CONNECTED;
	}

	LOG("HookXInputGetCapabilities"
		", idx=" << userIndex <<
		", flags=" << flags);

	capabilities->Type = 1;
	capabilities->SubType = 1;
	capabilities->Flags = 4;
	capabilities->Gamepad.bLeftTrigger = 1;
	capabilities->Gamepad.bRightTrigger = 1;
	capabilities->Gamepad.sThumbLX = 1;
	capabilities->Gamepad.sThumbLY = 1;
	capabilities->Gamepad.sThumbRX = 1;
	capabilities->Gamepad.sThumbRY = 1;
	capabilities->Gamepad.wButtons = 62463;
	capabilities->Vibration.wLeftMotorSpeed = 1;
	capabilities->Vibration.wRightMotorSpeed = 1;

	return ERROR_SUCCESS;
}

synxinhk_API LRESULT CALLBACK
HookProc(int code, WPARAM wParam, LPARAM lParam) 
{
	return CallNextHookEx(s_hook, code, wParam, lParam);
}

synxinhk_API BOOL
InstallXInputHook()
{
	if (_stricmp(XINPUT_DLL, REQUIRED_XINPUT_DLL) != 0)
	{
		LOG("DLL not supported: " << XINPUT_DLL);
		return FALSE;
	}

	LOG("installing hook");
	s_hook = SetWindowsHookEx(WH_CBT, HookProc, dll, 0);
	LOG("hook installed");

	return TRUE;
}

synxinhk_API void
RemoveXInputHook()
{
	LOG("removing hook");
	UnhookWindowsHookEx(s_hook);
	LOG("hook removed");
}
