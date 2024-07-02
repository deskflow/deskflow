/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2014-2016 Symless Ltd.
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

// HACK: gcc on osx106 doesn't give you an easy way to hide warnings
// from included headers, so use the system_header pragma. the downside
// is that everything in the header file following this also has warnings
// ignored, so we need to put it in a separate header file.
#if __APPLE__
#pragma GCC system_header
#endif

// gtest has a warning on osx106 (signed/unsigned int compare).
#include <gtest/gtest.h>
