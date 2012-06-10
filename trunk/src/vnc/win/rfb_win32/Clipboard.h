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

// -=- Clipboard.h
//
// The Clipboard is used to set the system clipboard, and to get callbacks
// when the system clipboard has changed.

#ifndef __RFB_WIN32_CLIPBOARD_H__
#define __RFB_WIN32_CLIPBOARD_H__

#include <rfb/SDesktop.h>
#include <rfb/Threading.h>
#include <rfb_win32/MsgWindow.h>
#include <rfb_win32/DeviceFrameBuffer.h>

namespace rfb {

  namespace win32 {

    class Clipboard : MsgWindow {
    public:

      // -=- Abstract base class for callback recipients
      class Notifier {
      public:
        virtual void notifyClipboardChanged(const char* text, int len) = 0;
      };

      Clipboard();
      ~Clipboard();

      // - Set the notifier to use
      void setNotifier(Notifier* cbn) {notifier = cbn;}

      // - Set the clipboard contents
      void setClipText(const char* text);

    protected:
      // - Internal MsgWindow callback
      virtual LRESULT processMessage(UINT msg, WPARAM wParam, LPARAM lParam);

      Notifier* notifier;
      HWND next_window;
    };

  };

};

#endif // __RFB_WIN32_CLIPBOARD_H__
