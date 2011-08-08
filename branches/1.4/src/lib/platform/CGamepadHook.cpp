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

#pragma data_seg(".HookSection")
HHOOK hook = NULL;
#pragma data_seg()

// TODO: use synergy logging
#include <sstream>
std::stringstream logStream2;
#define LOG(s) \
	logStream2.str(""); \
	logStream2 << s; \
	OutputDebugString( logStream2.str().c_str() );

using namespace std;

typedef DWORD (WINAPI *XInputGetState_Type)(DWORD dwUserIndex, XINPUT_STATE* pState);
DWORD WINAPI MyXInputGetState(DWORD dwUserIndex, XINPUT_STATE* pState);

enum
{
	XIFN_XInputGetState = 0
};

SDLLHook XInputHook =
{
	XINPUT_DLL,
	false, NULL,
	{
		{ (char*)(0x80000002), MyXInputGetState },
		{ "XInputGetState", MyXInputGetState },
	}
};

bool toggle = false;
DWORD WINAPI MyXInputGetState(DWORD userIndex, XINPUT_STATE* state)
{
	OutputDebugString("CGamepadHook: MyXInputGetState called\n");

	/*XINPUT_STATE origState;
	ZeroMemory(&origState, sizeof(XINPUT_STATE));
	XInputGetState_Type oldFn = (XInputGetState_Type)XInputHook.Functions[XIFN_XInputGetState].OrigFn;
	bool origResult = oldFn(userIndex, &origState);*/

	toggle = !toggle;
	state->Gamepad.wButtons = toggle ? XINPUT_GAMEPAD_START : XINPUT_GAMEPAD_A;

	return ERROR_SUCCESS;

	//XInputGetState_Type oldFn = (XInputGetState_Type)XInputHook.Functions[XIFN_XInputGetState].OrigFn;
	//return oldFn(userIndex, state);
}

BOOL APIENTRY DllMain(HINSTANCE module, DWORD reason, LPVOID reserved)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		dll = module;

		// disable unwanted thread notifications to reduce overhead
		DisableThreadLibraryCalls(dll);

		GetModuleFileName(GetModuleHandle(NULL), name, sizeof(name));

		LOG("CGamepadHook: checking process: "
			<< name << ", hook: " << XINPUT_DLL << endl);

		HookAPICalls(&XInputHook);
	}

	return TRUE;
}

sygpadhk_API LRESULT CALLBACK HookProc(int code, WPARAM wParam, LPARAM lParam) 
{
	return CallNextHookEx(hook, code, wParam, lParam);
}

sygpadhk_API BOOL InstallGamepadHook()
{
	if (_stricmp(XINPUT_DLL, REQUIRED_XINPUT_DLL) != 0)
	{
		LOG("CGamepadHook: DLL not supported: " << XINPUT_DLL << endl);
		return FALSE;
	}

	OutputDebugString("CGamepadHook: installing hook\n");
	hook = SetWindowsHookEx(WH_CBT, HookProc, dll, 0);
	OutputDebugString("CGamepadHook: hook installed\n");

	return TRUE;
}

sygpadhk_API void RemoveGamepadHook()
{
	OutputDebugString("CGamepadHook: removing hook\n");
	UnhookWindowsHookEx(hook);
	OutputDebugString("CGamepadHook: hook removed\n");
}
