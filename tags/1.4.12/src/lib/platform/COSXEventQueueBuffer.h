/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Bolton Software Ltd.
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef COSXEVENTQUEUEBUFFER_H
#define COSXEVENTQUEUEBUFFER_H

#include <Carbon/Carbon.h>
#include "IEventQueueBuffer.h"

//! Event queue buffer for OS X
class COSXEventQueueBuffer : public IEventQueueBuffer {
public:
	COSXEventQueueBuffer();
	virtual ~COSXEventQueueBuffer();

	// IEventQueueBuffer overrides
	virtual void		waitForEvent(double timeout);
	virtual Type		getEvent(CEvent& event, UInt32& dataID);
	virtual bool		addEvent(UInt32 dataID);
	virtual bool		isEmpty() const;
	virtual CEventQueueTimer*
						newTimer(double duration, bool oneShot) const;
	virtual void		deleteTimer(CEventQueueTimer*) const;

private:
	EventRef			m_event;
};

#endif
