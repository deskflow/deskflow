/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2003 Chris Schoeneman
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

#include "CXWindowsClientTaskBarReceiver.h"
#include "CArch.h"

//
// CXWindowsClientTaskBarReceiver
//

CXWindowsClientTaskBarReceiver::CXWindowsClientTaskBarReceiver(
				const CBufferedLogOutputter*)
{
	// add ourself to the task bar
	ARCH->addReceiver(this);
}

CXWindowsClientTaskBarReceiver::~CXWindowsClientTaskBarReceiver()
{
	ARCH->removeReceiver(this);
}

void
CXWindowsClientTaskBarReceiver::showStatus()
{
	// do nothing
}

void
CXWindowsClientTaskBarReceiver::runMenu(int, int)
{
	// do nothing
}

void
CXWindowsClientTaskBarReceiver::primaryAction()
{
	// do nothing
}

const IArchTaskBarReceiver::Icon
CXWindowsClientTaskBarReceiver::getIcon() const
{
	return NULL;
}
