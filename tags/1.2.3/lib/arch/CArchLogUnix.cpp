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

#include "CArchLogUnix.h"
#include <syslog.h>

//
// CArchLogUnix
//

CArchLogUnix::CArchLogUnix()
{
	// do nothing
}

CArchLogUnix::~CArchLogUnix()
{
	// do nothing
}

void
CArchLogUnix::openLog(const char* name)
{
	openlog(name, 0, LOG_DAEMON);
}

void
CArchLogUnix::closeLog()
{
	closelog();
}

void
CArchLogUnix::writeLog(ELevel level, const char* msg)
{
	// convert level
	int priority;
	switch (level) {
	case kERROR:
		priority = LOG_ERR;
		break;

	case kWARNING:
		priority = LOG_WARNING;
		break;

	case kNOTE:
		priority = LOG_NOTICE;
		break;

	case kINFO:
		priority = LOG_INFO;
		break;

	default:
		priority = LOG_DEBUG;
		break;
	}

	// log it
	syslog(priority, "%s", msg);
}
