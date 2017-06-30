/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2004 Chris Schoeneman
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "base/IEventQueueBuffer.h"
#include "arch/IArchMultithread.h"
#include "common/stddeque.h"

//! In-memory event queue buffer
/*!
An event queue buffer provides a queue of events for an IEventQueue.
*/
class SimpleEventQueueBuffer : public IEventQueueBuffer {
public:
    SimpleEventQueueBuffer ();
    ~SimpleEventQueueBuffer ();

    // IEventQueueBuffer overrides
    void
    init () {
    }
    virtual void waitForEvent (double timeout);
    virtual Type getEvent (Event& event, UInt32& dataID);
    virtual bool addEvent (UInt32 dataID);
    virtual bool isEmpty () const;
    virtual EventQueueTimer* newTimer (double duration, bool oneShot) const;
    virtual void deleteTimer (EventQueueTimer*) const;

private:
    typedef std::deque<UInt32> EventDeque;

    ArchMutex m_queueMutex;
    ArchCond m_queueReadyCond;
    bool m_queueReady;
    EventDeque m_queue;
};
