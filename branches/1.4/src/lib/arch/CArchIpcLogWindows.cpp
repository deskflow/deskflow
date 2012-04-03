/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2002 Chris Schoeneman, Nick Bolton, Sorin Sbarnea
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

#include "CArchIpcLogWindows.h"
#include "CArchMiscWindows.h"
#include "XArch.h"
#include "CThread.h"
#include "TMethodJob.h"
#include "CArch.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

//
// CArchIpcLogWindows
//

CArchIpcLogWindows::CArchIpcLogWindows() :
	m_pipe(NULL),
	m_listenThread(NULL),
	m_connected(false)
{
}

CArchIpcLogWindows::~CArchIpcLogWindows()
{
	if (m_listenThread != NULL)
		delete m_listenThread;
}

void
CArchIpcLogWindows::openLog(const char* name)
{
	// grant access to everyone.
	SECURITY_DESCRIPTOR sd;
	InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
	SetSecurityDescriptorDacl(&sd, TRUE, static_cast<PACL>(0), FALSE);

	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = &sd;

	HANDLE pipe = CreateNamedPipe(
		TEXT("\\\\.\\pipe\\SynergyLog"),
		PIPE_ACCESS_DUPLEX,
		PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
		PIPE_UNLIMITED_INSTANCES,
		1024, 1024, 0, &sa);

	if (pipe == INVALID_HANDLE_VALUE)
		XArch("could not create named pipe.");

	m_pipe = pipe;

	m_listenThread = new CThread(new TMethodJob<CArchIpcLogWindows>(
		this, &CArchIpcLogWindows::connectThread, nullptr));
}

void
CArchIpcLogWindows::connectThread(void*)
{
	// HACK: this seems like a hacky pile of bollocks. it will continuously call
	// ConnectNamedPipe every second. if there is no client, it will block,
	// but if there is a client it will return FALSE and GetLastError() will
	// be ERROR_PIPE_CONNECTED. in any other case, the client has gone away
	// and ConnectNamedPipe will go back to blocking (waiting for the client
	// to reconnect).
	while (true) {
		BOOL result = ConnectNamedPipe(m_pipe, NULL);
		if ((result == TRUE) || (GetLastError() == ERROR_PIPE_CONNECTED)) {
			m_connected = true;
			ARCH->sleep(1);
		} else {
			DisconnectNamedPipe(m_pipe);
		}
	}
}

void
CArchIpcLogWindows::closeLog()
{
}

void
CArchIpcLogWindows::showLog(bool)
{
}

void
CArchIpcLogWindows::writeLog(ELevel level, const char* data)
{
	if (!m_connected)
		return;

	DWORD bytesWritten;
	WriteFile(m_pipe, data, (DWORD)strlen(data), &bytesWritten, NULL);
}
