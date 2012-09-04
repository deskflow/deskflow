/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Bolton Software Ltd.
 * Copyright (C) 2011 Chris Schoeneman
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

#define SYNERGY_EXPORT_XINPUT_HOOKS
#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include "XInputProxy13.h"
#include "XInputHook.h"

#pragma comment(linker, "/EXPORT:XInputGetState=_XInputGetState@8,@2")
#pragma comment(linker, "/EXPORT:XInputSetState=_XInputSetState@8,@3")
#pragma comment(linker, "/EXPORT:XInputGetCapabilities=_XInputGetCapabilities@12,@4")
#pragma comment(linker, "/EXPORT:XInputEnable=_XInputEnable@4,@5")
#pragma comment(linker, "/EXPORT:XInputGetDSoundAudioDeviceGuids=_XInputGetDSoundAudioDeviceGuids@12,@6")
#pragma comment(linker, "/EXPORT:XInputGetBatteryInformation=_XInputGetBatteryInformation@12,@7")
#pragma comment(linker, "/EXPORT:XInputGetKeystroke=_XInputGetKeystroke@12,@8")

sxinpx13_API DWORD WINAPI
XInputGetState(DWORD dwUserIndex, XINPUT_STATE* pState)
{
	return HookXInputGetState(dwUserIndex, pState);
}

sxinpx13_API DWORD WINAPI
XInputSetState(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration)
{
	return HookXInputSetState(dwUserIndex, pVibration);
}

sxinpx13_API DWORD WINAPI
XInputGetCapabilities(DWORD dwUserIndex, DWORD dwFlags, XINPUT_CAPABILITIES* pCapabilities)
{
	return HookXInputGetCapabilities(dwUserIndex, dwFlags, pCapabilities);
}

sxinpx13_API void WINAPI
XInputEnable(BOOL enable)
{
}

sxinpx13_API DWORD WINAPI
XInputGetDSoundAudioDeviceGuids(DWORD dwUserIndex, GUID* pDSoundRenderGuid, GUID* pDSoundCaptureGuid)
{
	return ERROR_DEVICE_NOT_CONNECTED;
}

sxinpx13_API DWORD WINAPI
XInputGetBatteryInformation(DWORD dwUserIndex, BYTE devType, XINPUT_BATTERY_INFORMATION* pBatteryInformation)
{
	return ERROR_DEVICE_NOT_CONNECTED;
}

sxinpx13_API DWORD WINAPI
XInputGetKeystroke(DWORD dwUserIndex, DWORD dwReserved, PXINPUT_KEYSTROKE pKeystroke)
{
	return ERROR_DEVICE_NOT_CONNECTED;
}
