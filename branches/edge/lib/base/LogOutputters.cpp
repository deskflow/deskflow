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
#include "TMethodJob.h"

#include <fstream>
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

void
CStopLogOutputter::show(bool)
{
	// do nothing
}

bool
CStopLogOutputter::write(ELevel, const char*)
{
	return false;
}


//
// CConsoleLogOutputter
//

CConsoleLogOutputter::CConsoleLogOutputter() :
m_threadCancel(false)
{
	m_writeThread = new CThread(new TMethodJob<CConsoleLogOutputter>(
		this, &CConsoleLogOutputter::writeThread));
}

CConsoleLogOutputter::~CConsoleLogOutputter()
{
	// queue cancel and wait for 3 seconds, then give up and delete
	m_threadCancel = true;
	m_writeThread->wait();
	delete m_writeThread;
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

void
CConsoleLogOutputter::show(bool showIfEmpty)
{
	ARCH->showConsole(showIfEmpty);
}

bool
CConsoleLogOutputter::write(ELevel level, const char* msg)
{
	// we want to ignore messages above CLOG->getConsoleMaxLevel(), since
	// the console can use a lot of CPU time to display messages, and on windows
	// this is done on the same thread.
	if (level <= CLOG->getConsoleMaxLevel()) {
		m_buffer.push_back(msg);
	}
	return true;
}

// in case our console is cpu hungry, buffer the log messages and dequeue 
// asynchronously in another thread. this way, if we hammer the console, 
// it won't cause the mouse to stutter.
void
CConsoleLogOutputter::writeThread(void*)
{
	// keep writing until it's safe to stop
	bool stop = false;
	while(!stop) {

		if (m_buffer.empty()) {

			// wait for some messages
			ARCH->sleep(.1);
			continue;
		}

		CString &s = m_buffer.front();
		ARCH->writeConsole(s.c_str());
		m_buffer.pop_front();

		// only cancel writer if all messages have been sent
		stop = m_buffer.empty() && m_threadCancel;
	}
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

void
CSystemLogOutputter::show(bool showIfEmpty)
{
	ARCH->showLog(showIfEmpty);
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

//
// CSystemLogger
//

CSystemLogger::CSystemLogger(const char* title, bool blockConsole) :
	m_stop(NULL)
{
	// redirect log messages
	if (blockConsole) {
		m_stop = new CStopLogOutputter;
		CLOG->insert(m_stop);
	}
	m_syslog = new CSystemLogOutputter;
	m_syslog->open(title);
	CLOG->insert(m_syslog);
}

CSystemLogger::~CSystemLogger()
{
	CLOG->remove(m_syslog);
	delete m_syslog;
	if (m_stop != NULL) {
		CLOG->remove(m_stop);
		delete m_stop;
	}
}


//
// CBufferedLogOutputter
//

CBufferedLogOutputter::CBufferedLogOutputter(UInt32 maxBufferSize) :
	m_maxBufferSize(maxBufferSize)
{
	// do nothing
}

CBufferedLogOutputter::~CBufferedLogOutputter()
{
	// do nothing
}

CBufferedLogOutputter::const_iterator
CBufferedLogOutputter::begin() const
{
	return m_buffer.begin();
}

CBufferedLogOutputter::const_iterator
CBufferedLogOutputter::end() const
{
	return m_buffer.end();
}

void
CBufferedLogOutputter::open(const char*)
{
	// do nothing
}

void
CBufferedLogOutputter::close()
{
	// remove all elements from the buffer
	m_buffer.clear();
}

void
CBufferedLogOutputter::show(bool)
{
	// do nothing
}

bool
CBufferedLogOutputter::write(ELevel, const char* message)
{
	while (m_buffer.size() >= m_maxBufferSize) {
		m_buffer.pop_front();
	}
	m_buffer.push_back(CString(message));
	return true;
}


//
// CFileLogOutputter
//

CFileLogOutputter::CFileLogOutputter(const char * logFile)
{
	assert(logFile != NULL);

	m_handle.open(logFile, std::fstream::app);
	// open file handle
}

CFileLogOutputter::~CFileLogOutputter()
{
	// close file handle
	if (m_handle.is_open())
		m_handle.close();
}

bool
CFileLogOutputter::write(ILogOutputter::ELevel level, const char *message)
{
	if (m_handle.is_open() && m_handle.fail() != true) {
		m_handle << message << std::endl;
		
		// write buffer to file
		m_handle.flush();
	}

	return true;
}

void
CFileLogOutputter::open(const char *title) {}

void
CFileLogOutputter::close() {}

void
CFileLogOutputter::show(bool showIfEmpty) {}