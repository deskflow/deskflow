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

#include "CVncClient.h"
#include "CThread.h"
#include "TMethodJob.h"

#if VNC_SUPPORT
#include "vnc/win/vncviewer/vncviewer.h"
#include "vnc/win/vncviewer/CConn.h"
#include "vnc/win/vncviewer/CConnThread.h"
#endif

CVncClient::CVncClient(const char* hostInfo, const std::string& screen) :
m_hostInfo(hostInfo),
m_screen(screen),
m_thread(NULL),
m_connThread(NULL)
{
}

CVncClient::~CVncClient()
{
	if (m_thread)
		delete m_thread;
}

void
CVncClient::thread(void*)
{
#if VNC_SUPPORT
	vncClientMain(this);
#endif
}

void
CVncClient::start()
{
	m_thread = new CThread(new TMethodJob<CVncClient>(
		this, &CVncClient::thread, NULL));
}

void
CVncClient::showViewer()
{
#if VNC_SUPPORT
	m_connThread->connRef->showViewer();
#endif
}

void
CVncClient::hideViewer()
{
#if VNC_SUPPORT
	m_connThread->connRef->hideViewer();
#endif
}
