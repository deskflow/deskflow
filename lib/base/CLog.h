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

#ifndef CLOG_H
#define CLOG_H

#include "common.h"
#include <stdarg.h>

//! Logging facility
/*!
The logging class;  all console output should go through this class.
It supports multithread safe operation, several message priority levels,
filtering by priority, and output redirection.  The macros log() and
clog() provide convenient access.
*/
class CLog {
public:
	//! Log levels
	/*!
	The logging priority levels in order of highest to lowest priority.
	*/
	enum ELevel {
		kFATAL,			//!< For fatal errors
		kERROR,			//!< For serious errors
		kWARNING,		//!< For minor errors and warnings
		kNOTE,			//!< For messages about notable events
		kINFO,			//!< For informational messages
		kDEBUG,			//!< For important debugging messages
		kDEBUG1,		//!< For more detailed debugging messages
		kDEBUG2			//!< For even more detailed debugging messages
	};

	//! Outputter function.
	/*!
	Type of outputter function.  The outputter should write \c message,
	which has the given \c priority, to a log and return true.  Or it can
	return false to let CLog use the default outputter.
	*/
	typedef bool		(*Outputter)(int priority, const char* message);

	//! Locking function
	/*!
	Type of lock/unlock function.  If \c lock is true then block other
	threads that try to lock until this thread unlocks.  If \c lock is
	false then unlock and allow another (waiting) thread to lock.
	*/
	typedef void		(*Lock)(bool lock);

	//! @name manipulators
	//@{

	//! Set the function used to write the log
	/*!
	Sets the function used to write to the log.  The outputter function
	is called with the formatted string to write and the priority level.
	CLog will have already filtered messages below the current filter
	priority.  A NULL outputter means to use the default which is to print
	to stderr.  Note that the outputter should not call CLog methods but,
	if it does, the current lock function must permit recursive locks.
	*/
	static void			setOutputter(Outputter);

	//! Set the lock/unlock function
	/*!
	Set the lock/unlock function.  Use setLock(NULL) to remove the
	locking function.  There is no default lock function;  do not call
	CLog from multiple threads unless a working lock function has been
	installed.
	*/
	static void			setLock(Lock);

	//! Set the minimum priority filter.
	/*!
	Set the filter.  Messages below this priority are discarded.
	The default priority is 4 (INFO) (unless built without NDEBUG
	in which case it's 5 (DEBUG)).  The default can be overridden
	by setting the SYN_LOG_PRI env var to "FATAL", "ERROR", etc.
	setFilter(const char*) returns true if the priority \c name was
	recognized;  if \c name is NULL then it simply returns true.
	*/
	static bool			setFilter(const char* name);
	static void			setFilter(int);

	//@}
	//! @name accessors
	//@{

	//! Print a log message
	/*!
	Print a log message using the printf-like \c format and arguments.
	*/
	static void			print(const char* format, ...);

	//! Print a log message
	/*!
	Print a log message using the printf-like \c format and arguments
	preceded by the filename and line number.
	*/
	static void			printt(const char* file, int line,
							const char* format, ...);

	//! Get the function used to write the log
	static Outputter	getOutputter();

	//! Get the lock/unlock function
	/*!
	Get the lock/unlock function.  Note that the lock function is
	used when retrieving the lock function.
	*/
	static Lock			getLock();

	//! Get the minimum priority level.
	static int			getFilter();

	//@}

private:
	class CHoldLock {
	public:
		CHoldLock(Lock lock) : m_lock(lock) { m_lock(true); }
		~CHoldLock() { m_lock(false); }

	private:
		Lock			m_lock;
	};

	static void			dummyLock(bool);
	static int			getMaxPriority();
	static void			output(int priority, char* msg);
#if WINDOWS_LIKE
	static void			openConsole();
#endif

private:
	static Outputter	s_outputter;
	static Lock			s_lock;
	static int			s_maxPriority;
};

/*!
\def log(arg)
Write to the log.  Because macros cannot accept variable arguments, this
should be invoked like so:
\code
log((CLOG_XXX "%d and %d are %s", x, y, x == y ? "equal" : "not equal"));
\endcode
In particular, notice the double open and close parentheses.  Also note
that there is no comma after the \c CLOG_XXX.  The \c XXX should be
replaced by one of enumerants in \c CLog::ELevel without the leading
\c k.  For example, \c CLOG_INFO.  The special \c CLOG_PRINT level will
not be filtered and is never prefixed by the filename and line number.

If \c NOLOGGING is defined during the build then this macro expands to
nothing.  If \c NDEBUG is defined during the build then it expands to a
call to CLog::print.  Otherwise it expands to a call to CLog::printt,
which includes the filename and line number.
*/

/*!
\def logc(expr, arg)
Write to the log if and only if expr is true.  Because macros cannot accept
variable arguments, this should be invoked like so:
\code
clog(x == y, (CLOG_XXX "%d and %d are equal", x, y));
\endcode
In particular, notice the parentheses around everything after the boolean
expression.    Also note that there is no comma after the \c CLOG_XXX.
The \c XXX should be replaced by one of enumerants in \c CLog::ELevel
without the leading \c k.  For example, \c CLOG_INFO.  The special
\c CLOG_PRINT level will not be filtered and is never prefixed by the
filename and line number.

If \c NOLOGGING is defined during the build then this macro expands to
nothing.  If \c NDEBUG is defined during the build then it expands to a
call to CLog::print.  Otherwise it expands to a call to CLog::printt,
which includes the filename and line number.
*/

#if defined(NOLOGGING)
#define log(_a1)
#define logc(_a1, _a2)
#define CLOG_TRACE
#elif defined(NDEBUG)
#define log(_a1)		CLog::print _a1
#define logc(_a1, _a2)	if (_a1) CLog::print _a2
#define CLOG_TRACE
#else
#define log(_a1)		CLog::printt _a1
#define logc(_a1, _a2)	if (_a1) CLog::printt _a2
#define CLOG_TRACE		__FILE__, __LINE__,
#endif

#define CLOG_PRINT		CLOG_TRACE "%z\057"
#define CLOG_CRIT		CLOG_TRACE "%z\060"
#define CLOG_ERR		CLOG_TRACE "%z\061"
#define CLOG_WARN		CLOG_TRACE "%z\062"
#define CLOG_NOTE		CLOG_TRACE "%z\063"
#define CLOG_INFO		CLOG_TRACE "%z\064"
#define CLOG_DEBUG		CLOG_TRACE "%z\065"
#define CLOG_DEBUG1		CLOG_TRACE "%z\066"
#define CLOG_DEBUG2		CLOG_TRACE "%z\067"

#endif
