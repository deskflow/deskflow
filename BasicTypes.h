#ifndef BASICTYPES_H
#define BASICTYPES_H

#if defined(__linux__)

#define CONFIG_PLATFORM_LINUX
#define CONFIG_TYPES_X11

#include <stdint.h>

typedef int8_t			SInt8;
typedef int16_t			SInt16;
typedef int32_t			SInt32;
typedef int64_t			SInt64;

typedef uint8_t			UInt8;
typedef uint16_t		UInt16;
typedef uint32_t		UInt32;
typedef uint64_t		UInt64;

#else

#error unsupported platform

#endif

#ifndef NULL
#define NULL 0
#endif

#endif


