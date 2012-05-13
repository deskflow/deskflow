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

// -=- WinVNC Version 4.0 Tray Icon implementation

#include <winvnc/STrayIcon.h>
#include <winvnc/VNCServerService.h>
#include <winvnc/resource.h>

#include <rfb/LogWriter.h>
#include <rfb/Configuration.h>
#include <rfb_win32/LaunchProcess.h>
#include <rfb_win32/TrayIcon.h>
#include <rfb_win32/AboutDialog.h>
#include <rfb_win32/MsgBox.h>
#include <rfb_win32/Service.h>
#include <rfb_win32/CurrentUser.h>

using namespace rfb;
using namespace win32;
using namespace winvnc;

static LogWriter vlog("STrayIcon");

BoolParameter STrayIconThread::disableOptions("DisableOptions", "Disable the Options entry in the VNC Server tray menu.", false);
BoolParameter STrayIconThread::disableClose("DisableClose", "Disable the Close entry in the VNC Server tray menu.", false);


//
// -=- AboutDialog global values
//

const WORD rfb::win32::AboutDialog::DialogId = IDD_ABOUT;
const WORD rfb::win32::AboutDialog::Copyright = IDC_COPYRIGHT;
const WORD rfb::win32::AboutDialog::Version = IDC_VERSION;
const WORD rfb::win32::AboutDialog::BuildTime = IDC_BUILDTIME;
const WORD rfb::win32::AboutDialog::Description = IDC_DESCRIPTION;


//
// -=- Internal tray icon class
//

const UINT WM_SET_TOOLTIP = WM_USER + 1;


class winvnc::STrayIcon : public TrayIcon {
public:
  STrayIcon(STrayIconThread& t) : thread(t),
    vncConfig(_T("vncconfig.exe"), isServiceProcess() ? _T("-noconsole -service") : _T("-noconsole")),
    vncConnect(_T("winvnc4.exe"), _T("-noconsole -connect")) {

    // ***
    SetWindowText(getHandle(), _T("winvnc::IPC_Interface"));
    // ***

    SetTimer(getHandle(), 1, 3000, 0);
    PostMessage(getHandle(), WM_TIMER, 1, 0);
    PostMessage(getHandle(), WM_SET_TOOLTIP, 0, 0);
  }

  virtual LRESULT processMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
    switch(msg) {

    case WM_USER:
      {
        bool userKnown = CurrentUserToken().canImpersonate();
        bool allowOptions = !STrayIconThread::disableOptions && userKnown;
        bool allowClose = !STrayIconThread::disableClose && userKnown;

        switch (lParam) {
        case WM_LBUTTONDBLCLK:
          SendMessage(getHandle(), WM_COMMAND, allowOptions ? ID_OPTIONS : ID_ABOUT, 0);
          break;
        case WM_RBUTTONUP:
          HMENU menu = LoadMenu(GetModuleHandle(0), MAKEINTRESOURCE(thread.menu));
          HMENU trayMenu = GetSubMenu(menu, 0);


          // Default item is Options, if available, or About if not
          SetMenuDefaultItem(trayMenu, allowOptions ? ID_OPTIONS : ID_ABOUT, FALSE);
          
          // Enable/disable options as required
          EnableMenuItem(trayMenu, ID_OPTIONS, (!allowOptions ? MF_GRAYED : MF_ENABLED) | MF_BYCOMMAND);
          EnableMenuItem(trayMenu, ID_CONNECT, (!userKnown ? MF_GRAYED : MF_ENABLED) | MF_BYCOMMAND);
          EnableMenuItem(trayMenu, ID_CLOSE, (!allowClose ? MF_GRAYED : MF_ENABLED) | MF_BYCOMMAND);

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
      }
    
      // Handle tray icon menu commands
    case WM_COMMAND:
      switch (LOWORD(wParam)) {
      case ID_OPTIONS:
        {
          CurrentUserToken token;
          if (token.canImpersonate())
            vncConfig.start(isServiceProcess() ? (HANDLE)token : INVALID_HANDLE_VALUE);
          else
            vlog.error("Options: unknown current user");
        }
        break;
      case ID_CONNECT:
        {
          CurrentUserToken token;
          if (token.canImpersonate())
            vncConnect.start(isServiceProcess() ? (HANDLE)token : INVALID_HANDLE_VALUE);
          else
            vlog.error("Options: unknown current user");
        }
        break;
      case ID_DISCONNECT:
        thread.server.disconnectClients("tray menu disconnect");
        break;
      case ID_CLOSE:
        if (MsgBox(0, _T("Are you sure you want to close the server?"),
                   MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2) == IDYES) {
          if (isServiceProcess()) {
            ImpersonateCurrentUser icu;
            try {
              rfb::win32::stopService(VNCServerService::Name);
            } catch (rdr::Exception& e) {
              MsgBox(0, TStr(e.str()), MB_ICONERROR | MB_OK);
            }
          } else {
            thread.server.stop();
          }
        }
        break;
      case ID_ABOUT:
        AboutDialog::instance.showDialog();
        break;
      }
      return 0;

      // Handle commands send by other processes
    case WM_COPYDATA:
      {
        COPYDATASTRUCT* command = (COPYDATASTRUCT*)lParam;
        switch (command->dwData) {
        case 1:
          {
            CharArray viewer = new char[command->cbData + 1];
            memcpy(viewer.buf, command->lpData, command->cbData);
            viewer.buf[command->cbData] = 0;
            return thread.server.addNewClient(viewer.buf) ? 1 : 0;
          }
        case 2:
          return thread.server.disconnectClients("IPC disconnect") ? 1 : 0;
        };
      };
      break;

    case WM_CLOSE:
      PostQuitMessage(0);
      break;

    case WM_TIMER:
      if (rfb::win32::desktopChangeRequired()) {
        SendMessage(getHandle(), WM_CLOSE, 0, 0);
        return 0;
      }
      setIcon(thread.server.isServerInUse() ? thread.activeIcon : thread.inactiveIcon);
      return 0;

    case WM_SET_TOOLTIP:
      {
        rfb::Lock l(thread.lock);
        if (thread.toolTip.buf)
          setToolTip(thread.toolTip.buf);
      }
      return 0;

    }

    return TrayIcon::processMessage(msg, wParam, lParam);
  }

protected:
  LaunchProcess vncConfig;
  LaunchProcess vncConnect;
  STrayIconThread& thread;
};


STrayIconThread::STrayIconThread(VNCServerWin32& sm, UINT inactiveIcon_, UINT activeIcon_, UINT menu_)
: Thread("TrayIcon"), server(sm), inactiveIcon(inactiveIcon_), activeIcon(activeIcon_), menu(menu_),
  windowHandle(0), runTrayIcon(true) {
  start();
}


void STrayIconThread::run() {
  while (runTrayIcon) {
    if (rfb::win32::desktopChangeRequired() && 
      !rfb::win32::changeDesktop())
      Sleep(2000);

    STrayIcon icon(*this);
    windowHandle = icon.getHandle();

    MSG msg;
    while (runTrayIcon && ::GetMessage(&msg, 0, 0, 0) > 0) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }

    windowHandle = 0;
  }
}

void STrayIconThread::setToolTip(const TCHAR* text) {
  if (!windowHandle) return;
  Lock l(lock);
  delete [] toolTip.buf;
  toolTip.buf = tstrDup(text);
  PostMessage(windowHandle, WM_SET_TOOLTIP, 0, 0);
}


