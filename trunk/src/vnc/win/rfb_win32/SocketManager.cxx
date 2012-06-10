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

// -=- SocketManager.cxx

#include <winsock2.h>
#include <list>
#include <rfb/LogWriter.h>
#include <rfb_win32/SocketManager.h>

using namespace rfb;
using namespace rfb::win32;

static LogWriter vlog("SocketManager");


// -=- SocketManager

SocketManager::SocketManager() {
}

SocketManager::~SocketManager() {
}


static void requestAddressChangeEvents(network::SocketListener* sock_) {
  DWORD dummy = 0;
  if (WSAIoctl(sock_->getFd(), SIO_ADDRESS_LIST_CHANGE, 0, 0, 0, 0, &dummy, 0, 0) == SOCKET_ERROR) {
    DWORD err = WSAGetLastError();
    if (err != WSAEWOULDBLOCK)
      vlog.error("Unable to track address changes", err);
  }
}


void SocketManager::addListener(network::SocketListener* sock_,
                                network::SocketServer* srvr,
                                AddressChangeNotifier* acn) {
  WSAEVENT event = WSACreateEvent();
  long flags = FD_ACCEPT | FD_CLOSE;
  if (acn)
    flags |= FD_ADDRESS_LIST_CHANGE;
  try {
    if (event && (WSAEventSelect(sock_->getFd(), event, flags) == SOCKET_ERROR))
      throw rdr::SystemException("Unable to select on listener", WSAGetLastError());

    // requestAddressChangeEvents MUST happen after WSAEventSelect, so that the socket is non-blocking
    if (acn)
      requestAddressChangeEvents(sock_);

    // addEvent is the last thing we do, so that the event is NOT registered if previous steps fail
    if (!event || !addEvent(event, this))
      throw rdr::Exception("Unable to add listener");
  } catch (rdr::Exception& e) {
    if (event)
      WSACloseEvent(event);
    delete sock_;
    vlog.error(e.str());
    throw;
  }

  ListenInfo li;
  li.sock = sock_;
  li.server = srvr;
  li.notifier = acn;
  listeners[event] = li;
}

void SocketManager::remListener(network::SocketListener* sock) {
  std::map<HANDLE,ListenInfo>::iterator i;
  for (i=listeners.begin(); i!=listeners.end(); i++) {
    if (i->second.sock == sock) {
      removeEvent(i->first);
      WSACloseEvent(i->first);
      delete sock;
      listeners.erase(i);
      return;
    }
  }
  throw rdr::Exception("Listener not registered");
}


void SocketManager::addSocket(network::Socket* sock_, network::SocketServer* srvr, bool outgoing) {
  WSAEVENT event = WSACreateEvent();
  if (!event || !addEvent(event, this) ||
      (WSAEventSelect(sock_->getFd(), event, FD_READ | FD_CLOSE) == SOCKET_ERROR)) {
    if (event)
      WSACloseEvent(event);
    delete sock_;
    vlog.error("Unable to add connection");
    return;
  }
  ConnInfo ci;
  ci.sock = sock_;
  ci.server = srvr;
  connections[event] = ci;
  srvr->addSocket(sock_, outgoing);
}

void SocketManager::remSocket(network::Socket* sock_) {
  std::map<HANDLE,ConnInfo>::iterator i;
  for (i=connections.begin(); i!=connections.end(); i++) {
    if (i->second.sock == sock_) {
      i->second.server->removeSocket(sock_);
      removeEvent(i->first);
      WSACloseEvent(i->first);
      delete sock_;
      connections.erase(i);
      return;
    }
  }
  throw rdr::Exception("Socket not registered");
}


int SocketManager::checkTimeouts() {
  network::SocketServer* server = 0;
  int timeout = EventManager::checkTimeouts();

  std::map<HANDLE,ListenInfo>::iterator i;
  for (i=listeners.begin(); i!=listeners.end(); i++)
    soonestTimeout(&timeout, i->second.server->checkTimeouts());

  std::list<network::Socket*> shutdownSocks;
  std::map<HANDLE,ConnInfo>::iterator j, j_next;
  for (j=connections.begin(); j!=connections.end(); j=j_next) {
    j_next = j; j_next++;
    if (j->second.sock->isShutdown())
      shutdownSocks.push_back(j->second.sock);
  }

  std::list<network::Socket*>::iterator k;
  for (k=shutdownSocks.begin(); k!=shutdownSocks.end(); k++)
    remSocket(*k);

  return timeout;
}


void SocketManager::processEvent(HANDLE event) {
  if (listeners.count(event)) {
    ListenInfo li = listeners[event];

    // Accept an incoming connection
    vlog.debug("accepting incoming connection");

    // What kind of event is this?
    WSANETWORKEVENTS network_events;
    WSAEnumNetworkEvents(li.sock->getFd(), event, &network_events);
    if (network_events.lNetworkEvents & FD_ACCEPT) {
      network::Socket* new_sock = li.sock->accept();
      if (new_sock)
        addSocket(new_sock, li.server, false);
    } else if (network_events.lNetworkEvents & FD_CLOSE) {
      vlog.info("deleting listening socket");
      remListener(li.sock);
    } else if (network_events.lNetworkEvents & FD_ADDRESS_LIST_CHANGE) {
      li.notifier->processAddressChange(li.sock);
      DWORD dummy = 0;
      requestAddressChangeEvents(li.sock);
    } else {
      vlog.error("unknown listener event: %lx", network_events.lNetworkEvents);
    }
  } else if (connections.count(event)) {
    ConnInfo ci = connections[event];

    try {
      // Process data from an active connection

      // Cancel event notification for this socket
      if (WSAEventSelect(ci.sock->getFd(), event, 0) == SOCKET_ERROR)
        throw rdr::SystemException("unable to disable WSAEventSelect:%u", WSAGetLastError());

      // Reset the event object
      WSAResetEvent(event);

      // Call the socket server to process the event
      ci.server->processSocketEvent(ci.sock);
      if (ci.sock->isShutdown()) {
        remSocket(ci.sock);
        return;
      }

      // Re-instate the required socket event
      // If the read event is still valid, the event object gets set here
      if (WSAEventSelect(ci.sock->getFd(), event, FD_READ | FD_CLOSE) == SOCKET_ERROR)
        throw rdr::SystemException("unable to re-enable WSAEventSelect:%u", WSAGetLastError());
    } catch (rdr::Exception& e) {
      vlog.error(e.str());
      remSocket(ci.sock);
    }
  }
}
