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

// use VC++ extensions if available
#if defined(_MSC_VER)
typedef signed __int8		SInt8;
typedef signed __int16		SInt16;
typedef signed __int32		SInt32;
typedef signed __int64		SInt64;

typedef unsigned __int8		UInt8;
typedef unsigned __int16	UInt16;
typedef unsigned __int32	UInt32;
typedef unsigned __int64	UInt64;
#else
typedef signed char			SInt8;
typedef short				SInt16;
typedef int					SInt32;
typedef long long			SInt64;

typedef unsigned char		UInt8;
typedef unsigned short		UInt16;
typedef unsigned int		UInt32;
typedef unsigned long long	UInt64;
#endif

#endif // CONFIG_PLATFORM_WIN32

#endif


