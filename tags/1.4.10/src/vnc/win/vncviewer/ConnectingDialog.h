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

// -=- ConnectingDialog.h

// ConnectingDialog instances are used to display a status dialog while a
// connection attempt is in progress.  The connection attempt is performed
// in a background thread by the ConnectingDialog, to allow the status dialog
// to remain interactive.  If the dialog is cancelled then it will close and
// the connection dialog will eventually tidy itself up.

#ifndef __RFB_WIN32_CONNECTING_DLG_H__
#define __RFB_WIN32_CONNECTING_DLG_H__

#include <windows.h>
#include <network/Socket.h>
#include <rfb/util.h>
#include <rfb_win32/Handle.h>

namespace rfb {

  namespace win32 {

    class ConnectingDialog {
    public:
      ConnectingDialog();

      // connect
      //   Show a Connecting dialog and attempt to connect to the specified host
      //   in the background.
      //   If the connection succeeds then the Socket is returned.
      //   If an error occurs, an Exception is thrown.
      //   If the dialog is cancelled then null is returned.
      network::Socket* connect(const char* hostAndPort);
    protected:
      HWND dialog;
      network::Socket* newSocket;
      CharArray errMsg;
      Handle readyEvent;
      int dialogId;

      class Thread;
      friend class Thread;
    };

  };

};

#endif