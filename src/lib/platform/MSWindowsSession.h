/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2013 Synergy Si Ltd.
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
 * 
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the OpenSSL
 * library.
 * You must obey the GNU General Public License in all respects for all of
 * the code used other than OpenSSL. If you modify file(s) with this
 * exception, you may extend this exception to your version of the file(s),
 * but you are not obligated to do so. If you do not wish to do so, delete
 * this exception statement from your version. If you delete this exception
 * statement from all source files in the program, then also delete it here.
 */

#pragma once

#include "base/String.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Tlhelp32.h>

class MSWindowsSession {
public:
	MSWindowsSession();
	~MSWindowsSession();

	//!
	/*!
	Returns true if the session ID has changed since updateActiveSession was called.
	*/
	BOOL				hasChanged();
	
	bool				isProcessInSession(const char* name, PHANDLE process);

	HANDLE				getUserToken(LPSECURITY_ATTRIBUTES security);

	DWORD				getActiveSessionId() { return m_activeSessionId; }

	void				updateActiveSession();

	String				getActiveDesktopName();

private:
	BOOL				nextProcessEntry(HANDLE snapshot, LPPROCESSENTRY32 entry);

private:
	DWORD				m_activeSessionId;
};
