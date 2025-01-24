/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "arch/IArchMultithread.h"

//! Mutual exclusion
/*!
A non-recursive mutual exclusion object.  Only one thread at a time can
hold a lock on a mutex.  Any thread that attempts to lock a locked mutex
will block until the mutex is unlocked.  At that time, if any threads are
blocked, exactly one waiting thread will acquire the lock and continue
running.  A thread may not lock a mutex it already owns the lock on;  if
it tries it will deadlock itself.
*/
class Mutex
{
public:
  Mutex();
  //! Equivalent to default c'tor
  /*!
  Copy c'tor doesn't copy anything.  It just makes it possible to
  copy objects that contain a mutex.
  */
  Mutex(const Mutex &);
  ~Mutex();

  //! @name manipulators
  //@{

  //! Does nothing
  /*!
  This does nothing.  It just makes it possible to assign objects
  that contain a mutex.
  */
  Mutex &operator=(const Mutex &);

  //@}
  //! @name accessors
  //@{

  //! Lock the mutex
  /*!
  Locks the mutex, which must not have been previously locked by the
  calling thread.  This blocks if the mutex is already locked by another
  thread.

  (cancellation point)
  */
  void lock() const;

  //! Unlock the mutex
  /*!
  Unlocks the mutex, which must have been previously locked by the
  calling thread.
  */
  void unlock() const;

  //@}

private:
  friend class CondVarBase;
  ArchMutex m_mutex;
};
