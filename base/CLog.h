#ifndef CLOG_H
#define CLOG_H

#include "BasicTypes.h"
#include <stdarg.h>

class CLog {
public:
	typedef void		(*Outputter)(const char*);
	typedef void		(*Lock)(bool lock);

	// 
	static void			print(const char*, ...);
	static void			printt(const char* file, int line, const char*, ...);

	// get/set the function used to write the log.  a NULL outputter
	// means to use the default which is fprintf(stderr, ...).  note
	// that the outputter should not call CLog methods but, if it
	// does, the current lock function must permit recursive locks.
	static void			setOutputter(Outputter);
	static Outputter	getOutputter();

	// get/set the lock/unlock function.  use setLock(NULL) to remove
	// the locking function.  note that the lock function is used when
	// retrieving the lock function.  there is no default lock function.
	static void			setLock(Lock);
	static Lock			getLock();

	// get/set the minimum priority filter.  any message below this
	// priority is discarded.  the default priority is 4 (INFO)
	// (unless built without NDEBUG in which case it's 5 (DEBUG)).
	// the default can be overridden by setting the SYN_LOG_PRI env
	// var to "CRIT", "ERR", etc.
	static void			setFilter(int);
	static int			getFilter();

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
	static char*		vsprint(int pad, char*, int len, const char*, va_list);
	static int			nprint(const char*, va_list);
#if defined(CONFIG_PLATFORM_WIN32)
	static void			openConsole();
#endif

private:
	static Outputter	s_outputter;
	static Lock			s_lock;
	static int			s_maxPriority;
};

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

#define CLOG_CRIT		CLOG_TRACE "%z\060"
#define CLOG_ERR		CLOG_TRACE "%z\061"
#define CLOG_WARN		CLOG_TRACE "%z\062"
#define CLOG_NOTE		CLOG_TRACE "%z\063"
#define CLOG_INFO		CLOG_TRACE "%z\064"
#define CLOG_DEBUG		CLOG_TRACE "%z\065"
#define CLOG_DEBUG1		CLOG_TRACE "%z\066"
#define CLOG_DEBUG2		CLOG_TRACE "%z\067"

#endif
