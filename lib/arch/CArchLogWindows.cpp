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

#include "CArchLogWindows.h"
#include "CArchMiscWindows.h"
#include <string.h>

//
// CArchLogWindows
//

CArchLogWindows::CArchLogWindows() : m_eventLog(NULL)
{
	// do nothing
}

CArchLogWindows::~CArchLogWindows()
{
	// do nothing
}

void
CArchLogWindows::openLog(const char* name)
{
	if (m_eventLog == NULL && !CArchMiscWindows::isWindows95Family()) {
		m_eventLog = RegisterEventSource(NULL, name);
	}
}

void
CArchLogWindows::closeLog()
{
	if (m_eventLog != NULL) {
		DeregisterEventSource(m_eventLog);
		m_eventLog = NULL;
	}
}

void
CArchLogWindows::writeLog(ELevel level, const char* msg)
{
	if (m_eventLog != NULL) {
		// convert priority
		WORD type;
		switch (level) {
		case kERROR:
			type = EVENTLOG_ERROR_TYPE;
			break;

		case kWARNING:
			type = EVENTLOG_WARNING_TYPE;
			break;

		default:
			type = EVENTLOG_INFORMATION_TYPE;
			break;
		}

		// log it
		// FIXME -- win32 wants to use a message table to look up event
		// strings.  log messages aren't organized that way so we'll
		// just dump our string into the raw data section of the event
		// so users can at least see the message.  note that we use our
		// level as the event category.
		ReportEvent(m_eventLog, type, static_cast<WORD>(level),
								0,					// event ID
								NULL,
								0,
								strlen(msg) + 1,	// raw data size
								NULL,
								const_cast<char*>(msg));// raw data
	}
}
