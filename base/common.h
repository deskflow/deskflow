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
// turn off bonehead warnings
#pragma warning(disable: 4786) // identifier truncated in debug info

//
// ignore warnings inside microsoft's standard C++ library
//
// bonehead bugs/warnings
#pragma warning(disable: 4097) // typedef-name used as synonym
#pragma warning(disable: 4511) // copy constructor can't be generated
#pragma warning(disable: 4512) // assignment operator can't be generated

// we'd really rather have these enabled to check our code
#pragma warning(disable: 4100) // unreferenced formal parameter
#endif // (_MSC_VER >= 1200)

#else

#error unsupported platform

#endif

#ifndef NULL
#define NULL 0
#endif

#endif
