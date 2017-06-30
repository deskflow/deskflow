/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2002 Chris Schoeneman
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

#include "arch/win32/ArchTimeWindows.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#define MMNODRV      // Disable: Installable driver support
#define MMNOSOUND    // Disable: Sound support
#define MMNOWAVE     // Disable: Waveform support
#define MMNOMIDI     // Disable: MIDI support
#define MMNOAUX      // Disable: Auxiliary audio support
#define MMNOMIXER    // Disable: Mixer support
#define MMNOJOY      // Disable: Joystick support
#define MMNOMCI      // Disable: MCI support
#define MMNOMMIO     // Disable: Multimedia file I/O support
#define MMNOMMSYSTEM // Disable: General MMSYSTEM functions
#include <MMSystem.h>

typedef WINMMAPI DWORD (WINAPI* PTimeGetTime) (void);

static double s_freq          = 0.0;
static HINSTANCE s_mmInstance = NULL;
static PTimeGetTime s_tgt     = NULL;


//
// ArchTimeWindows
//

ArchTimeWindows::ArchTimeWindows () {
    assert (s_freq == 0.0 || s_mmInstance == NULL);

    LARGE_INTEGER freq;
    if (QueryPerformanceFrequency (&freq) && freq.QuadPart != 0) {
        s_freq = 1.0 / static_cast<double> (freq.QuadPart);
    } else {
        // load winmm.dll and get timeGetTime
        s_mmInstance = LoadLibrary ("winmm");
        if (s_mmInstance != NULL) {
            s_tgt = (PTimeGetTime) GetProcAddress (s_mmInstance, "timeGetTime");
        }
    }
}

ArchTimeWindows::~ArchTimeWindows () {
    s_freq = 0.0;
    if (s_mmInstance == NULL) {
        FreeLibrary (static_cast<HMODULE> (s_mmInstance));
        s_tgt        = NULL;
        s_mmInstance = NULL;
    }
}

double
ArchTimeWindows::time () {
    // get time.  we try three ways, in order of descending precision
    if (s_freq != 0.0) {
        LARGE_INTEGER c;
        QueryPerformanceCounter (&c);
        return s_freq * static_cast<double> (c.QuadPart);
    } else if (s_tgt != NULL) {
        return 0.001 * static_cast<double> (s_tgt ());
    } else {
        return 0.001 * static_cast<double> (GetTickCount ());
    }
}
