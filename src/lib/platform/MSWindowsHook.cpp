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

#include "platform/MSWindowsHook.h"

#include "synergy/XScreen.h"
#include "base/Log.h"

static const char* g_name = "synwinhk";

MSWindowsHook::MSWindowsHook ()
    : m_initFunc (NULL),
      m_cleanupFunc (NULL),
      m_setSidesFunc (NULL),
      m_setZoneFunc (NULL),
      m_setModeFunc (NULL),
      m_instance (NULL) {
}

MSWindowsHook::~MSWindowsHook () {
    cleanup ();

    if (m_instance != NULL) {
        FreeLibrary (m_instance);
    }
}

void
MSWindowsHook::loadLibrary () {
    // load library
    m_instance = LoadLibrary (g_name);
    if (m_instance == NULL) {
        LOG ((CLOG_ERR
              "failed to load hook library, %s.dll is missing or invalid",
              g_name));
        throw XScreenOpenFailure ();
    }

    // look up functions
    m_setSidesFunc = (SetSidesFunc) GetProcAddress (m_instance, "setSides");
    m_setZoneFunc  = (SetZoneFunc) GetProcAddress (m_instance, "setZone");
    m_setModeFunc  = (SetModeFunc) GetProcAddress (m_instance, "setMode");
    m_initFunc     = (InitFunc) GetProcAddress (m_instance, "init");
    m_cleanupFunc  = (CleanupFunc) GetProcAddress (m_instance, "cleanup");

    if (m_setSidesFunc == NULL || m_setZoneFunc == NULL ||
        m_setModeFunc == NULL || m_initFunc == NULL || m_cleanupFunc == NULL) {
        LOG ((CLOG_ERR
              "failed to load hook function, %s.dll could be out of date",
              g_name));
        throw XScreenOpenFailure ();
    }

    // initialize library
    if (init (GetCurrentThreadId ()) == 0) {
        LOG ((CLOG_ERR "failed to init %s.dll, another program may be using it",
              g_name));
        LOG ((CLOG_INFO "restarting your computer may solve this error"));
        throw XScreenOpenFailure ();
    }
}

HINSTANCE
MSWindowsHook::getInstance () const {
    return m_instance;
}

int
MSWindowsHook::init (DWORD threadID) {
    if (m_initFunc == NULL) {
        return NULL;
    }
    return m_initFunc (threadID);
}

int
MSWindowsHook::cleanup () {
    if (m_cleanupFunc == NULL) {
        return NULL;
    }
    return m_cleanupFunc ();
}

void
MSWindowsHook::setSides (UInt32 sides) {
    if (m_setSidesFunc == NULL) {
        return;
    }
    m_setSidesFunc (sides);
}

void
MSWindowsHook::setZone (SInt32 x, SInt32 y, SInt32 w, SInt32 h,
                        SInt32 jumpZoneSize) {
    if (m_setZoneFunc == NULL) {
        return;
    }
    m_setZoneFunc (x, y, w, h, jumpZoneSize);
}

void
MSWindowsHook::setMode (EHookMode mode) {
    if (m_setModeFunc == NULL) {
        return;
    }
    m_setModeFunc (mode);
}
