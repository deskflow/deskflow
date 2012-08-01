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

#ifndef __VNCSERVER_MANAGED_LISTENER_H__
#define __VNCSERVER_MANAGED_LISTENER_H__

////#include <winsock2.h>
#include <network/TcpSocket.h>
#include <rfb_win32/SocketManager.h>

namespace winvnc {

  // -=- ManagedListener
  //     Wrapper class which simplifies the management of a listening socket
  //     on a specified port, attached to a SocketManager and SocketServer.
  //     Reopens sockets & reconfigures filters & callbacks as appropriate.
  //     Handles addition/removal of Listeners from SocketManager internally.

  class ManagedListener {
  public:
    ManagedListener(rfb::win32::SocketManager* mgr);
    ~ManagedListener();
    
    void setServer(network::SocketServer* svr);
    void setPort(int port, bool localOnly=false);
    void setFilter(const char* filter);
    void setAddressChangeNotifier(rfb::win32::SocketManager::AddressChangeNotifier* acn);
  
    network::TcpListener* sock;
  protected:
    void refresh();
    network::TcpFilter* filter;
    rfb::win32::SocketManager* manager;
    rfb::win32::SocketManager::AddressChangeNotifier* addrChangeNotifier;
    network::SocketServer* server;
    int port;
    bool localOnly;
  };

};

#endif
