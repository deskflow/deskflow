/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2004 Chris Schoeneman
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

#include "COSXClientTaskBarReceiver.h"
#include "CArch.h"

//
// COSXClientTaskBarReceiver
//

COSXClientTaskBarReceiver::COSXClientTaskBarReceiver(
				const CBufferedLogOutputter*)
{
	// add ourself to the task bar
	ARCH->addReceiver(this);
}

COSXClientTaskBarReceiver::~COSXClientTaskBarReceiver()
{
	ARCH->removeReceiver(this);
}

void
COSXClientTaskBarReceiver::showStatus()
{
	// do nothing
}

void
COSXClientTaskBarReceiver::runMenu(int, int)
{
	// do nothing
}

void
COSXClientTaskBarReceiver::primaryAction()
{
	// do nothing
}

const IArchTaskBarReceiver::Icon
COSXClientTaskBarReceiver::getIcon() const
{
	return NULL;
}
