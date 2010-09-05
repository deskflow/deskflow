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

#ifndef VERSION_H
#define VERSION_H

#include "common.h"

// set version macro if not set yet
#if !defined(VERSION)
#	define VERSION "1.0.14"
#endif

// important strings
static const char* kApplication     = "synergy";
static const char* kCopyright       = "Copyright (C) 2002 Chris Schoeneman";
static const char* kContact         = "Chris Schoeneman, crs23@bigfoot.com";
static const char* kWebsite         = "http://synergy2.sourceforge.net/";

// build version.  follows linux kernel style:  an even minor number implies
// a release version, odd implies development version.
static const char* kVersion			= VERSION;

// application version
static const char* kAppVersion      = "synergy " VERSION;

// exit codes
static const int kExitSuccess		= 0;	// successful completion
static const int kExitFailed		= 1;	// general failure
static const int kExitTerminated	= 2;	// killed by signal
static const int kExitArgs			= 3;	// bad arguments
static const int kExitConfig		= 4;	// cannot read configuration

#endif
