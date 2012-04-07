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

#pragma once

#include "CString.h"

class CArgsBase {
public:
	CArgsBase();
	virtual ~CArgsBase();
	bool m_daemon;
	bool m_backend;
	bool m_restartable;
	bool m_noHooks;
	const char* m_pname;
	const char* m_logFilter;
	const char*	m_logFile;
	const char*	m_display;
	CString m_name;
	bool m_disableTray;
#if SYSAPI_WIN32
	bool m_relaunchMode;
	bool m_debugServiceWait;
	bool m_pauseOnExit;
	bool m_gameDevice;
#endif
#if WINAPI_XWINDOWS
	bool m_disableXInitThreads;
#endif
};
