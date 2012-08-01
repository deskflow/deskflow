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

// -=- TcpSocket.h - base-class for TCP stream sockets.
//     This header also defines the TcpListener class, used
//     to listen for incoming socket connections over TCP
//
//     NB: Any file descriptors created by the TcpSocket or
//     TcpListener classes are close-on-exec if the OS supports
//     it.  TcpSockets initialised with a caller-supplied fd
//     are NOT set to close-on-exec.

#ifndef __NETWORK_TCP_SOCKET_H__
#define __NETWORK_TCP_SOCKET_H__

#include <network/Socket.h>

#include <list>

namespace network {

  class TcpSocket : public Socket {
  public:
    TcpSocket(int sock, bool close=true);
    TcpSocket(const char *name, int port);
    virtual ~TcpSocket();

    virtual char* getMyAddress();
    virtual int getMyPort();
    virtual char* getMyEndpoint();

    virtual char* getPeerAddress();
    virtual int getPeerPort();
    virtual char* getPeerEndpoint();
    virtual bool sameMachine();

    virtual void shutdown();

    static bool enableNagles(int sock, bool enable);
    static bool isSocket(int sock);
    static bool isConnected(int sock);
    static int getSockPort(int sock);
  private:
    bool closeFd;
  };

  class TcpListener : public SocketListener {
  public:
    TcpListener(int port, bool localhostOnly=false, int sock=-1,
                bool close=true);
    virtual ~TcpListener();

    virtual void shutdown();
    virtual Socket* accept();

    void getMyAddresses(std::list<char*>* addrs);
    int getMyPort();

  private:
    bool closeFd;
  };

  class TcpFilter : public ConnectionFilter {
  public:
    TcpFilter(const char* filter);
    virtual ~TcpFilter();

    virtual bool verifyConnection(Socket* s);

    typedef enum {Accept, Reject, Query} Action;
    struct Pattern {
      Action action;
      unsigned long address;
      unsigned long mask;
    };
    static Pattern parsePattern(const char* s);
    static char* patternToStr(const Pattern& p);
  protected:
    std::list<Pattern> filter;
  };

}

#endif // __NETWORK_TCP_SOCKET_H__
