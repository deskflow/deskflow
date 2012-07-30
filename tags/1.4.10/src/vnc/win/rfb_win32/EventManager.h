/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 * USA.
 */

// -=- EventManager.h

// Win32 event manager.  Caller supplies event & handler pairs and
// then uses getMessage() in place of ::GetMessage() in the main
// loop.  EventManager calls the event handler whenever the event
// is set.
// Ownership of events remains with the caller.
// It is the responsibility of handlers to reset events.

#ifndef __RFB_WIN32_EVENT_MGR_H__
#define __RFB_WIN32_EVENT_MGR_H__

#include <rfb_win32/Win32Util.h>

namespace rfb {
  namespace win32 {

    class EventHandler {
    public:
      virtual ~EventHandler() {}
      virtual void processEvent(HANDLE event) = 0;
    };

    class EventManager {
    public:
      EventManager();
      virtual ~EventManager();

      // Add a Win32 event & handler for it
      //   NB: The handler must call ResetEvent on the event.
      //   NB: The caller retains ownership of the event.
      virtual bool addEvent(HANDLE event, EventHandler* ecb);

      // Remove a Win32 event
      virtual void removeEvent(HANDLE event);

      // getMessage
      //   Waits for a message to become available on the thread's message queue,
      //   and returns it.  If any registered events become set while waiting then
      //   their handlers are called before returning.
      //   Returns zero if the message is WM_QUIT, -1 in case of error, >0 otherwise.
      virtual BOOL getMessage(MSG* msg, HWND hwnd, UINT minMsg, UINT maxMsg);

    protected:
      // checkTimeouts
      //   Derived classes should override this to perform any extra processing,
      //   returning the maximum number of milliseconds after which the callback
      //   should be called again.
      virtual int checkTimeouts();

      HANDLE events[MAXIMUM_WAIT_OBJECTS];
      EventHandler* handlers[MAXIMUM_WAIT_OBJECTS-1];
      int eventCount;
    };

  };
};

#endif
