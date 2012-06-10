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

// -=- ConnectionDialog.h

// Connection dialog for VNC Viewer 4.0

#ifndef __RFB_WIN32_CONN_DIALOG_H__
#define __RFB_WIN32_CONN_DIALOG_H__

#include <rfb_win32/Dialog.h>
#include <vncviewer/MRU.h>
#include <rfb/util.h>

namespace rfb {

  namespace win32 {

    class CConn;

    class ConnectionDialog : Dialog {
    public:
      ConnectionDialog(CConn* view);
      virtual bool showDialog();
      virtual void initDialog();
      virtual bool onOk();
      virtual bool onCommand(int id, int cmd);
      TCharArray hostname;
    protected:
      CConn* conn;
    };

  };

};

#endif
