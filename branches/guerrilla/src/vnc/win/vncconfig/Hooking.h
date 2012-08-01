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
#ifndef WINVNCCONF_HOOKING
#define WINVNCCONF_HOOKING

#include <rfb_win32/Registry.h>
#include <rfb_win32/Dialog.h>
#include <rfb_win32/SDisplay.h>
#include <rfb_win32/WMPoller.h>
#include <rfb/ServerCore.h>

namespace rfb {

  namespace win32 {

    class HookingPage : public PropSheetPage {
    public:
      HookingPage(const RegKey& rk)
        : PropSheetPage(GetModuleHandle(0), MAKEINTRESOURCE(IDD_HOOKING)), regKey(rk) {}
      void initDialog() {
        setItemChecked(IDC_USEPOLLING, rfb::win32::SDisplay::updateMethod == 0);
        setItemChecked(IDC_USEHOOKS, (rfb::win32::SDisplay::updateMethod == 1) &&
                       rfb::win32::SDisplay::areHooksAvailable());
        enableItem(IDC_USEHOOKS, rfb::win32::SDisplay::areHooksAvailable());
        setItemChecked(IDC_USEDRIVER, (rfb::win32::SDisplay::updateMethod == 2) &&
                       rfb::win32::SDisplay::isDriverAvailable());
        enableItem(IDC_USEDRIVER, rfb::win32::SDisplay::isDriverAvailable());
        setItemChecked(IDC_POLLCONSOLES, rfb::win32::WMPoller::poll_console_windows);
        setItemChecked(IDC_CAPTUREBLT, osVersion.isPlatformNT &&
                       rfb::win32::DeviceFrameBuffer::useCaptureBlt);
        enableItem(IDC_CAPTUREBLT, osVersion.isPlatformNT);
        onCommand(IDC_USEHOOKS, 0);
      }
      bool onCommand(int id, int cmd) {
        switch (id) {
        case IDC_USEPOLLING:
        case IDC_USEHOOKS:
        case IDC_USEDRIVER:
        case IDC_POLLCONSOLES:
        case IDC_CAPTUREBLT:
          setChanged(((rfb::win32::SDisplay::updateMethod == 0) != isItemChecked(IDC_USEPOLLING)) ||
            ((rfb::win32::SDisplay::updateMethod == 1) != isItemChecked(IDC_USEHOOKS)) ||
            ((rfb::win32::SDisplay::updateMethod == 2) != isItemChecked(IDC_USEDRIVER)) ||
            (rfb::win32::WMPoller::poll_console_windows != isItemChecked(IDC_POLLCONSOLES)) ||
            (rfb::win32::DeviceFrameBuffer::useCaptureBlt != isItemChecked(IDC_CAPTUREBLT)));
          enableItem(IDC_POLLCONSOLES, isItemChecked(IDC_USEHOOKS));
          break;
        }
        return false;
      }
      bool onOk() {
        if (isItemChecked(IDC_USEPOLLING))
          regKey.setInt(_T("UpdateMethod"), 0);
        if (isItemChecked(IDC_USEHOOKS))
          regKey.setInt(_T("UpdateMethod"), 1);
        if (isItemChecked(IDC_USEDRIVER))
          regKey.setInt(_T("UpdateMethod"), 2);
        regKey.setBool(_T("PollConsoleWindows"), isItemChecked(IDC_POLLCONSOLES));
        regKey.setBool(_T("UseCaptureBlt"), isItemChecked(IDC_CAPTUREBLT));

        // *** LEGACY compatibility ***
        regKey.setBool(_T("UseHooks"), isItemChecked(IDC_USEHOOKS));
        return true;
      }
    protected:
      RegKey regKey;
    };

  };

};

#endif