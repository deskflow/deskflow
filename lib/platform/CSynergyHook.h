/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2002 Chris Schoeneman
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef CSYNERGYHOOK_H
#define CSYNERGYHOOK_H

#include "BasicTypes.h"

#if WINDOWS_LIKE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#error CSynergyHook is a win32 specific file
#endif

#if defined(SYNRGYHK_EXPORTS)
#define CSYNERGYHOOK_API __declspec(dllexport)
#else
#define CSYNERGYHOOK_API __declspec(dllimport)
#endif

#define SYNERGY_MSG_MARK			WM_APP + 0x0011	// mark id; <unused>
#define SYNERGY_MSG_KEY				WM_APP + 0x0012	// vk code; key data
#define SYNERGY_MSG_MOUSE_BUTTON	WM_APP + 0x0013	// button msg; <unused>
#define SYNERGY_MSG_MOUSE_WHEEL		WM_APP + 0x0014	// delta; <unused>
#define SYNERGY_MSG_MOUSE_MOVE		WM_APP + 0x0015	// x; y
#define SYNERGY_MSG_POST_WARP		WM_APP + 0x0016	// <unused>; <unused>
#define SYNERGY_MSG_PRE_WARP		WM_APP + 0x0017	// x; y
#define SYNERGY_MSG_SCREEN_SAVER	WM_APP + 0x0018	// activated; <unused>
#define SYNERGY_MSG_INPUT_FIRST		SYNERGY_MSG_KEY
#define SYNERGY_MSG_INPUT_LAST		SYNERGY_MSG_PRE_WARP

extern "C" {

enum EHookResult {
	kHOOK_FAILED,
	kHOOK_OKAY,
	kHOOK_OKAY_LL
};

typedef int				(*InitFunc)(DWORD targetQueueThreadID);
typedef int				(*CleanupFunc)(void);
typedef EHookResult		(*InstallFunc)(void);
typedef int				(*UninstallFunc)(void);
typedef int				(*InstallScreenSaverFunc)(void);
typedef int				(*UninstallScreenSaverFunc)(void);
typedef void			(*SetSidesFunc)(UInt32);
typedef void			(*SetZoneFunc)(SInt32, SInt32, SInt32, SInt32, SInt32);
typedef void			(*SetRelayFunc)(int);

CSYNERGYHOOK_API int	init(DWORD);
CSYNERGYHOOK_API int	cleanup(void);
CSYNERGYHOOK_API EHookResult	install(void);
CSYNERGYHOOK_API int	uninstall(void);
CSYNERGYHOOK_API int	installScreenSaver(void);
CSYNERGYHOOK_API int	uninstallScreenSaver(void);
CSYNERGYHOOK_API void	setSides(UInt32 sides);
CSYNERGYHOOK_API void	setZone(SInt32 x, SInt32 y, SInt32 w, SInt32 h,
							SInt32 jumpZoneSize);
CSYNERGYHOOK_API void	setRelay(int enable);	// relay iff enable != 0

}

#endif
