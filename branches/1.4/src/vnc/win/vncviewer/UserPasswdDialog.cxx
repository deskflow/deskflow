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

#include <vncviewer/UserPasswdDialog.h>
#include <vncviewer/resource.h>
#include <rfb/Exception.h>

using namespace rfb;
using namespace rfb::win32;


UserPasswdDialog::UserPasswdDialog() : Dialog(GetModuleHandle(0)),
  showUsername(false), showPassword(false) {
}


void UserPasswdDialog::setCSecurity(const CSecurity* cs) {
  description.replaceBuf(tstrDup(cs->description()));
}

bool UserPasswdDialog::showDialog() {
  return Dialog::showDialog(MAKEINTRESOURCE(IDD_VNC_AUTH_DLG));
}

void UserPasswdDialog::initDialog() {
  if (username.buf)
    setItemString(IDC_USERNAME, username.buf);
  if (password.buf)
    setItemString(IDC_PASSWORD, password.buf);
  if (!showUsername) {
    setItemString(IDC_USERNAME, _T(""));
    enableItem(IDC_USERNAME, false);
  }
  if (!showPassword) {
    setItemString(IDC_PASSWORD, _T(""));
    enableItem(IDC_PASSWORD, false);
  }
  if (description.buf) {
    TCharArray title(128);
    GetWindowText(handle, title.buf, 128);
    _tcsncat(title.buf, _T(" ["), 128);
    _tcsncat(title.buf, description.buf, 128);
    _tcsncat(title.buf, _T("]"), 128);
    SetWindowText(handle, title.buf);
  }
}

bool UserPasswdDialog::onOk() {
	username.replaceBuf(getItemString(IDC_USERNAME));
	password.replaceBuf(getItemString(IDC_PASSWORD));
  return true;
}


void UserPasswdDialog::getUserPasswd(char** user, char** passwd) {
  showUsername = user != 0;
  showPassword = passwd != 0;
  if (user && *user)
    username.replaceBuf(tstrDup(*user));
  if (passwd && *passwd)
    password.replaceBuf(tstrDup(*passwd));

  //if (!showDialog())
    //throw rfb::AuthCancelledException();

  if (user)
    *user = strDup(username.buf);
  if (passwd)
    *passwd = strDup(password.buf);
}
