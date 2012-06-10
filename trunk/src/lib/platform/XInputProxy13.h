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

#pragma once

#ifdef sxinpx13_EXPORTS
#define sxinpx13_API __declspec(dllexport)
#else
#define sxinpx13_API __declspec(dllimport)
#endif

#include "XInput13.h"

#ifdef __cplusplus
extern "C" {
#endif

sxinpx13_API DWORD WINAPI XInputGetState
(
    __in  DWORD         dwUserIndex,  // Index of the gamer associated with the device
    __out XINPUT_STATE* pState		  // Receives the current state
);

sxinpx13_API DWORD WINAPI XInputSetState
(
    __in DWORD             dwUserIndex,  // Index of the gamer associated with the device
    __in XINPUT_VIBRATION* pVibration    // The vibration information to send to the controller
);

sxinpx13_API DWORD WINAPI XInputGetCapabilities
(
    __in  DWORD                dwUserIndex,   // Index of the gamer associated with the device
    __in  DWORD                dwFlags,       // Input flags that identify the device type
    __out XINPUT_CAPABILITIES* pCapabilities  // Receives the capabilities
);

sxinpx13_API void WINAPI XInputEnable
(
    __in BOOL enable     // [in] Indicates whether xinput is enabled or disabled. 
);

sxinpx13_API DWORD WINAPI XInputGetDSoundAudioDeviceGuids
(
    __in  DWORD dwUserIndex,          // Index of the gamer associated with the device
    __out GUID* pDSoundRenderGuid,    // DSound device ID for render
    __out GUID* pDSoundCaptureGuid    // DSound device ID for capture
);

sxinpx13_API DWORD WINAPI XInputGetBatteryInformation
(
    __in  DWORD                       dwUserIndex,        // Index of the gamer associated with the device
    __in  BYTE                        devType,            // Which device on this user index
    __out XINPUT_BATTERY_INFORMATION* pBatteryInformation // Contains the level and types of batteries
);

sxinpx13_API DWORD WINAPI XInputGetKeystroke
(
    __in       DWORD dwUserIndex,              // Index of the gamer associated with the device
    __reserved DWORD dwReserved,               // Reserved for future use
    __out      PXINPUT_KEYSTROKE pKeystroke    // Pointer to an XINPUT_KEYSTROKE structure that receives an input event.
);

#ifdef __cplusplus
}
#endif