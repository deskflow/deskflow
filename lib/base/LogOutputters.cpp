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

#include "LogOutputters.h"
#include "CArch.h"

//
// CStopLogOutputter
//

CStopLogOutputter::CStopLogOutputter()
{
	// do nothing
}

CStopLogOutputter::~CStopLogOutputter()
{
	// do nothing
}

void
CStopLogOutputter::open(const char*)
{
	// do nothing
}

void
CStopLogOutputter::close()
{
	// do nothing
}

bool
CStopLogOutputter::write(ELevel, const char*)
{
	return false;
}

const char*
CStopLogOutputter::getNewline() const
{
	return "";
}


//
// CConsoleLogOutputter
//

CConsoleLogOutputter::CConsoleLogOutputter()
{
	// do nothing
}

CConsoleLogOutputter::~CConsoleLogOutputter()
{
	// do nothing
}

void
CConsoleLogOutputter::open(const char* title)
{
	ARCH->openConsole(title);
}

void
CConsoleLogOutputter::close()
{
	ARCH->closeConsole();
}

bool
CConsoleLogOutputter::write(ELevel, const char* msg)
{
	ARCH->writeConsole(msg);
	return true;
}

const char*
CConsoleLogOutputter::getNewline() const
{
	return ARCH->getNewlineForConsole();
}


//
// CSystemLogOutputter
//

CSystemLogOutputter::CSystemLogOutputter()
{
	// do nothing
}

CSystemLogOutputter::~CSystemLogOutputter()
{
	// do nothing
}

void
CSystemLogOutputter::open(const char* title)
{
	ARCH->openLog(title);
}

void
CSystemLogOutputter::close()
{
	ARCH->closeLog();
}

bool
CSystemLogOutputter::write(ELevel level, const char* msg)
{
	IArchLog::ELevel archLogLevel;
	switch (level) {
	case CLog::kFATAL:
	case CLog::kERROR:
		archLogLevel = IArchLog::kERROR;
		break;

	case CLog::kWARNING:
		archLogLevel = IArchLog::kWARNING;
		break;

	case CLog::kNOTE:
		archLogLevel = IArchLog::kNOTE;
		break;

	case CLog::kINFO:
		archLogLevel = IArchLog::kINFO;
		break;

	default:
		archLogLevel = IArchLog::kDEBUG;
		break;

	};
	ARCH->writeLog(archLogLevel, msg);
	return true;
}

const char*
CSystemLogOutputter::getNewline() const
{
	return "";
}


//
// CSystemLogger
//

CSystemLogger::CSystemLogger(const char* title)
{
	// redirect log messages
	m_syslog = new CSystemLogOutputter;
	m_stop   = new CStopLogOutputter;
	m_syslog->open(title);
	CLOG->insert(m_stop);
	CLOG->insert(m_syslog);
}

CSystemLogger::~CSystemLogger()
{
	CLOG->remove(m_syslog);
	CLOG->remove(m_stop);
	delete m_stop;
	delete m_syslog;
}
