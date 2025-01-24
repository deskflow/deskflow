/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "mt/CondVar.h"
#include "arch/Arch.h"
#include "base/Stopwatch.h"

//
// CondVarBase
//

CondVarBase::CondVarBase(Mutex *mutex) : m_mutex(mutex)
{
  assert(m_mutex != NULL);
  m_cond = ARCH->newCondVar();
}

CondVarBase::~CondVarBase()
{
  ARCH->closeCondVar(m_cond);
}

void CondVarBase::lock() const
{
  m_mutex->lock();
}

void CondVarBase::unlock() const
{
  m_mutex->unlock();
}

void CondVarBase::signal()
{
  ARCH->signalCondVar(m_cond);
}

void CondVarBase::broadcast()
{
  ARCH->broadcastCondVar(m_cond);
}

bool CondVarBase::wait(Stopwatch &timer, double timeout) const
{
  double remain = timeout - timer.getTime();
  // Some ARCH wait()s return prematurely, retry until really timed out
  // In particular, ArchMultithreadPosix::waitCondVar() returns every 100ms
  do {
    // Always call wait at least once, even if remain is 0, to give
    // other thread a chance to grab the mutex to avoid deadlocks on
    // busy waiting.
    if (remain < 0.0)
      remain = 0.0;
    if (wait(remain))
      return true;
    remain = timeout - timer.getTime();
  } while (remain >= 0.0);
  return false;
}

bool CondVarBase::wait(double timeout) const
{
  return ARCH->waitCondVar(m_cond, m_mutex->m_mutex, timeout);
}

Mutex *CondVarBase::getMutex() const
{
  return m_mutex;
}
