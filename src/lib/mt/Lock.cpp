/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "mt/Lock.h"
#include "mt/CondVar.h"
#include "mt/Mutex.h"

//
// Lock
//

Lock::Lock(const Mutex *mutex) : m_mutex(mutex)
{
  m_mutex->lock();
}

Lock::Lock(const CondVarBase *cv) : m_mutex(cv->getMutex())
{
  m_mutex->lock();
}

Lock::~Lock()
{
  m_mutex->unlock();
}
