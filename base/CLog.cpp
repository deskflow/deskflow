#include "CLog.h"
#include "BasicTypes.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#if defined(CONFIG_PLATFORM_WIN32)
#include <windows.h>
#define vsnprintf _vsnprintf
#endif

static int				g_maxPriority = -1;
static const char*		g_priority[] = {
								"FATAL",
								"ERROR",
								"WARNING",
								"NOTE",
								"INFO",
								"DEBUG",
							};
static const int		g_numPriority = (int)(sizeof(g_priority) /
											sizeof(g_priority[0]));
static const int		g_maxPriorityLength = 7; // length of longest string
static const int		g_prioritySuffixLength = 2;
static const int		g_priorityPad = g_maxPriorityLength +
										g_prioritySuffixLength;
static const int		g_newlineLength = 2;

//
// CLog
//

CLog::Outputter			CLog::s_outputter = NULL;

void					CLog::print(const char* fmt, ...)
{
	// check if fmt begins with a priority argument
	int priority = 4;
	if (fmt[0] == '%' && fmt[1] == 'z') {
		priority = fmt[2] - '\060';
		fmt += 3;
	}

	// compute prefix padding length
	int pad = g_priorityPad;

	// print to buffer
	char stack[1024];
	va_list args;
	va_start(args, fmt);
	char* buffer = vsprint(pad, stack,
							sizeof(stack) / sizeof(stack[0]), fmt, args);
	va_end(args);

	// output buffer
	output(priority, buffer);

	// clean up
	if (buffer != stack)
		delete[] buffer;
}

void					CLog::printt(const char* file, int line,
								const char* fmt, ...)
{
	// check if fmt begins with a priority argument
	int priority = 4;
	if (fmt[0] == '%' && fmt[1] == 'z') {
		priority = fmt[2] - '\060';
		fmt += 3;
	}

	// compute prefix padding length
	char stack[1024];
	sprintf(stack, "%d", line);
	int pad = strlen(file) + 1 /* comma */ +
				strlen(stack) + 1 /* colon */ + 1 /* space */ +
				g_priorityPad;

	// print to buffer, leaving space for a newline at the end
	va_list args;
	va_start(args, fmt);
	char* buffer = vsprint(pad, stack,
								sizeof(stack) / sizeof(stack[0]), fmt, args);
	va_end(args);

	// print the prefix to the buffer.  leave space for priority label.
	sprintf(buffer + g_priorityPad, "%s,%d:", file, line);
	buffer[pad - 1] = ' ';

	// output buffer
	output(priority, buffer);

	// clean up
	if (buffer != stack)
		delete[] buffer;
}

void					CLog::setOutputter(Outputter outputter)
{
	s_outputter = outputter;
}

void					CLog::output(int priority, char* msg)
{
	assert(priority >= 0 && priority < g_numPriority);
	assert(msg != 0);

	if (g_maxPriority == -1) {
		g_maxPriority = g_numPriority - 1;
		const char* priEnv = getenv("SYN_LOG_PRI");
		if (priEnv != NULL) {
			for (int i = 0; i < g_numPriority; ++i)
				if (strcmp(priEnv, g_priority[i]) == 0) {
					g_maxPriority = i;
					break;
				}
		}
	}

	if (priority <= g_maxPriority) {
		// insert priority label
		int n = strlen(g_priority[priority]);
		sprintf(msg + g_maxPriorityLength - n, "%s:", g_priority[priority]);
		msg[g_maxPriorityLength + 1] = ' ';

		// put a newline at the end
#if defined(CONFIG_PLATFORM_WIN32)
		strcat(msg + g_priorityPad, "\r\n");
#else
		strcat(msg + g_priorityPad, "\n");
#endif

		// print it
		if (s_outputter)
			s_outputter(msg + g_maxPriorityLength - n);
		else
#if defined(CONFIG_PLATFORM_WIN32)
			OutputDebugString(msg + g_maxPriorityLength - n);
#else
			fprintf(stderr, "%s", msg + g_maxPriorityLength - n);
#endif
	}
}

char*					CLog::vsprint(int pad, char* buffer, int len,
								const char* fmt, va_list args)
{
	assert(len > 0);

	// try writing to input buffer
	int n;
	if (len >= pad) {
		n = vsnprintf(buffer + pad, len - pad, fmt, args);
		if (n != -1 && n <= len - pad + g_newlineLength)
			return buffer;
	}

	// start allocating buffers until we write the whole string
	buffer = 0;
	do {
		delete[] buffer;
		len *= 2;
		buffer = new char[len + pad];
		n = vsnprintf(buffer + pad, len - pad, fmt, args);
	} while (n == -1 || n > len - pad + g_newlineLength);

	return buffer;
}
