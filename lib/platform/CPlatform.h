/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2002 Chris Schoeneman
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef CPLATFORM_H
#define CPLATFORM_H

#include "common.h"

#if WINDOWS_LIKE

#include "CWin32Platform.h"
typedef CWin32Platform CPlatform;

#elif UNIX_LIKE

#include "CUnixPlatform.h"
typedef CUnixPlatform CPlatform;

#else

#error Unsupported platform

#endif

#endif
