/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "arch/IArchString.h"

#if HAVE_VSNPRINTF

#if !defined(ARCH_VSNPRINTF)
#define ARCH_VSNPRINTF vsnprintf
#endif

int IArchString::vsnprintf(char *str, int size, const char *fmt, va_list ap)
{
  int n = ::ARCH_VSNPRINTF(str, size, fmt, ap);
  if (n > size) {
    n = -1;
  }
  return n;
}

#elif SYSAPI_UNIX // !HAVE_VSNPRINTF

#include <stdio.h>

int IArchString::vsnprintf(char *str, int size, const char *fmt, va_list ap)
{
  static FILE *bitbucket = fopen("/dev/null", "w");
  if (bitbucket == NULL) {
    // uh oh
    if (size > 0) {
      str[0] = '\0';
    }
    return 0;
  } else {
    // count the characters using the bitbucket
    int n = vfprintf(bitbucket, fmt, ap);
    if (n + 1 <= size) {
      // it'll fit so print it into str
      vsprintf(str, fmt, ap);
    }
    return n;
  }
}

#else // !HAVE_VSNPRINTF && !SYSAPI_UNIX

#error vsnprintf not implemented

#endif // !HAVE_VSNPRINTF
