#ifndef CTRACE_H
#define CTRACE_H

class CTrace {
  public:
	static void			print(const char* fmt, ...);
};

#if defined(NDEBUG)

#define TRACE(_X)

#else // NDEBUG

#define TRACE(_X)		CTrace::print ## _X

#endif // NDEBUG

#endif
