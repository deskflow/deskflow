#include "CLog.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

//
// CLog
//

static int g_maxPriority = -1;

void					CLog::print(const char* fmt, ...)
{
	// check if fmt begins with a priority argument
	int priority = 4;
	if (fmt[0] == '%' && fmt[1] == 'z') {
		priority = fmt[2] - '\060';
		fmt += 3;
	}

	// print to buffer
	char stack[1024];
	va_list args;
	va_start(args, fmt);
	char* buffer = vsprint(0, stack,
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
	int pad = strlen(file) + 1 + strlen(stack) + 1 + 1;

	// print to buffer
	va_list args;
	va_start(args, fmt);
	char* buffer = vsprint(pad, stack,
								sizeof(stack) / sizeof(stack[0]), fmt, args);
	va_end(args);

	// print the prefix to the buffer
	sprintf(buffer, "%s,%d:", file, line);
	buffer[pad - 1] = ' ';

	// output buffer
	output(priority, buffer);

	// clean up
	if (buffer != stack)
		delete[] buffer;
}

void					CLog::output(int priority, const char* msg)
{
	static const char* s_priority[] = {
								"FATAL",
								"ERROR",
								"WARNING",
								"NOTE",
								"INFO",
								"DEBUG",
							};
	static const int s_numPriority = (int)(sizeof(s_priority) /
											sizeof(s_priority[0]));
	assert(priority >= 0 && priority < s_numPriority);
	assert(msg != 0);

	if (g_maxPriority == -1) {
		g_maxPriority = s_numPriority - 1;
		const char* priEnv = getenv("SYN_LOG_PRI");
		if (priEnv != NULL) {
			for (int i = 0; i < s_numPriority; ++i)
				if (strcmp(priEnv, s_priority[i]) == 0) {
					g_maxPriority = i;
					break;
				}
		}
	}

	if (priority <= g_maxPriority)
		fprintf(stderr, "%s: %s\n", s_priority[priority], msg);
}

char*					CLog::vsprint(int pad, char* buffer, int len,
								const char* fmt, va_list args)
{
	assert(len > 0);

	// try writing to input buffer
	int n;
	if (len >= pad) {
		n = vsnprintf(buffer + pad, len - pad, fmt, args);
		if (n != -1 && n <= len - pad)
			return buffer;
	}

	// start allocating buffers until we write the whole string
	buffer = 0;
	do {
		delete[] buffer;
		len *= 2;
		buffer = new char[len + pad];
		n = vsnprintf(buffer + pad, len - pad, fmt, args);
	} while (n == -1 || n > len - pad);

	return buffer;
}
