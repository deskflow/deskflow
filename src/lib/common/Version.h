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

#include "common/common.h"

// set version macro if not set yet
#if !defined(SYNERGY_VERSION)
#error Version was not set (should be passed to compiler).
#endif

// important strings
extern const char* kApplication;
extern const char* kCopyright;
extern const char* kContact;
extern const char* kWebsite;

// build version.  follows linux kernel style:  an even minor number implies
// a release version, odd implies development version.
extern const char* kVersion;

// application version
extern const char* kAppVersion;
