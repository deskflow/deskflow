#ifndef CLOG_H
#define CLOG_H

#include <stdarg.h>

class CLog {
public:
	typedef void		(*Outputter)(const char*);

	static void			print(const char*, ...);
	static void			printt(const char* file, int line, const char*, ...);
	static void			setOutputter(Outputter);

private:
	static void			output(int priority, char* msg);
	static char*		vsprint(int pad, char*, int len, const char*, va_list);
	static int			nprint(const char*, va_list);

private:
	static Outputter	s_outputter;
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
