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

// -=- ListenServer.h

#ifndef __RFB_WIN32_LISTEN_SERVER_H__
#define __RFB_WIN32_LISTEN_SERVER_H__

#include <windows.h>
//#include <winsock2.h>
#include <network/Socket.h>
#include <rfb_win32/MsgWindow.h>
#include <vncviewer/CConnThread.h>


namespace rfb {
  namespace win32 {

    class ListenServer : MsgWindow {
    public:
      ListenServer(network::SocketListener* l) : MsgWindow(_T("rfb::win32::ListenServer")), sock(l) {
        if (WSAAsyncSelect(l->getFd(), getHandle(), WM_USER, FD_ACCEPT) == SOCKET_ERROR)
          throw rdr::SystemException("unable to monitor listen socket", WSAGetLastError());
      }

      LRESULT processMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
        if (msg == WM_USER) {
          network::Socket* newConn = sock->accept();
          Thread* newThread = new CConnThread(newConn, true);
          return 0;
        }
        return MsgWindow::processMessage(msg, wParam, lParam);
      }
    protected:
      network::SocketListener* sock;
    };

  };
};

#endif