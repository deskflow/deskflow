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

// -=- ListenTrayIcon.h

#ifndef __RFB_WIN32_LISTEN_TRAY_ICON_H__
#define __RFB_WIN32_LISTEN_TRAY_ICON_H__

#include <rfb_win32/TrayIcon.h>
#include <rfb_win32/AboutDialog.h>

namespace rfb {
  namespace win32 {

    class ListenTrayIcon : public TrayIcon {
    public:
      ListenTrayIcon() {
        setIcon(IDI_ICON);
        setToolTip(_T("VNC Viewer"));
      }
      virtual LRESULT processMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
        switch(msg) {

        case WM_USER:
          switch (lParam) {
          case WM_LBUTTONDBLCLK:
            SendMessage(getHandle(), WM_COMMAND, ID_NEW_CONNECTION, 0);
            break;
          case WM_RBUTTONUP:
            HMENU menu = LoadMenu(GetModuleHandle(0), MAKEINTRESOURCE(IDR_TRAY));
            HMENU trayMenu = GetSubMenu(menu, 0);

            // First item is New Connection, the default
            SetMenuDefaultItem(trayMenu, ID_NEW_CONNECTION, FALSE);

            // SetForegroundWindow is required, otherwise Windows ignores the
            // TrackPopupMenu because the window isn't the foreground one, on
            // some older Windows versions...
            SetForegroundWindow(getHandle());

            // Display the menu
            POINT pos;
            GetCursorPos(&pos);
            TrackPopupMenu(trayMenu, 0, pos.x, pos.y, 0, getHandle(), 0);
            break;
			    } 
			    return 0;

        case WM_COMMAND:
          switch (LOWORD(wParam)) {
          case ID_NEW_CONNECTION:
            {
              Thread* connThread = new CConnThread();
              break;
            }
          case ID_OPTIONS:
            OptionsDialog::global.showDialog(0);
            break;
          case ID_ABOUT:
            AboutDialog::instance.showDialog();
            break;
          case ID_CLOSE:
            SendMessage(getHandle(), WM_CLOSE, 0, 0);
            break;
          }
          return 0;

        case WM_CLOSE:
          PostQuitMessage(0);
          return 0;
        }

        return TrayIcon::processMessage(msg, wParam, lParam);
      }
    };

  };
};

#endif // __RFB_WIN32_LISTEN_TRAY_ICON_H__