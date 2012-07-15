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

#ifndef WINVNCCONF_LEGACY
#define WINVNCCONF_LEGACY

#include <windows.h>
#include <lmcons.h>
#include <vncconfig/resource.h>
#include <rfb_win32/Registry.h>
#include <rfb_win32/Dialog.h>
#include <rfb_win32/MsgBox.h>
#include <rfb/ServerCore.h>
#include <rfb/secTypes.h>

namespace rfb {

  namespace win32 {

    class LegacyPage : public PropSheetPage {
    public:
      LegacyPage(const RegKey& rk, bool userSettings_)
        : PropSheetPage(GetModuleHandle(0), MAKEINTRESOURCE(IDD_LEGACY)), regKey(rk), userSettings(userSettings_) {}
      void initDialog() {
        setItemChecked(IDC_PROTOCOL_3_3, rfb::Server::protocol3_3);
      }
      bool onCommand(int id, int cmd) {
        switch (id) {
        case IDC_LEGACY_IMPORT:
          {
            DWORD result = MsgBox(0,
              _T("Importing your legacy VNC 3.3 settings will overwrite your existing settings.\n")
              _T("Are you sure you wish to continue?"),
              MB_ICONWARNING | MB_YESNO);
            if (result == IDYES) {
              LoadPrefs();
              MsgBox(0, _T("Imported VNC 3.3 settings successfully."),
                     MB_ICONINFORMATION | MB_OK);

              // Sleep to allow RegConfig thread to reload settings
              Sleep(1000);
              propSheet->reInitPages();
            }
          }
          return true;
        case IDC_PROTOCOL_3_3:
          setChanged(isItemChecked(IDC_PROTOCOL_3_3) != rfb::Server::protocol3_3);
          return false;
        };
        return false;
      }
      bool onOk() {
        regKey.setBool(_T("Protocol3.3"), isItemChecked(IDC_PROTOCOL_3_3));
        return true;
      }

      void LoadPrefs();
      void LoadUserPrefs(const RegKey& key);

    protected:
      bool allowProperties;
      RegKey regKey;
      bool userSettings;
    };

  };

};

#endif