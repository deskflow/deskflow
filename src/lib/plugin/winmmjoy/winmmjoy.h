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

#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#if defined(winmmjoy_EXPORTS)
#define WINMMJOY_API __declspec(dllexport)
#else
#define WINMMJOY_API __declspec(dllimport)
#endif

extern "C" {

WINMMJOY_API void		init(void* log, void* arch);
WINMMJOY_API int		initEvent(void (*sendEvent)(const char*, void*));
WINMMJOY_API void		cleanup();

}

DWORD WINAPI mainLoop(void* data);

typedef unsigned char GameDeviceID;
typedef unsigned short GameDeviceButton;

class CGameDeviceButtonInfo {
public:
	CGameDeviceButtonInfo(GameDeviceID id, GameDeviceButton buttons) :
	  m_id(id), m_buttons(buttons) { }
public:
	GameDeviceID m_id;
	GameDeviceButton m_buttons;
};

class CGameDeviceStickInfo {
public:
	CGameDeviceStickInfo(GameDeviceID id, short x1, short y1, short x2, short y2) :
	  m_id(id), m_x1(x1), m_x2(x2), m_y1(y1), m_y2(y2) { }
public:
	GameDeviceID m_id;
	short m_x1;
	short m_x2;
	short m_y1;
	short m_y2;
};

class CGameDeviceTriggerInfo {
public:
	CGameDeviceTriggerInfo(GameDeviceID id, unsigned char t1, unsigned char t2) :
	  m_id(id), m_t1(t1), m_t2(t2) { }
public:
	GameDeviceID m_id;
	unsigned char m_t1;
	unsigned char m_t2;
};
