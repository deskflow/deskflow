/* Copyright (C) 2005 RealVNC Ltd.  All Rights Reserved.
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
#include <winvnc/ManagedListener.h>
#include <rfb/LogWriter.h>

using namespace winvnc;
using namespace rfb;
using namespace win32;

static LogWriter vlog("ManagedListener");


ManagedListener::ManagedListener(SocketManager* mgr)
: sock(0), filter(0), manager(mgr), addrChangeNotifier(0), server(0), port(0), localOnly(false) {
}

ManagedListener::~ManagedListener() {
  if (sock)
    manager->remListener(sock);
  delete filter;
}


void ManagedListener::setServer(network::SocketServer* svr) {
  if (svr == server)
    return;
  vlog.info("set server to %p", svr);
  server = svr;
  refresh();
}

void ManagedListener::setPort(int port_, bool localOnly_) {
  if ((port_ == port) && (localOnly == localOnly_))
    return;
  vlog.info("set port to %d", port_);
  port = port_;
  localOnly = localOnly_;
  refresh();
}

void ManagedListener::setFilter(const char* filterStr) {
  vlog.info("set filter to %s", filterStr);
  delete filter;
  filter = new network::TcpFilter(filterStr);
  if (sock && !localOnly)
    sock->setFilter(filter);
}

void ManagedListener::setAddressChangeNotifier(SocketManager::AddressChangeNotifier* acn) {
  if (acn == addrChangeNotifier)
    return;
  addrChangeNotifier = acn;
  refresh();
}


void ManagedListener::refresh() {
  if (sock)
    manager->remListener(sock);
  sock = 0;
  if (!server)
    return;
  try {
    if (port)
      sock = new network::TcpListener(port, localOnly);
  } catch (rdr::Exception& e) {
    vlog.error(e.str());
  }
  if (sock) {
    if (!localOnly)
      sock->setFilter(filter);
    try {
      manager->addListener(sock, server, addrChangeNotifier);
    } catch (...) {
      sock = 0;
      throw;
    }
  }
}
