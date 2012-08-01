/*
 * synergy -- mouse and keyboard sharing utility
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

#include "winmmjoy.h"

#include <MMSystem.h>
#include <iostream>
#include <sstream>

#pragma comment(lib, "winmm.lib")

std::stringstream _logStream;
#define LOG(s) \
	_logStream.str(""); \
	_logStream << "winmmjoy: " << s << std::endl; \
	s_log( _logStream.str().c_str())

static bool s_running = true;
static void (*s_sendEvent)(const char*, void*) = NULL;
static void (*s_log)(const char*) = NULL;

extern "C" {

int
init(void (*sendEvent)(const char*, void*), void (*log)(const char*))
{
	s_sendEvent = sendEvent;
	s_log = log;
	LOG("init");
	CreateThread(NULL, 0, mainLoop, NULL, 0, NULL);
	return 0;
}

int
cleanup()
{
	LOG("cleanup");
	s_running = false;
	return 0;
}

}

// convert winmm joystick value to sign 8 bit
char
convert_stick(int val)
{
	// convert to signed byte
	int sb = (val / 256) - 128;

	// clamp and return
	if (sb < -127)
		return -127;
	else if (sb > 127)
		return 127;
	else
		return sb;
}

// calculate absolute difference between values difference
int
abs_diff(int val, int reference)
{
	int diff = val-reference;
	return (diff < 0) ? -diff : diff;
}

// winmm joystick driver
DWORD WINAPI
mainLoop(void* data)
{
	const char* buttonsEvent = "IPrimaryScreen::getGameDeviceButtonsEvent";
	const char* sticksEvent = "IPrimaryScreen::getGameDeviceSticksEvent";
	const char* triggersEvent = "IPrimaryScreen::getGameDeviceTriggersEvent";

	JOYINFOEX joyInfo;
	ZeroMemory(&joyInfo, sizeof(joyInfo));
	joyInfo.dwSize = sizeof(joyInfo);
	joyInfo.dwFlags = JOY_RETURNALL;

	// note: synergy data is often 8-bit, where winmm is 32-bit.
	UINT index = JOYSTICKID1;
	DWORD buttons, buttonsLast = 0;
	DWORD xPos1, xPos1Last = 0;
	DWORD yPos1, yPos1Last = 0;
	DWORD xPos2, xPos2Last = 0;
	DWORD yPos2, yPos2Last = 0;

	while (s_running) {
		
		// sleep if no joystick is connected
		if (joyGetPosEx(index, &joyInfo) != JOYERR_NOERROR) {
			Sleep(1000);
			continue;
		}

		// convert POV to discrete four buttons (for joypads that return the four direction buttons as POV)
		int pov = 0;
		switch (joyInfo.dwPOV) {
			case 0:		pov = 0x1;			break;								// up
			case 4500:	pov = 0x1 | 0x2;	break;								// up-right
			case 9000:	pov = 0x2;			break;								// right
			case 13500:	pov = 0x2 | 0x4;	break;								// right-down
			case 18000:	pov = 0x4;			break;								// down
			case 22500:	pov = 0x4 | 0x8;	break;								// down-left
			case 27000:	pov = 0x8;			break;								// left
			case 31500:	pov = 0x8 | 0x1;	break;								// up-left
			case 65536: pov = 0;			break;								// none
		}

		// update button changes and map POV hat to four discrete buttons
		buttons = (joyInfo.dwButtons & 4095) | (pov << 12);						// mask off 12 buttons to pass on transparently
		if (buttons != buttonsLast) {
			s_sendEvent(buttonsEvent,
				new CGameDeviceButtonInfo(index, (GameDeviceButton)buttons));
			
			buttonsLast = buttons;
		}

		// update stick changes
		xPos1 = convert_stick(joyInfo.dwXpos);
		yPos1 = convert_stick(joyInfo.dwYpos);
		xPos2 = convert_stick(joyInfo.dwZpos);
		yPos2 = convert_stick(joyInfo.dwRpos);
		if ((abs_diff(xPos1, xPos1Last) > 2) ||
			(abs_diff(yPos1, yPos1Last) > 2) ||
			(abs_diff(xPos2, xPos2Last) > 2) ||
			(abs_diff(yPos2, yPos2Last) > 2)) {
			s_sendEvent(sticksEvent,
				new CGameDeviceStickInfo(index, (char)xPos1, (char)yPos1, (char)xPos2, (char)yPos2));
		
			xPos1Last = xPos1;
			yPos1Last = yPos1;
			xPos2Last = xPos2;
			yPos2Last = yPos2;
		}

		Sleep(1);
	}
	return 0;
}
