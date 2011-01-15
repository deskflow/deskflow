/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2009 Chris Schoeneman, Nick Bolton, Sorin Sbarnea
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

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <string>

class CThread;

class CMSWindowsRelauncher {
public:
	CMSWindowsRelauncher();
	virtual ~CMSWindowsRelauncher();
	void startAsync();
	CThread* m_thread;
	void startThread(void*);
	BOOL winlogonInSession(DWORD sessionId, PHANDLE process);
	DWORD getSessionId();
	HANDLE getCurrentUserToken(DWORD sessionId, LPSECURITY_ATTRIBUTES security);
	int relaunchLoop();
	std::string getCommand();
};
