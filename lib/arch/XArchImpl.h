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

#ifndef XARCHIMPL_H
#define XARCHIMPL_H

// include appropriate architecture implementation
#if WINDOWS_LIKE
#	include "XArchWindows.h"
#elif UNIX_LIKE
#	include "XArchUnix.h"
#endif

#endif
