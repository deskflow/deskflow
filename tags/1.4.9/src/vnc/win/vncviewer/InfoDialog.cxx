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
#include <vncviewer/InfoDialog.h>
#include <vncviewer/resource.h>
#include <vncviewer/CConn.h>
#include <rfb/secTypes.h>
#include <rfb/encodings.h>
#include <rfb/CSecurity.h>
#include <rfb/LogWriter.h>

using namespace rfb;
using namespace rfb::win32;

static LogWriter vlog("Info");


bool InfoDialog::showDialog(CConn* cc) {
  conn = cc;
  return Dialog::showDialog(MAKEINTRESOURCE(IDD_CONNECTION_INFO));
}

void InfoDialog::initDialog() {
  char buf[256];

  setItemString(IDC_INFO_NAME, TStr(conn->cp.name()));

  setItemString(IDC_INFO_HOST, TCharArray(conn->getSocket()->getPeerAddress()).buf);

  sprintf(buf, "%dx%d", conn->cp.width, conn->cp.height);
  setItemString(IDC_INFO_SIZE, TStr(buf));

  conn->cp.pf().print(buf, 256);
  setItemString(IDC_INFO_PF, TStr(buf));

  conn->getServerDefaultPF().print(buf, 256);
  setItemString(IDC_INFO_DEF_PF, TStr(buf));

  setItemString(IDC_REQUESTED_ENCODING, TStr(encodingName(conn->getOptions().preferredEncoding)));
  setItemString(IDC_LAST_ENCODING, TStr(encodingName(conn->lastUsedEncoding())));

  sprintf(buf, "%d kbits/s", conn->getSocket()->inStream().kbitsPerSecond());
  setItemString(IDC_INFO_LINESPEED, TStr(buf));

  sprintf(buf, "%d.%d", conn->cp.majorVersion, conn->cp.minorVersion);
  setItemString(IDC_INFO_VERSION, TStr(buf));

  const CSecurity* cSec = conn->getCurrentCSecurity();
  setItemString(IDC_INFO_SECURITY, TStr(secTypeName(cSec->getType())));
  setItemString(IDC_INFO_ENCRYPTION, TStr(cSec->description()));
}
