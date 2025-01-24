/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "arch/unix/ArchTimeUnix.h"

#if TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#else
#if HAVE_SYS_TIME_H
#include <sys/time.h>
#else
#include <time.h>
#endif
#endif

//
// ArchTimeUnix
//

ArchTimeUnix::ArchTimeUnix()
{
  // do nothing
}

ArchTimeUnix::~ArchTimeUnix()
{
  // do nothing
}

double ArchTimeUnix::time()
{
  struct timeval t;
  gettimeofday(&t, NULL);
  return (double)t.tv_sec + 1.0e-6 * (double)t.tv_usec;
}
