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

#include "COSXEventQueueBuffer.h"

//
// COSXEventQueueBuffer
//

COSXEventQueueBuffer::COSXEventQueueBuffer()
{
	// FIXME
}

COSXEventQueueBuffer::~COSXEventQueueBuffer()
{
	// FIXME
}

void
COSXEventQueueBuffer::waitForEvent(double timeout)
{
	EventRef event;
	ReceiveNextEvent(0, NULL, timeout, false, &event);
}

IEventQueueBuffer::Type
COSXEventQueueBuffer::getEvent(CEvent& event, UInt32& dataID)
{
	// FIXME
	(void)event;
	(void)dataID;
	return kNone;
}

bool
COSXEventQueueBuffer::addEvent(UInt32 dataID)
{
	// FIXME
	(void)dataID;
	return false;
}

bool
COSXEventQueueBuffer::isEmpty() const
{
	EventRef event;
	OSStatus status = ReceiveNextEvent(0, NULL, 0.0, false, &event);
	return (status != eventLoopTimedOutErr);
}

CEventQueueTimer*
COSXEventQueueBuffer::newTimer(double duration, bool oneShot) const
{
	// FIXME
	(void)duration;
	(void)oneShot;
	return NULL;
}

void
COSXEventQueueBuffer::deleteTimer(CEventQueueTimer*) const
{
	// FIXME
}
