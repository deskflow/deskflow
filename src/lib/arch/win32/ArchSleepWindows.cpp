/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "arch/win32/ArchSleepWindows.h"
#include "arch/Arch.h"
#include "arch/win32/ArchMultithreadWindows.h"

//
// ArchSleepWindows
//

ArchSleepWindows::ArchSleepWindows()
{
  // do nothing
}

ArchSleepWindows::~ArchSleepWindows()
{
  // do nothing
}

void ArchSleepWindows::sleep(double timeout)
{
  ARCH->testCancelThread();
  if (timeout < 0.0) {
    return;
  }

  // get the cancel event from the current thread.  this only
  // works if we're using the windows multithread object but
  // this is windows so that's pretty certain;  we'll get a
  // link error if we're not, though.
  ArchMultithreadWindows *mt = ArchMultithreadWindows::getInstance();
  if (mt != NULL) {
    HANDLE cancelEvent = mt->getCancelEventForCurrentThread();
    WaitForSingleObject(cancelEvent, (DWORD)(1000.0 * timeout));
    if (timeout == 0.0) {
      Sleep(0);
    }
  } else {
    Sleep((DWORD)(1000.0 * timeout));
  }
  ARCH->testCancelThread();
}
