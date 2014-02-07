/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Bolton Software Ltd.
 * Copyright (C) 2011 Chris Schoeneman
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

#ifndef CMSWINDOWSHOOKLIBRARYLOADER_H
#define CMSWINDOWSHOOKLIBRARYLOADER_H

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "synwinhk.h"
#include "synwinxt.h"

//! Loads Windows hook DLLs.
class CMSWindowsHookLibraryLoader
{
public:
	CMSWindowsHookLibraryLoader();
	virtual ~CMSWindowsHookLibraryLoader();

	HINSTANCE			openHookLibrary(const char* name);
	HINSTANCE			openShellLibrary(const char* name);

	// TODO: either make these private or expose properly
	InitFunc			m_init;
	CleanupFunc			m_cleanup;
	SetSidesFunc		m_setSides;
	SetZoneFunc			m_setZone;
	SetModeFunc			m_setMode;

	GetDraggingFilename	m_getDraggingFilename;
};

#endif