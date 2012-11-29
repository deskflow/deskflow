#ifndef CRYPTOPP_STDCPP_H
#define CRYPTOPP_STDCPP_H

#if _MSC_VER >= 1500
#define _DO_NOT_DECLARE_INTERLOCKED_INTRINSICS_IN_MEMORY
#include <intrin.h>
#endif

#include <stddef.h>
#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <memory>
#include <string>
#include <exception>
#include <typeinfo>
#include <algorithm>
#include <map>
#include <vector>

#ifdef CRYPTOPP_INCLUDE_VECTOR_CC
// workaround needed on Sun Studio 12u1 Sun C++ 5.10 SunOS_i386 128229-02 2009/09/21
#include <vector.cc>
#endif

// for alloca
#ifdef __sun
#include <alloca.h>
#elif defined(__MINGW32__) || defined(__BORLANDC__)
#include <malloc.h>
#endif

#ifdef _MSC_VER
#pragma warning(disable: 4231)	// re-disable this
#ifdef _CRTAPI1
#define CRYPTOPP_MSVCRT6
#endif
#endif

#endif
