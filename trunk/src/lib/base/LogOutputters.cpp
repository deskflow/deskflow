/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2002 Chris Schoeneman, Nick Bolton, Sorin Sbarnea
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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

CConsoleLogOutputter::CConsoleLogOutputter()
{
}

CConsoleLogOutputter::~CConsoleLogOutputter()
{
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
	ARCH->writeConsole(level, msg);
	return true;
}

void
CConsoleLogOutputter::flush()
{

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
	ARCH->writeLog(level, msg);
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

CFileLogOutputter::CFileLogOutputter(const char* logFile)
{
	assert(logFile != NULL);
	m_fileName = logFile;
}

CFileLogOutputter::~CFileLogOutputter()
{
}

bool
CFileLogOutputter::write(ELevel level, const char *message)
{
	std::ofstream m_handle;
	m_handle.open(m_fileName.c_str(), std::fstream::app);
	if (m_handle.is_open() && m_handle.fail() != true) {
		m_handle << message << std::endl;
	}
	m_handle.close();

	return true;
}

void
CFileLogOutputter::open(const char *title) {}

void
CFileLogOutputter::close() {}

void
CFileLogOutputter::show(bool showIfEmpty) {}