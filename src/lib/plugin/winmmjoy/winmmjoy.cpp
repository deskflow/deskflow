/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Synergy Si Ltd.
 * Copyright (C) 2012 Nick Bolton
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "winmmjoy.h"

#include <MMSystem.h>
#include <iostream>
#include <sstream>

#pragma comment(lib, "winmm.lib")

std::stringstream _logStream;
#define LOG(s) \
	_logStream.str(""); \
	_logStream << "winmmjoy: " << s << std::endl; \
	s_log(_logStream.str().c_str())

static bool s_running = true;
static void (*s_sendEvent)(const char*, void*) = NULL;
static void (*s_log)(const char*) = NULL;

extern "C" {

void
init(void* log, void* arch)
{
}

int
initEvent(void (*sendEvent)(const char*, void*))
{
	s_sendEvent = sendEvent;
	CreateThread(NULL, 0, mainLoop, NULL, 0, NULL);
	return 0;
}

void
cleanup()
{
	s_running = false;
}

}

DWORD WINAPI
mainLoop(void* data)
{
	// TODO: use a different message - e.g. DPLG%s (data - plugin)
	const char* buttonsEvent = "IPrimaryScreen::getGameDeviceButtonsEvent";
	const char* sticksEvent = "IPrimaryScreen::getGameDeviceSticksEvent";
	const char* triggersEvent = "IPrimaryScreen::getGameDeviceTriggersEvent";

	JOYINFOEX joyInfo;
	ZeroMemory(&joyInfo, sizeof(joyInfo));
	joyInfo.dwSize = sizeof(joyInfo);
	joyInfo.dwFlags = JOY_RETURNALL;

	// note: synergy data is often 16-bit, where winmm is 32-bit.
	UINT index = JOYSTICKID1;
	DWORD buttons, buttonsLast = 0;
	DWORD xPos, xPosLast = 0;
	DWORD yPos, yPosLast = 0;

	while (s_running) {
		
		if (joyGetPosEx(index, &joyInfo) != JOYERR_NOERROR) {
			Sleep(1000);
			continue;
		}

		buttons = joyInfo.dwButtons;
		xPos = joyInfo.dwXpos;
		yPos = joyInfo.dwYpos;

		if (buttons != buttonsLast) {
			s_sendEvent(buttonsEvent,
				new CGameDeviceButtonInfo(index, (GameDeviceButton)joyInfo.dwButtons));
		}

		if (xPos != xPosLast || yPos != yPosLast) {
			s_sendEvent(sticksEvent,
				new CGameDeviceStickInfo(index, (short)xPos, (short)yPos, 0, 0));
		}

		buttonsLast = buttons;
		xPosLast = xPos;
		yPosLast = yPos;
		Sleep(1);
	}
	return 0;
}
