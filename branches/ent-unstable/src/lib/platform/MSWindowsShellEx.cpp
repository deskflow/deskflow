/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2014 Bolton Software Ltd.
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

#include "platform/MSWindowsShellEx.h"

#include "synergy/XScreen.h"
#include "base/Log.h"

static const char* g_name = "synwinxt";

CMSWindowsShellEx::CMSWindowsShellEx() :
	m_getDraggingFilenameFunc(NULL),
	m_clearDraggingFilenameFunc(NULL),
	m_instance(NULL)
{
}

CMSWindowsShellEx::~CMSWindowsShellEx()
{
	if (m_instance != NULL) {
		FreeLibrary(m_instance);
	}
}

void
CMSWindowsShellEx::loadLibrary()
{
    OSVERSIONINFO osvi;
    ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&osvi);

    if (osvi.dwMajorVersion < 6) {
		LOG((CLOG_INFO "skipping shell extension library load, %s.dll not supported before vista", g_name));
        return;
    }

	// load library
	m_instance = LoadLibrary(g_name);
	if (m_instance == NULL) {
		LOG((CLOG_ERR "failed to load shell extension library, %s.dll is missing or invalid", g_name));
		throw XScreenOpenFailure();
	}

	// look up functions
	m_getDraggingFilenameFunc = (GetDraggingFilenameFunc)GetProcAddress(m_instance, "getDraggingFilename");
	m_clearDraggingFilenameFunc = (ClearDraggingFilenameFunc)GetProcAddress(m_instance, "clearDraggingFilename");

	if (m_getDraggingFilenameFunc == NULL ||
		m_clearDraggingFilenameFunc == NULL) {
		LOG((CLOG_ERR "failed to load shell extension function, %s.dll could be out of date", g_name));
		throw XScreenOpenFailure();
	}
}

HINSTANCE
CMSWindowsShellEx::getInstance() const
{
	return m_instance;
}

void
CMSWindowsShellEx::getDraggingFilename(char* filename) const
{
	if (m_getDraggingFilenameFunc == NULL) {
		return;
	}
	m_getDraggingFilenameFunc(filename);
}

void
CMSWindowsShellEx::clearDraggingFilename()
{
	if (m_getDraggingFilenameFunc == NULL) {
		return;
	}
	m_clearDraggingFilenameFunc();
}
