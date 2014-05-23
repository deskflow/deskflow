/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2013 Bolton Software Ltd.
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

class CMSWindowsSession {
public:
	CMSWindowsSession();
	~CMSWindowsSession();

	//!
	/*!
	Returns true if the session ID has changed since updateActiveSession was called.
	*/
	BOOL				hasChanged();
	
	bool				isProcessInSession(const char* name, PHANDLE process);

	HANDLE				getUserToken(LPSECURITY_ATTRIBUTES security);

	DWORD				getActiveSessionId() { return m_activeSessionId; }

	void				updateActiveSession();

private:
	DWORD				m_activeSessionId;
};
