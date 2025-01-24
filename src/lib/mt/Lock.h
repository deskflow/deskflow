/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "common/common.h"

class Mutex;
class CondVarBase;

//! Mutual exclusion lock utility
/*!
This class locks a mutex or condition variable in the c'tor and unlocks
it in the d'tor.  It's easier and safer than manually locking and
unlocking since unlocking must usually be done no matter how a function
exits (including by unwinding due to an exception).
*/
class Lock
{
public:
  //! Lock the mutex \c mutex
  Lock(const Mutex *mutex);
  //! Lock the condition variable \c cv
  Lock(const CondVarBase *cv);
  //! Unlock the mutex or condition variable
  ~Lock();

private:
  // not implemented
  Lock(const Lock &);
  Lock &operator=(const Lock &);

private:
  const Mutex *m_mutex;
};
