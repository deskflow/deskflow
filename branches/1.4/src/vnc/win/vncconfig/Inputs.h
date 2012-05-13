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
#ifndef WINVNCCONF_INPUTS
#define WINVNCCONF_INPUTS

#ifndef SPI_GETBLOCKSENDINPUTRESETS
#define SPI_GETBLOCKSENDINPUTRESETS 0x1026
#define SPI_SETBLOCKSENDINPUTRESETS 0x1027
#endif

#include <rfb_win32/Registry.h>
#include <rfb_win32/Dialog.h>
#include <rfb_win32/OSVersion.h>
#include <rfb/ServerCore.h>

namespace rfb {
  namespace win32 {

    class InputsPage : public PropSheetPage {
    public:
      InputsPage(const RegKey& rk)
        : PropSheetPage(GetModuleHandle(0), MAKEINTRESOURCE(IDD_INPUTS)),
          regKey(rk), enableAffectSSaver(true) {}
      void initDialog() {
        setItemChecked(IDC_ACCEPT_KEYS, rfb::Server::acceptKeyEvents);
        setItemChecked(IDC_ACCEPT_PTR, rfb::Server::acceptPointerEvents);
        setItemChecked(IDC_ACCEPT_CUTTEXT, rfb::Server::acceptCutText);
        setItemChecked(IDC_SEND_CUTTEXT, rfb::Server::sendCutText);
        setItemChecked(IDC_DISABLE_LOCAL_INPUTS, SDisplay::disableLocalInputs);
        enableItem(IDC_DISABLE_LOCAL_INPUTS, !osVersion.isPlatformWindows);
        BOOL blocked = FALSE;
        if (SystemParametersInfo(SPI_GETBLOCKSENDINPUTRESETS, 0, &blocked, 0))
          setItemChecked(IDC_AFFECT_SCREENSAVER, !blocked);
        else
          enableAffectSSaver = false;
        enableItem(IDC_AFFECT_SCREENSAVER, enableAffectSSaver);
      }
      bool onCommand(int id, int cmd) {
        BOOL inputResetsBlocked;
        SystemParametersInfo(SPI_GETBLOCKSENDINPUTRESETS, 0, &inputResetsBlocked, 0);
        setChanged((rfb::Server::acceptKeyEvents != isItemChecked(IDC_ACCEPT_KEYS)) ||
          (rfb::Server::acceptPointerEvents != isItemChecked(IDC_ACCEPT_PTR)) ||
          (rfb::Server::acceptCutText != isItemChecked(IDC_ACCEPT_CUTTEXT)) ||
          (rfb::Server::sendCutText != isItemChecked(IDC_SEND_CUTTEXT)) ||
          (SDisplay::disableLocalInputs != isItemChecked(IDC_DISABLE_LOCAL_INPUTS)) ||
          (enableAffectSSaver && (!inputResetsBlocked != isItemChecked(IDC_AFFECT_SCREENSAVER))));
        return false;
      }
      bool onOk() {
        regKey.setBool(_T("AcceptKeyEvents"), isItemChecked(IDC_ACCEPT_KEYS));
        regKey.setBool(_T("AcceptPointerEvents"), isItemChecked(IDC_ACCEPT_PTR));
        regKey.setBool(_T("AcceptCutText"), isItemChecked(IDC_ACCEPT_CUTTEXT));
        regKey.setBool(_T("SendCutText"), isItemChecked(IDC_SEND_CUTTEXT));
        regKey.setBool(_T("DisableLocalInputs"), isItemChecked(IDC_DISABLE_LOCAL_INPUTS));
        if (enableAffectSSaver) {
          BOOL blocked = !isItemChecked(IDC_AFFECT_SCREENSAVER);
          SystemParametersInfo(SPI_SETBLOCKSENDINPUTRESETS, blocked, 0, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
        }
        return true;
      }
    protected:
      RegKey regKey;
      bool enableAffectSSaver;
    };

  };
};

#endif