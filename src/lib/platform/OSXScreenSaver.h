/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2002 Chris Schoeneman
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

#include "synergy/IScreenSaver.h"

#include <Carbon/Carbon.h>

class IEventQueue;

//! OSX screen saver implementation
class OSXScreenSaver : public IScreenSaver {
public:
    OSXScreenSaver (IEventQueue* events, void* eventTarget);
    virtual ~OSXScreenSaver ();

    // IScreenSaver overrides
    virtual void enable ();
    virtual void disable ();
    virtual void activate ();
    virtual void deactivate ();
    virtual bool isActive () const;

private:
    void processLaunched (ProcessSerialNumber psn);
    void processTerminated (ProcessSerialNumber psn);

    static pascal OSStatus launchTerminationCallback (
        EventHandlerCallRef nextHandler, EventRef theEvent, void* userData);

private:
    // the target for the events we generate
    void* m_eventTarget;

    bool m_enabled;
    void* m_screenSaverController;
    void* m_autoReleasePool;
    EventHandlerRef m_launchTerminationEventHandlerRef;
    ProcessSerialNumber m_screenSaverPSN;
    IEventQueue* m_events;
};
