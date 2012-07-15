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
#ifndef WINVNCCONF_SHARING
#define WINVNCCONF_SHARING

#include <rfb_win32/Registry.h>
#include <rfb_win32/Dialog.h>
#include <rfb/ServerCore.h>

namespace rfb {

  namespace win32 {

    class SharingPage : public PropSheetPage {
    public:
      SharingPage(const RegKey& rk)
        : PropSheetPage(GetModuleHandle(0), MAKEINTRESOURCE(IDD_SHARING)), regKey(rk) {}
      void initDialog() {
        setItemChecked(IDC_DISCONNECT_CLIENTS, rfb::Server::disconnectClients);
        setItemChecked(IDC_SHARE_NEVER, rfb::Server::neverShared);
        setItemChecked(IDC_SHARE_ALWAYS, rfb::Server::alwaysShared);
        setItemChecked(IDC_SHARE_CLIENT, !(rfb::Server::neverShared || rfb::Server::alwaysShared));
      }
      bool onCommand(int id, int cmd) {
        setChanged((isItemChecked(IDC_DISCONNECT_CLIENTS) != rfb::Server::disconnectClients) ||
          (isItemChecked(IDC_SHARE_NEVER) != rfb::Server::neverShared) ||
          (isItemChecked(IDC_SHARE_ALWAYS) != rfb::Server::alwaysShared));
        return true;
      }
      bool onOk() {
        regKey.setBool(_T("DisconnectClients"), isItemChecked(IDC_DISCONNECT_CLIENTS));
        regKey.setBool(_T("AlwaysShared"), isItemChecked(IDC_SHARE_ALWAYS));
        regKey.setBool(_T("NeverShared"), isItemChecked(IDC_SHARE_NEVER));
       return true;
      }
    protected:
      RegKey regKey;
    };

  };

};

#endif