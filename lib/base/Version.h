#ifndef VERSION_H
#define VERSION_H

#include "common.h"

// set version macro if not set yet
#if !defined(VERSION)
#	define VERSION "1.0.0"
#endif

// important strings
static const char* kCopyright       = "Copyright (C) 2002 Chris Schoeneman";
static const char* kContact         = "Chris Schoeneman, crs23@bigfoot.com";
static const char* kWebsite         = "";

// build version.  follows linux kernel style:  an even minor number implies
// a release version, odd implies development version.
static const char* kVersion			= VERSION;

// exit codes
static const int kExitSuccess		= 0;	// successful completion
static const int kExitFailed		= 1;	// general failure
static const int kExitTerminated	= 2;	// killed by signal
static const int kExitArgs			= 3;	// bad arguments
static const int kExitConfig		= 4;	// cannot read configuration

#endif
