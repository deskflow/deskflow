/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2004 Chris Schoeneman
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

#include "base/String.h"

#define WINDOWS_LEAN_AND_MEAN
#include <Windows.h>

class MSWindowsUtil {
public:
    //! Get message string
    /*!
    Gets a string for \p id from the string table of \p instance.
    */
    static String getString (HINSTANCE instance, DWORD id);

    //! Get error string
    /*!
    Gets a system error message for \p error.  If the error cannot be
    found return the string for \p id, replacing ${1} with \p error.
    */
    static String getErrorString (HINSTANCE, DWORD error, DWORD id);
};
