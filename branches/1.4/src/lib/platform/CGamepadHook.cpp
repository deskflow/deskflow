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

#define REQUIRED_XINPUT_DLL "xinput1_3.dll"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "CGamepadHook.h"
#include "HookDLL.h"
#include <XInput.h>
#include <iostream>

HINSTANCE dll;
char name[256];

#pragma data_seg(".SHARED")
HHOOK s_hook = NULL;
WORD s_gamepadButtons = 0;
SHORT s_gamepadLeftStickX = 0;
SHORT s_gamepadLeftStickY = 0;
SHORT s_gamepadRightStickX = 0;
SHORT s_gamepadRightStickY = 0;
BYTE s_gamepadLeftTrigger = 0;
BYTE s_gamepadRightTrigger = 0;
#pragma data_seg()

#pragma comment(linker, "/SECTION:.SHARED,RWS")

// TODO: use synergy logging
#include <sstream>
std::stringstream logStream2;
#define LOG(s) \
	logStream2.str(""); \
	logStream2 << s; \
	OutputDebugString( logStream2.str().c_str() );

using namespace std;

typedef DWORD (WINAPI *XInputGetState_Type)(DWORD dwUserIndex, XINPUT_STATE* pState);
DWORD WINAPI hookXInputGetState(DWORD dwUserIndex, XINPUT_STATE* pState);

typedef DWORD (WINAPI *XInputGetCapabilities_Type)(DWORD userIndex, DWORD flags, XINPUT_CAPABILITIES* capabilities);
DWORD WINAPI hookXInputGetCapabilities(DWORD userIndex, DWORD flags, XINPUT_CAPABILITIES* capabilities);


enum
{
	XIFN_XInputGetState = 0,
	XIFN_XInputGetCapabilities = 1
};

SDLLHook s_xInputHook =
{
	XINPUT_DLL,
	false, NULL,
	{
		{ (char*)(0x80000002), hookXInputGetState },
		{ (char*)(0x80000004), hookXInputGetCapabilities },
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

		LOG("CGamepadHook: checking process: "
			<< name << ", hook: " << XINPUT_DLL << endl);

		// don't hook synergys (this needs to detect real input)
		if (string(name).find("synergys.exe") == string::npos)
		{
			HookAPICalls(&s_xInputHook);
		}
	}

	return TRUE;
}

void
hookGamepadButtonDown(WORD button)
{
	s_gamepadButtons |= button;
	LOG("CGamepadHook: hookGamepadButtonDown: " << s_gamepadButtons << endl);
}

void
hookGamepadButtonUp(WORD button)
{
	s_gamepadButtons ^= button;
	LOG("CGamepadHook: hookGamepadButtonUp: " << s_gamepadButtons << endl);
}

void
hookGamepadAnalog(BYTE id, SHORT x, SHORT y)
{
	LOG("CGamepadHook: hookGamepadAnalog: id=" << id << " " <<
		x << "," << y << endl);
	switch (id)
	{
	case 0: // kGamepadLeftStick
		s_gamepadLeftStickX = x;
		s_gamepadLeftStickY = y;
		break;

	case 1: // kGamepadRightStick
		s_gamepadRightStickX = x;
		s_gamepadRightStickY = y;
		break;

	case 2: // kGamepadLeftTrigger
		s_gamepadLeftTrigger = x;
		break;

	case 3: //kGamepadRightTrigger
		s_gamepadRightTrigger = x;
		break;
	}
}

DWORD WINAPI
hookXInputGetState(DWORD userIndex, XINPUT_STATE* state)
{
	if (userIndex != 0)
		return 1167; // @todo find macro for this

	LOG("CGamepadHook: hookXInputGetState index=" 
		<< userIndex <<  ", buttons=" << s_gamepadButtons << endl);

	state->Gamepad.wButtons = s_gamepadButtons;
	state->Gamepad.bLeftTrigger = s_gamepadLeftTrigger;
	state->Gamepad.bRightTrigger = s_gamepadRightTrigger;
	state->Gamepad.sThumbLX = s_gamepadLeftStickX;
	state->Gamepad.sThumbLY = s_gamepadLeftStickY;
	state->Gamepad.sThumbRX = s_gamepadRightStickX;
	state->Gamepad.sThumbRY = s_gamepadRightStickY;

	return ERROR_SUCCESS;
}

DWORD WINAPI
hookXInputGetCapabilities(DWORD userIndex, DWORD flags, XINPUT_CAPABILITIES* capabilities)
{
	if (userIndex != 0)
		return 1167; // @todo find macro for this

	LOG("CGamepadHook: hookXInputGetCapabilities id=" << userIndex <<
		", flags=" << flags << endl);

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

sygpadhk_API LRESULT CALLBACK
hookProc(int code, WPARAM wParam, LPARAM lParam) 
{
	return CallNextHookEx(s_hook, code, wParam, lParam);
}

sygpadhk_API BOOL
installGamepadHook()
{
	if (_stricmp(XINPUT_DLL, REQUIRED_XINPUT_DLL) != 0)
	{
		LOG("CGamepadHook: DLL not supported: " << XINPUT_DLL << endl);
		return FALSE;
	}

	OutputDebugString("CGamepadHook: installing hook\n");
	s_hook = SetWindowsHookEx(WH_CBT, hookProc, dll, 0);
	OutputDebugString("CGamepadHook: hook installed\n");

	return TRUE;
}

sygpadhk_API void
removeGamepadHook()
{
	OutputDebugString("CGamepadHook: removing hook\n");
	UnhookWindowsHookEx(s_hook);
	OutputDebugString("CGamepadHook: hook removed\n");
}
