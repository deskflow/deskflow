#ifndef COMMON_H
#define COMMON_H

#if defined(__linux__)

#define CONFIG_PLATFORM_LINUX
#define CONFIG_PLATFORM_UNIX
#define CONFIG_TYPES_X11
#define CONFIG_PTHREADS

#elif defined(__sun__)

#define CONFIG_PLATFORM_SOLARIS
#define CONFIG_PLATFORM_UNIX
#define CONFIG_TYPES_X11
#define CONFIG_PTHREADS

#elif defined(_WIN32)

#define CONFIG_PLATFORM_WIN32

#if (_MSC_VER >= 1200)
// work around for statement scoping bug
#define for if (false) { } else for

// turn off bonehead warnings
#pragma warning(disable: 4786) // identifier truncated in debug info
#pragma warning(disable: 4514) // unreferenced inline function removed

// this one's a little too aggressive
#pragma warning(disable: 4127) // conditional expression is constant

// emitted incorrectly under release build in some circumstances
#if defined(NDEBUG)
#pragma warning(disable: 4702) // unreachable code
#pragma warning(disable: 4701) // local variable maybe used uninitialized
#endif

#endif // (_MSC_VER >= 1200)

#else

#error unsupported platform

#endif

#ifndef NULL
#define NULL 0
#endif

#include <cassert>

#endif
