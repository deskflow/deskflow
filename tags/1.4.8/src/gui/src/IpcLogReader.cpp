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

#include "IpcLogReader.h"

#if defined(Q_OS_WIN)
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

IpcLogReader::IpcLogReader()
{
}

IpcLogReader::~IpcLogReader()
{
}

void
IpcLogReader::run()
{
#if defined(Q_OS_WIN)

	const WCHAR* name = L"\\\\.\\pipe\\SynergyLog";

	HANDLE pipe = CreateFile(
		name, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);

	if (pipe == INVALID_HANDLE_VALUE)
	{
		receivedLine(
			QString("ERROR: could not connect to service log, error: ") +
			QString::number(GetLastError()));
		return;
	}

	char buffer[1024];
	DWORD bytesRead;

	while (true)
	{
		if (!ReadFile(pipe, buffer, sizeof(buffer), &bytesRead, NULL)) {
			break;
		}

		buffer[bytesRead] = '\0';

		QString text = QString::fromAscii(buffer, bytesRead);
		text = text.trimmed().append("\n");
		receivedLine(text);
	}
#endif
}
