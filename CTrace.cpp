#include "CTrace.h"
#include <stdarg.h>
#include <stdio.h>

//
// CTrace
//

void					CTrace::print(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	fprintf(stderr, "\n");
}
