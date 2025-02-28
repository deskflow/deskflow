/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "arch/Arch.h"
#include "arch/IArchMultithread.h"
#include "common/common.h"
#include "common/stdlist.h"

#include <stdarg.h>

#define CLOG (Log::getInstance())
#define BYE "\nTry `%s --help' for more information."

class ILogOutputter;
class Thread;

//! Logging facility
/*!
The logging class;  all console output should go through this class.
It supports multithread safe operation, several message priority levels,
filtering by priority, and output redirection.  The macros LOG() and
LOGC() provide convenient access.
*/
class Log
{
public:
  Log(bool singleton = true);
  Log(Log *src);
  Log(Log const &) = delete;
  Log(Log &&) = delete;
  ~Log();

  Log &operator=(Log const &) = delete;
  Log &operator=(Log &&) = delete;

  //! @name manipulators
  //@{

  //! Add an outputter to the head of the list and adopts it.
  /*!
  Inserts an outputter to the head of the outputter list. The outputter
  is deleted when Log destructor is called.

  When the logger writes a message, it goes to the outputter at the head of
  the outputter list.  If that outputter's \c write() method returns
  true then it also goes to the next outputter, as so on until an
  outputter returns false or there are no more outputters.  Outputters
  still in the outputter list when the log is destroyed will be
  deleted.  If \c alwaysAtHead is true then the outputter is always
  called before all outputters with \c alwaysAtHead false and the
  return value of the outputter is ignored.

  By default, the logger has one outputter installed which writes to
  the console.
  */
  void insert(ILogOutputter *adoptedOutputter, bool alwaysAtHead = false);

  //! Remove an outputter from the list
  /*!
  Removes the first occurrence of the given outputter from the
  outputter list.  It does nothing if the outputter is not in the
  list.  The outputter is not deleted.
  */
  void remove(ILogOutputter *orphaned);

  //! Remove the outputter from the head of the list
  /*!
  Removes and deletes the outputter at the head of the outputter list.
  This does nothing if the outputter list is empty.  Only removes
  outputters that were inserted with the matching \c alwaysAtHead.
  */
  void pop_front(bool alwaysAtHead = false);

  //! Set the minimum priority filter.
  /*!
  Set the filter.  Messages below this priority are discarded.
  The default priority is 4 (INFO) (unless built without NDEBUG
  in which case it's 5 (DEBUG)).   setFilter(const char*) returns
  true if the priority \c name was recognized;  if \c name is NULL
  then it simply returns true.
  */
  bool setFilter(const char *name);

  //! Set the minimum priority filter (by ordinal).
  void setFilter(int);

  //@}
  //! @name accessors
  //@{

  //! Print a log message
  /*!
  Print a log message using the printf-like \c format and arguments
  preceded by the filename and line number.  If \c file is NULL then
  neither the file nor the line are printed.
  */
  void print(const char *file, int line, const char *format, ...);

  //! Get the minimum priority level.
  int getFilter() const;

  //! Get the filter name of the current filter level.
  const char *getFilterName() const;

  //! Get the filter name of a specified filter level.
  const char *getFilterName(int level) const;

  //! Get the singleton instance of the log
  static Log *getInstance();

  //! Get the console filter level (messages above this are not sent to
  //! console).
  int getConsoleMaxLevel() const
  {
    return kDEBUG2;
  }

  //@}

private:
  void output(ELevel priority, char *msg);

private:
  using OutputterList = std::list<ILogOutputter *>;

  static Log *s_log;

  ArchMutex m_mutex;
  OutputterList m_outputters;
  OutputterList m_alwaysOutputters;
  int m_maxPriority;
};

/*!
\def LOG(arg)
Write to the log.  Because macros cannot accept variable arguments, this
should be invoked like so:
\code
LOG((CLOG_XXX "%d and %d are %s", x, y, x == y ? "equal" : "not equal"));
\endcode
In particular, notice the double open and close parentheses.  Also note
that there is no comma after the \c CLOG_XXX.  The \c XXX should be
replaced by one of enumerants in \c Log::ELevel without the leading
\c k.  For example, \c CLOG_INFO.  The special \c CLOG_PRINT level will
not be filtered and is never prefixed by the filename and line number.

If \c NOLOGGING is defined during the build then this macro expands to
nothing.  If \c NDEBUG is defined during the build then it expands to a
call to Log::print.  Otherwise it expands to a call to Log::print,
which includes the filename and line number.
*/

/*!
\def LOGC(expr, arg)
Write to the log if and only if expr is true.  Because macros cannot accept
variable arguments, this should be invoked like so:
\code
LOGC(x == y, (CLOG_XXX "%d and %d are equal", x, y));
\endcode
In particular, notice the parentheses around everything after the boolean
expression.    Also note that there is no comma after the \c CLOG_XXX.
The \c XXX should be replaced by one of enumerants in \c Log::ELevel
without the leading \c k.  For example, \c CLOG_INFO.  The special
\c CLOG_PRINT level will not be filtered and is never prefixed by the
filename and line number.

If \c NOLOGGING is defined during the build then this macro expands to
nothing.  If \c NDEBUG is not defined during the build then it expands
to a call to Log::print that prints the filename and line number,
otherwise it expands to a call that doesn't.
*/

#if defined(NOLOGGING)
#define LOG(_a1)
#define LOGC(_a1, _a2)
#define CLOG_TRACE
#elif defined(NDEBUG)
#define LOG(_a1) CLOG->print _a1
#define LOGC(_a1, _a2)                                                                                                 \
  if (_a1)                                                                                                             \
  CLOG->print _a2
#define CLOG_TRACE NULL, 0,
#else
#define LOG(_a1) CLOG->print _a1
#define LOGC(_a1, _a2)                                                                                                 \
  if (_a1)                                                                                                             \
  CLOG->print _a2
#define CLOG_TRACE __FILE__, __LINE__,
#endif

// the CLOG_* defines are line and file plus %z and an octal number (060=0,
// 071=9), but the limitation is that once we run out of numbers at either
// end, then we resort to using non-numerical chars. this still works (since
// to deduce the number we subtract octal \060, so '/' is -1, and ':' is 10

#define CLOG_PRINT CLOG_TRACE "%z\057" // char is '/'
#define CLOG_CRIT CLOG_TRACE "%z\060"  // char is '0'
#define CLOG_ERR CLOG_TRACE "%z\061"
#define CLOG_WARN CLOG_TRACE "%z\062"
#define CLOG_NOTE CLOG_TRACE "%z\063"
#define CLOG_INFO CLOG_TRACE "%z\064"
#define CLOG_DEBUG CLOG_TRACE "%z\065"
#define CLOG_DEBUG1 CLOG_TRACE "%z\066"
#define CLOG_DEBUG2 CLOG_TRACE "%z\067"
#define CLOG_DEBUG3 CLOG_TRACE "%z\070"
#define CLOG_DEBUG4 CLOG_TRACE "%z\071" // char is '9'
#define CLOG_DEBUG5 CLOG_TRACE "%z\072" // char is ':'

#define LOG_PRINT(...) LOG((CLOG_PRINT __VA_ARGS__))
#define LOG_CRIT(...) LOG((CLOG_CRIT __VA_ARGS__))
#define LOG_ERR(...) LOG((CLOG_ERR __VA_ARGS__))
#define LOG_WARN(...) LOG((CLOG_WARN __VA_ARGS__))
#define LOG_NOTE(...) LOG((CLOG_NOTE __VA_ARGS__))
#define LOG_INFO(...) LOG((CLOG_INFO __VA_ARGS__))
#define LOG_DEBUG(...) LOG((CLOG_DEBUG __VA_ARGS__))
#define LOG_DEBUG1(...) LOG((CLOG_DEBUG1 __VA_ARGS__))
#define LOG_DEBUG2(...) LOG((CLOG_DEBUG2 __VA_ARGS__))
#define LOG_DEBUG3(...) LOG((CLOG_DEBUG3 __VA_ARGS__))
#define LOG_DEBUG4(...) LOG((CLOG_DEBUG4 __VA_ARGS__))
#define LOG_DEBUG5(...) LOG((CLOG_DEBUG5 __VA_ARGS__))
