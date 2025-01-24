/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "arch/unix/ArchSleepUnix.h"

#include "arch/Arch.h"

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
#if !HAVE_NANOSLEEP
#if HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#endif

//
// ArchSleepUnix
//

ArchSleepUnix::ArchSleepUnix()
{
  // do nothing
}

ArchSleepUnix::~ArchSleepUnix()
{
  // do nothing
}

void ArchSleepUnix::sleep(double timeout)
{
  ARCH->testCancelThread();
  if (timeout < 0.0) {
    return;
  }

#if HAVE_NANOSLEEP
  // prep timeout
  struct timespec t;
  t.tv_sec = (long)timeout;
  t.tv_nsec = (long)(1.0e+9 * (timeout - (double)t.tv_sec));

  // wait
  while (nanosleep(&t, &t) < 0)
    ARCH->testCancelThread();
#else
  /* emulate nanosleep() with select() */
  double startTime = ARCH->time();
  double timeLeft = timeout;
  while (timeLeft > 0.0) {
    struct timeval timeout2;
    timeout2.tv_sec = static_cast<int>(timeLeft);
    timeout2.tv_usec = static_cast<int>(1.0e+6 * (timeLeft - timeout2.tv_sec));
    select(
        (SELECT_TYPE_ARG1)0, SELECT_TYPE_ARG234 NULL, SELECT_TYPE_ARG234 NULL, SELECT_TYPE_ARG234 NULL,
        SELECT_TYPE_ARG5 & timeout2
    );
    ARCH->testCancelThread();
    timeLeft = timeout - (ARCH->time() - startTime);
  }
#endif
}
