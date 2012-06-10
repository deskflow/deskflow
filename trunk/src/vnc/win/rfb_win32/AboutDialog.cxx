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

#include <rfb_win32/AboutDialog.h>
#include <rfb_win32/Win32Util.h>
#include <rfb_win32/TCharArray.h>
#include <rfb/LogWriter.h>

using namespace rfb;
using namespace rfb::win32;

static LogWriter vlog("AboutDialog");

AboutDialog AboutDialog::instance;


AboutDialog::AboutDialog() : Dialog(GetModuleHandle(0)) {
}

bool AboutDialog::showDialog() {
  return Dialog::showDialog(MAKEINTRESOURCE(DialogId));
}

void AboutDialog::initDialog() {
  // Set the build time field
  SetWindowText(GetDlgItem(handle, BuildTime), TStr(buildTime));

  // Get our executable's version info
  FileVersionInfo verInfo;

  SetWindowText(GetDlgItem(handle, Version), verInfo.getVerString(_T("ProductVersion")));
  SetWindowText(GetDlgItem(handle, Copyright), verInfo.getVerString(_T("LegalCopyright")));
  SetWindowText(GetDlgItem(handle, Description), verInfo.getVerString(_T("ProductName")));
}
