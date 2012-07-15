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

#ifndef __RFB_WIN32_MSGBOX_H__
#define __RFB_WIN32_MSGBOX_H__

#include <windows.h>
#include <rfb_win32/TCharArray.h>

namespace rfb {
  namespace win32 {

    // Define rfb::win32::AppName somewhere in the application.
    // The MsgBox function will use the specified application name
    // as the prefix for the message box title.
    // Message box titles are based on the (standard Win32) flags
    // passed to the MsgBox helper function.

    extern TStr AppName;

    // Wrapper around Win32 MessageBox()
    static int MsgBox(HWND parent, const TCHAR* msg, UINT flags) {
      const TCHAR* msgType = 0;
      UINT tflags = flags & 0x70;
      if (tflags == MB_ICONHAND)
        msgType = _T("Error");
      else if (tflags == MB_ICONQUESTION)
        msgType = _T("Question");
      else if (tflags == MB_ICONEXCLAMATION)
        msgType = _T("Warning");
      else if (tflags == MB_ICONASTERISK)
        msgType = _T("Information");
      flags |= MB_TOPMOST | MB_SETFOREGROUND;
      int len = _tcslen(AppName.buf) + 1;
      if (msgType) len += _tcslen(msgType) + 3;
      TCharArray title = new TCHAR[len];
      _tcscpy(title.buf, AppName.buf);
      if (msgType) {
        _tcscat(title.buf, _T(" : "));
        _tcscat(title.buf, msgType);
      }
      return MessageBox(parent, msg, title.buf, flags);
    }

  };
};

#endif
