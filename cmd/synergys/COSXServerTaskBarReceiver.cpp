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

#include "COSXServerTaskBarReceiver.h"
#include "CArch.h"

//
// COSXServerTaskBarReceiver
//

COSXServerTaskBarReceiver::COSXServerTaskBarReceiver(
				const CBufferedLogOutputter*)
{
	// add ourself to the task bar
	ARCH->addReceiver(this);
}

COSXServerTaskBarReceiver::~COSXServerTaskBarReceiver()
{
	ARCH->removeReceiver(this);
}

void
COSXServerTaskBarReceiver::showStatus()
{
	// do nothing
}

void
COSXServerTaskBarReceiver::runMenu(int, int)
{
	// do nothing
}

void
COSXServerTaskBarReceiver::primaryAction()
{
	// do nothing
}

const IArchTaskBarReceiver::Icon
COSXServerTaskBarReceiver::getIcon() const
{
	return NULL;
}

IArchTaskBarReceiver*
createTaskBarReceiver(const CBufferedLogOutputter* logBuffer)
{
	return new COSXServerTaskBarReceiver(logBuffer);
}
