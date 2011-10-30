/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2011 Chris Schoeneman, Nick Bolton, Sorin Sbarnea
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

#include "CMSWindowsHookLibraryLoader.h"
#include "XScreen.h"
#include "CLog.h"

CMSWindowsHookLibraryLoader::CMSWindowsHookLibraryLoader() :
	m_init(NULL),
	m_cleanup(NULL),
	m_setSides(NULL),
	m_setZone(NULL),
	m_setMode(NULL)
{
}

CMSWindowsHookLibraryLoader::~CMSWindowsHookLibraryLoader()
{
	// TODO: take ownership of m_ and delete them.
}

HINSTANCE
CMSWindowsHookLibraryLoader::openHookLibrary(const char* name)
{
	// load the hook library
	HINSTANCE hookLibrary = LoadLibrary(name);
	if (hookLibrary == NULL) {
		LOG((CLOG_ERR "Failed to load hook library;  %s.dll is missing", name));
		throw XScreenOpenFailure();
	}

	// look up functions
	m_setSides  = (SetSidesFunc)GetProcAddress(hookLibrary, "setSides");
	m_setZone   = (SetZoneFunc)GetProcAddress(hookLibrary, "setZone");
	m_setMode   = (SetModeFunc)GetProcAddress(hookLibrary, "setMode");
	m_init      = (InitFunc)GetProcAddress(hookLibrary, "init");
	m_cleanup   = (CleanupFunc)GetProcAddress(hookLibrary, "cleanup");
	if (m_setSides             == NULL ||
		m_setZone              == NULL ||
		m_setMode              == NULL ||
		m_init                 == NULL ||
		m_cleanup              == NULL) {
			LOG((CLOG_ERR "Invalid hook library;  use a newer %s.dll", name));
			throw XScreenOpenFailure();
	}

	// initialize hook library
	if (m_init(GetCurrentThreadId()) == 0) {
		LOG((CLOG_ERR "Cannot initialize hook library;  is synergy already running?"));
		throw XScreenOpenFailure();
	}

	return hookLibrary;
}