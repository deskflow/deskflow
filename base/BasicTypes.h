#ifndef BASICTYPES_H
#define BASICTYPES_H

#include "common.h"

#if defined(CONFIG_PLATFORM_LINUX)

#include <stdint.h>

typedef int8_t			SInt8;
typedef int16_t			SInt16;
typedef int32_t			SInt32;
typedef int64_t			SInt64;

typedef uint8_t			UInt8;
typedef uint16_t		UInt16;
typedef uint32_t		UInt32;
typedef uint64_t		UInt64;

#endif // CONFIG_PLATFORM_LINUX

#if defined(CONFIG_PLATFORM_SOLARIS)

#include <inttypes.h>

typedef int8_t			SInt8;
typedef int16_t			SInt16;
typedef int32_t			SInt32;
typedef int64_t			SInt64;

typedef uint8_t			UInt8;
typedef uint16_t		UInt16;
typedef uint32_t		UInt32;
typedef uint64_t		UInt64;

#endif // CONFIG_PLATFORM_SOLARIS

#if defined(CONFIG_PLATFORM_WIN32)

// FIXME

#endif // CONFIG_PLATFORM_WIN32

#endif


