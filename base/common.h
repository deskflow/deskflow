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

#elif defined(_WINDOWS) && defined(WIN32)

#define CONFIG_PLATFORM_WIN32

#else

#error unsupported platform

#endif

#ifndef NULL
#define NULL 0
#endif

#endif
