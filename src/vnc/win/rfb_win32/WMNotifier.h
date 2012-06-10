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

// -=- WMNotifier.h
//
// The WMNotifier is used to get callbacks indicating changes in the state
// of the system, for instance in the size/format/palette of the display.
// The WMNotifier contains a Win32 window, which receives notifications of
// system events and stores them.  Whenever processEvent is called, any
// incoming events are processed and the appropriate notifier called.

#ifndef __RFB_WIN32_NOTIFIER_H__
#define __RFB_WIN32_NOTIFIER_H__

#include <rfb/SDesktop.h>
#include <rfb/Threading.h>
#include <rfb_win32/MsgWindow.h>
#include <rfb_win32/DeviceFrameBuffer.h>
#include <rfb_win32/SInput.h>

namespace rfb {

  namespace win32 {

    // -=- Window Message Monitor implementation

    class WMMonitor : MsgWindow {
    public:

      class Notifier {
      public:
        typedef enum {DisplaySizeChanged, DisplayColourMapChanged,
          DisplayPixelFormatChanged} DisplayEventType;
        virtual void notifyDisplayEvent(DisplayEventType evt) = 0;
      };

      WMMonitor();
      virtual ~WMMonitor();

      void setNotifier(Notifier* wmn) {notifier=wmn;}

    protected:
      // - Internal MsgWindow callback
      virtual LRESULT processMessage(UINT msg, WPARAM wParam, LPARAM lParam);

      Notifier* notifier;
    };

  };

};

#endif // __RFB_WIN32_WMNOTIFIER_H__
