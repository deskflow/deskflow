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

// -=- AddnewClientDialog.h

#ifndef __WINVNC_ADD_NEW_CLIENT_DIALOG_H__
#define __WINVNC_ADD_NEW_CLIENT_DIALOG_H__

#include <winvnc/resource.h>
#include <rfb_win32/Dialog.h>
//#include <rfb_win32/TCharArray.h>

namespace winvnc {

  class AddNewClientDialog : public rfb::win32::Dialog {
  public:
    AddNewClientDialog() : Dialog(GetModuleHandle(0)) {}
    // - Show the dialog and return true if OK was clicked,
    //   false in case of error or Cancel
    virtual bool showDialog() {
      return Dialog::showDialog(MAKEINTRESOURCE(IDD_ADD_NEW_CLIENT));
    }
    const char* getHostName() const {return hostName.buf;}
  protected:

    // Dialog methods (protected)
    virtual void initDialog() {
      if (hostName.buf)
        setItemString(IDC_HOST, rfb::TStr(hostName.buf));
    }
    virtual bool onOk() {
      hostName.replaceBuf(rfb::strDup(rfb::CStr(getItemString(IDC_HOST))));
      return true;
    }

    rfb::CharArray hostName;
  };

};

#endif
