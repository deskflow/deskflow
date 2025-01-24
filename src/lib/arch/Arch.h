/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

// TODO: consider whether or not to use either encapsulation (as below)
// or inheritance (as it is now) for the ARCH stuff.
//
// case for encapsulation:
// pros:
//  - compiler errors for missing pv implementations are not absolutely bonkers.
//  - function names don't have to be so verbose.
//  - easier to understand and debug.
//  - ctors in IArch implementations can call other implementations.
// cons:
//  - slightly more code for calls to ARCH.
//  - you'll have to modify each ARCH call.
//
// also, we may want to consider making each encapsulated
// class lazy-loaded so that apps like the daemon don't load
// stuff when they don't need it.

#pragma once

#include "common/common.h"

#if SYSAPI_WIN32

#include "arch/win32/ArchConsoleWindows.h"
#include "arch/win32/ArchDaemonWindows.h"
#include "arch/win32/ArchFileWindows.h"
#include "arch/win32/ArchLogWindows.h"
#include "arch/win32/ArchMultithreadWindows.h"
#include "arch/win32/ArchNetworkWinsock.h"
#include "arch/win32/ArchSleepWindows.h"
#include "arch/win32/ArchStringWindows.h"
#include "arch/win32/ArchSystemWindows.h"
#include "arch/win32/ArchTimeWindows.h"

#elif SYSAPI_UNIX

#include "arch/unix/ArchConsoleUnix.h"
#include "arch/unix/ArchDaemonUnix.h"
#include "arch/unix/ArchFileUnix.h"
#include "arch/unix/ArchLogUnix.h"
#include "arch/unix/ArchNetworkBSD.h"
#include "arch/unix/ArchSleepUnix.h"
#include "arch/unix/ArchStringUnix.h"
#include "arch/unix/ArchSystemUnix.h"
#include "arch/unix/ArchTimeUnix.h"

#if HAVE_PTHREAD
#include "arch/unix/ArchMultithreadPosix.h"
#endif

#endif

/*!
\def ARCH
This macro evaluates to the singleton Arch object.
*/
#define ARCH (Arch::getInstance())

//! Delegating implementation of architecture dependent interfaces
/*!
This class is a centralized interface to all architecture dependent
interface implementations (except miscellaneous functions).  It
instantiates an implementation of each interface and delegates calls
to each method to those implementations.  Clients should use the
\c ARCH macro to access this object.  Clients must also instantiate
exactly one of these objects before attempting to call any method,
typically at the beginning of \c main().
*/
class Arch : public ARCH_CONSOLE,
             public ARCH_DAEMON,
             public ARCH_FILE,
             public ARCH_LOG,
             public ARCH_MULTITHREAD,
             public ARCH_NETWORK,
             public ARCH_SLEEP,
             public ARCH_STRING,
             public ARCH_SYSTEM,
             public ARCH_TIME
{
public:
  Arch();
  Arch(Arch *arch);
  virtual ~Arch();

  //! Call init on other arch classes.
  /*!
  Some arch classes depend on others to exist first. When init is called
  these classes will have ARCH available for use.
  */
  virtual void init();

  //
  // accessors
  //

  //! Return the singleton instance
  /*!
  The client must have instantiated exactly once Arch object before
  calling this function.
  */
  static Arch *getInstance();

  static void setInstance(Arch *s)
  {
    s_instance = s;
  }

private:
  static Arch *s_instance;
};

//! Convenience object to lock/unlock an arch mutex
class ArchMutexLock
{
public:
  ArchMutexLock(ArchMutex mutex) : m_mutex(mutex)
  {
    ARCH->lockMutex(m_mutex);
  }
  ArchMutexLock(ArchMutexLock const &) = delete;
  ArchMutexLock(ArchMutexLock &&) = delete;
  ~ArchMutexLock()
  {
    ARCH->unlockMutex(m_mutex);
  }

  ArchMutexLock &operator=(ArchMutexLock const &) = delete;
  ArchMutexLock &operator=(ArchMutexLock &&) = delete;

private:
  ArchMutex m_mutex;
};
