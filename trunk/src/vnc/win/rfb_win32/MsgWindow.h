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

// -=- MsgWindow.h

// Base-class for any hidden message-handling windows used in the rfb::win32
// implementation.

#ifndef __RFB_WIN32_MSG_WINDOW_H__
#define __RFB_WIN32_MSG_WINDOW_H__

#include <windows.h>
#include <rfb_win32/TCharArray.h>

namespace rfb {

  namespace win32 {

    class MsgWindow {
    public:
      MsgWindow(const TCHAR* _name);
      virtual ~MsgWindow();

      const TCHAR* getName() {return name.buf;}
      HWND getHandle() const {return handle;}

      virtual LRESULT processMessage(UINT msg, WPARAM wParam, LPARAM lParam);

    protected:
      TCharArray name;
      HWND handle;
    };

  };

};

#endif // __RFB_WIN32_MSG_WINDOW_H__
