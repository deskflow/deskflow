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
#define SYNERGY_MSG_MOUSE_MOVE		WM_APP + 0x0014	// x; y
#define SYNERGY_MSG_MOUSE_WHEEL		WM_APP + 0x0015	// delta; <unused>

extern "C" {

typedef int				(*InstallFunc)(DWORD targetQueueThreadID);
typedef int				(*UninstallFunc)(void);
typedef void			(*SetZoneFunc)(UInt32,
							SInt32, SInt32, SInt32, SInt32, SInt32);
typedef void			(*SetRelayFunc)(void);

CSYNERGYHOOK_API int	install(DWORD);
CSYNERGYHOOK_API int	uninstall(void);
CSYNERGYHOOK_API void	setZone(UInt32 sides,
							SInt32 x, SInt32 y, SInt32 w, SInt32 h,
							SInt32 jumpZoneSize);
CSYNERGYHOOK_API void	setRelay(void);

}

#endif
