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

#ifndef LOGOUTPUTTERS_H
#define LOGOUTPUTTERS_H

#include "ILogOutputter.h"

//! Stop traversing log chain outputter
/*!
This outputter performs no output and returns false from \c write(),
causing the logger to stop traversing the outputter chain.  Insert
this to prevent already inserted outputters from writing.
*/
class CStopLogOutputter : public ILogOutputter {
public:
	CStopLogOutputter();
	virtual ~CStopLogOutputter();

	// ILogOutputter overrides
	virtual void		open(const char* title);
	virtual void		close();
	virtual bool		write(ELevel level, const char* message);
	virtual const char*	getNewline() const;
};

//! Write log to console
/*!
This outputter writes output to the console.  The level for each
message is ignored.
*/
class CConsoleLogOutputter : public ILogOutputter {
public:
	CConsoleLogOutputter();
	virtual ~CConsoleLogOutputter();

	// ILogOutputter overrides
	virtual void		open(const char* title);
	virtual void		close();
	virtual bool		write(ELevel level, const char* message);
	virtual const char*	getNewline() const;
};

//! Write log to system log
/*!
This outputter writes output to the system log.
*/
class CSystemLogOutputter : public ILogOutputter {
public:
	CSystemLogOutputter();
	virtual ~CSystemLogOutputter();

	// ILogOutputter overrides
	virtual void		open(const char* title);
	virtual void		close();
	virtual bool		write(ELevel level, const char* message);
	virtual const char*	getNewline() const;
};

//! Write log to system log only
/*!
Creating an object of this type inserts a CStopLogOutputter followed
by a CSystemLogOutputter into CLog.  The destructor removes those
outputters.  Add one of these to any scope that needs to write to
the system log (only) and restore the old outputters when exiting
the scope.
*/
class CSystemLogger {
public:
	CSystemLogger(const char* title);
	~CSystemLogger();

private:
	ILogOutputter*		m_syslog;
	ILogOutputter*		m_stop;
};

#endif
