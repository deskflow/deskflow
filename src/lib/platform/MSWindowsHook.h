/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2011 Chris Schoeneman
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
 */

#pragma once

#include "synwinhk/synwinhk.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

//! Loads and provides functions for the Windows hook
class MSWindowsHook {
public:
    MSWindowsHook ();
    virtual ~MSWindowsHook ();

    void loadLibrary ();
    HINSTANCE getInstance () const;
    int init (DWORD threadID);
    int cleanup ();
    void setSides (UInt32 sides);
    void setZone (SInt32 x, SInt32 y, SInt32 w, SInt32 h, SInt32 jumpZoneSize);
    void setMode (EHookMode mode);

private:
    InitFunc m_initFunc;
    CleanupFunc m_cleanupFunc;
    SetSidesFunc m_setSidesFunc;
    SetZoneFunc m_setZoneFunc;
    SetModeFunc m_setModeFunc;
    HINSTANCE m_instance;
};
