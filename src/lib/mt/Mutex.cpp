/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "mt/Mutex.h"

#include "arch/Arch.h"

//
// Mutex
//

Mutex::Mutex()
{
  m_mutex = ARCH->newMutex();
}

Mutex::Mutex(const Mutex &)
{
  m_mutex = ARCH->newMutex();
}

Mutex::~Mutex()
{
  ARCH->closeMutex(m_mutex);
}

Mutex &Mutex::operator=(const Mutex &)
{
  return *this;
}

void Mutex::lock() const
{
  ARCH->lockMutex(m_mutex);
}

void Mutex::unlock() const
{
  ARCH->unlockMutex(m_mutex);
}
