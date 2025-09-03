/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

// Consider whether or not to use either encapsulation (as below)
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

#include "common/Common.h"

#if SYSAPI_WIN32

#include "arch/win32/ArchDaemonWindows.h"
#include "arch/win32/ArchLogWindows.h"
#include "arch/win32/ArchMultithreadWindows.h"
#include "arch/win32/ArchNetworkWinsock.h"

#elif SYSAPI_UNIX

#include "arch/unix/ArchDaemonUnix.h"
#include "arch/unix/ArchLogUnix.h"
#include "arch/unix/ArchMultithreadPosix.h"
#include "arch/unix/ArchNetworkBSD.h"

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
class Arch : public ARCH_DAEMON, public ARCH_LOG, public ARCH_MULTITHREAD, public ARCH_NETWORK
{
public:
  Arch();
  explicit Arch(Arch *arch);
  ~Arch() override = default;

#if SYSAPI_WIN32
  //! Call init on other arch classes.
  /*!
  Some arch classes depend on others to exist first. When init is called
  these classes will have ARCH available for use.
  */
  void init() override;
#endif

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

  /**
   * @brief blocks calling thread for timout seconds
   * @param timeout - blocking time in seconds. if < 0 not blocked if == 0 then caller yields the CPU
   */
  static void sleep(double timeout);

  /**
   * @brief time
   * @return Returns the number of seconds since some arbitrary starting time.
   * This should return as high a precision as reasonable.
   */
  static double time();

private:
  static Arch *s_instance;
};

//! Convenience object to lock/unlock an arch mutex
class ArchMutexLock
{
public:
  explicit ArchMutexLock(ArchMutex mutex) : m_mutex(mutex)
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
