/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2002 Chris Schoeneman
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "CMutex.h"
#include "CArch.h"

//
// CMutex
//

CMutex::CMutex()
{
	m_mutex = ARCH->newMutex();
}

CMutex::CMutex(const CMutex&)
{
	m_mutex = ARCH->newMutex();
}

CMutex::~CMutex()
{
	ARCH->closeMutex(m_mutex);
}

CMutex&
CMutex::operator=(const CMutex&)
{
	return *this;
}

void
CMutex::lock() const
{
	ARCH->lockMutex(m_mutex);
}

void
CMutex::unlock() const
{
	ARCH->unlockMutex(m_mutex);
}
