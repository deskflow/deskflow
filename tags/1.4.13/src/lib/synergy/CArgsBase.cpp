/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Bolton Software Ltd.
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

#include "CArgsBase.h"

CArgsBase::CArgsBase() :
#if SYSAPI_WIN32
m_daemon(false), // daemon mode not supported on windows (use --service)
m_debugServiceWait(false),
m_pauseOnExit(false),
m_stopOnDeskSwitch(false),
#else
m_daemon(true), // backward compatibility for unix (daemon by default)
#endif
#if WINAPI_XWINDOWS
m_disableXInitThreads(false),
#endif
m_backend(false),
m_restartable(true),
m_noHooks(false),
m_disableTray(false),
m_pname(NULL),
m_logFilter(NULL),
m_logFile(NULL),
m_display(NULL),
m_enableIpc(false),
m_enableDragDrop(false)
{
}

CArgsBase::~CArgsBase()
{
}
