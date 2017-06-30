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

#pragma once

#if defined(_WIN32)
#define SYSAPI_WIN32 1
#define WINAPI_MSWINDOWS 1
#elif HAVE_CONFIG_H
#include "config.h"
#else
#error "config.h missing"
#endif

// VC++ has built-in sized types
#if defined(_MSC_VER)
#include <wchar.h>
#define TYPE_OF_SIZE_1 __int8
#define TYPE_OF_SIZE_2 __int16
#define TYPE_OF_SIZE_4 __int32
#else
#define SIZE_OF_CHAR 1
#define SIZE_OF_SHORT 2
#define SIZE_OF_INT 4
#define SIZE_OF_LONG 4
#endif

// define NULL
#include <stddef.h>

// make assert available since we use it a lot
#include <assert.h>
#include <stdlib.h>
#include <string.h>

enum {
    kExitSuccess      = 0, // successful completion
    kExitFailed       = 1, // general failure
    kExitTerminated   = 2, // killed by signal
    kExitArgs         = 3, // bad arguments
    kExitConfig       = 4, // cannot read configuration
    kExitSubscription = 5  // subscription error
};
