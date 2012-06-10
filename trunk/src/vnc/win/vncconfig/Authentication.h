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
#ifndef WINVNCCONF_AUTHENTICATION
#define WINVNCCONF_AUTHENTICATION

#include <vncconfig/PasswordDialog.h>
#include <rfb_win32/Registry.h>
#include <rfb_win32/Dialog.h>
#include <rfb_win32/OSVersion.h>
#include <rfb_win32/MsgBox.h>
#include <rfb/ServerCore.h>
#include <rfb/secTypes.h>
#include <rfb/Password.h>

static rfb::BoolParameter queryOnlyIfLoggedOn("QueryOnlyIfLoggedOn",
  "Only prompt for a local user to accept incoming connections if there is a user logged on", false);

namespace rfb {

  namespace win32 {

    class AuthenticationPage : public PropSheetPage {
    public:
      AuthenticationPage(const RegKey& rk)
        : PropSheetPage(GetModuleHandle(0), MAKEINTRESOURCE(IDD_AUTHENTICATION)), regKey(rk) {}
      void initDialog() {
        CharArray sec_types_str(SSecurityFactoryStandard::sec_types.getData());
        std::list<int> sec_types = parseSecTypes(sec_types_str.buf);

        useNone = useVNC = false;
        std::list<int>::iterator i;
        for (i=sec_types.begin(); i!=sec_types.end(); i++) {
          if ((*i) == secTypeNone) useNone = true;
          else if ((*i) == secTypeVncAuth) useVNC = true;
        }

        HWND security = GetDlgItem(handle, IDC_ENCRYPTION);
        SendMessage(security, CB_ADDSTRING, 0, (LPARAM)_T("Always Off"));
        SendMessage(security, CB_SETCURSEL, 0, 0);
        enableItem(IDC_AUTH_NT, false); enableItem(IDC_AUTH_NT_CONF, false);
        enableItem(IDC_ENCRYPTION, false); enableItem(IDC_AUTH_RA2_CONF, false);

        setItemChecked(IDC_AUTH_NONE, useNone);
        setItemChecked(IDC_AUTH_VNC, useVNC);
        setItemChecked(IDC_QUERY_CONNECT, rfb::Server::queryConnect);
        setItemChecked(IDC_QUERY_LOGGED_ON, queryOnlyIfLoggedOn);
        onCommand(IDC_AUTH_NONE, 0);
      }
      bool onCommand(int id, int cmd) {
        switch (id) {
        case IDC_AUTH_VNC_PASSWD:
          {
            PasswordDialog passwdDlg(regKey, registryInsecure);
            passwdDlg.showDialog(handle);
          }
          return true;
        case IDC_AUTH_NONE:
        case IDC_AUTH_VNC:
          enableItem(IDC_AUTH_VNC_PASSWD, isItemChecked(IDC_AUTH_VNC));
        case IDC_QUERY_CONNECT:
        case IDC_QUERY_LOGGED_ON:
          setChanged((useNone != isItemChecked(IDC_AUTH_NONE)) ||
                     (useVNC != isItemChecked(IDC_AUTH_VNC)) ||
                     (rfb::Server::queryConnect != isItemChecked(IDC_QUERY_CONNECT)) ||
                     (queryOnlyIfLoggedOn != isItemChecked(IDC_QUERY_LOGGED_ON)));
          enableItem(IDC_QUERY_LOGGED_ON, enableQueryOnlyIfLoggedOn());
          return false;
        };
        return false;
      }
      bool onOk() {
        bool useVncChanged = useVNC != isItemChecked(IDC_AUTH_VNC);
        useVNC = isItemChecked(IDC_AUTH_VNC);
        useNone = isItemChecked(IDC_AUTH_NONE);
        if (useVNC) {
          verifyVncPassword(regKey);
          regKey.setString(_T("SecurityTypes"), _T("VncAuth"));
        } else {
          if (haveVncPassword() && useVncChanged &&
              MsgBox(0, _T("The VNC authentication method is disabled, but a password is still stored for it.\n")
                        _T("Do you want to remove the VNC authentication password from the registry?"),
                        MB_ICONWARNING | MB_YESNO) == IDYES) {
            regKey.setBinary(_T("Password"), 0, 0);
          }
          regKey.setString(_T("SecurityTypes"), _T("None"));
        }
        regKey.setString(_T("ReverseSecurityTypes"), _T("None"));
        regKey.setBool(_T("QueryConnect"), isItemChecked(IDC_QUERY_CONNECT));
        regKey.setBool(_T("QueryOnlyIfLoggedOn"), isItemChecked(IDC_QUERY_LOGGED_ON));
        return true;
      }
      void setWarnPasswdInsecure(bool warn) {
        registryInsecure = warn;
      }
      bool enableQueryOnlyIfLoggedOn() {
        return isItemChecked(IDC_QUERY_CONNECT) && osVersion.isPlatformNT && (osVersion.dwMajorVersion >= 5);
      }


      static bool haveVncPassword() {
        PlainPasswd password(SSecurityFactoryStandard::vncAuthPasswd.getVncAuthPasswd());
        return password.buf && strlen(password.buf) != 0;
      }

      static void verifyVncPassword(const RegKey& regKey) {
        if (!haveVncPassword()) {
          MsgBox(0, _T("The VNC authentication method is enabled, but no password is specified.\n")
                    _T("The password dialog will now be shown."), MB_ICONINFORMATION | MB_OK);
          PasswordDialog passwd(regKey, registryInsecure);
          passwd.showDialog();
        }
      }

    protected:
      RegKey regKey;
      static bool registryInsecure;
      bool useNone;
      bool useVNC;
    };

  };

  bool AuthenticationPage::registryInsecure = false;

};

#endif
