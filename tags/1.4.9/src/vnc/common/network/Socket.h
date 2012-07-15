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

// -=- Socket.h - abstract base-class for any kind of network stream/socket

#ifndef __NETWORK_SOCKET_H__
#define __NETWORK_SOCKET_H__

#include <limits.h>
#include <rdr/FdInStream.h>
#include <rdr/FdOutStream.h>
#include <rdr/Exception.h>

namespace network {

  class Socket {
  public:
    Socket(int fd)
      : instream(new rdr::FdInStream(fd)),
      outstream(new rdr::FdOutStream(fd)),
      ownStreams(true), isShutdown_(false),
      queryConnection(false) {}
    virtual ~Socket() {
      if (ownStreams) {
        delete instream;
        delete outstream;
      }
    }
    rdr::FdInStream &inStream() {return *instream;}
    rdr::FdOutStream &outStream() {return *outstream;}
    int getFd() {return outstream->getFd();}

    // if shutdown() is overridden then the override MUST call on to here
    virtual void shutdown() {isShutdown_ = true;}
    bool isShutdown() const {return isShutdown_;}

    // information about this end of the socket
    virtual char* getMyAddress() = 0; // a string e.g. "192.168.0.1"
    virtual int getMyPort() = 0;
    virtual char* getMyEndpoint() = 0; // <address>::<port>

    // information about the remote end of the socket
    virtual char* getPeerAddress() = 0; // a string e.g. "192.168.0.1"
    virtual int getPeerPort() = 0;
    virtual char* getPeerEndpoint() = 0; // <address>::<port>

    // Is the remote end on the same machine?
    virtual bool sameMachine() = 0;

    // Was there a "?" in the ConnectionFilter used to accept this Socket?
    void setRequiresQuery() {queryConnection = true;}
    bool requiresQuery() const {return queryConnection;}

  protected:
    Socket() : instream(0), outstream(0), ownStreams(false),
      isShutdown_(false), queryConnection(false) {}
    Socket(rdr::FdInStream* i, rdr::FdOutStream* o, bool own)
      : instream(i), outstream(o), ownStreams(own),
      isShutdown_(false), queryConnection(false) {}
    rdr::FdInStream* instream;
    rdr::FdOutStream* outstream;
    bool ownStreams;
    bool isShutdown_;
    bool queryConnection;
  };

  class ConnectionFilter {
  public:
    virtual bool verifyConnection(Socket* s) = 0;
  };

  class SocketListener {
  public:
    SocketListener() : fd(0), filter(0) {}
    virtual ~SocketListener() {}

    // shutdown() stops the socket from accepting further connections
    virtual void shutdown() = 0;

    // accept() returns a new Socket object if there is a connection
    // attempt in progress AND if the connection passes the filter
    // if one is installed.  Otherwise, returns 0.
    virtual Socket* accept() = 0;

    // setFilter() applies the specified filter to all new connections
    void setFilter(ConnectionFilter* f) {filter = f;}
    int getFd() {return fd;}
  protected:
    int fd;
    ConnectionFilter* filter;
  };

  struct SocketException : public rdr::SystemException {
    SocketException(const char* text, int err_) : rdr::SystemException(text, err_) {}
  };

  class SocketServer {
  public:
    virtual ~SocketServer() {}

    // addSocket() tells the server to serve the Socket.  The caller
    //   retains ownership of the Socket - the only way for the server
    //   to discard a Socket is by calling shutdown() on it.
    //   outgoing is set to true if the socket was created by connecting out
    //   to another host, or false if the socket was created by accept()ing
    //   an incoming connection.
    virtual void addSocket(network::Socket* sock, bool outgoing=false) = 0;

    // removeSocket() tells the server to stop serving the Socket.  The
    //   caller retains ownership of the Socket - the server must NOT
    //   delete the Socket!  This call is used mainly to cause per-Socket
    //   resources to be freed.
    virtual void removeSocket(network::Socket* sock) = 0;

    // processSocketEvent() tells the server there is a Socket read event.
    //   The implementation can indicate that the Socket is no longer active
    //   by calling shutdown() on it.  The caller will then call removeSocket()
    //   soon after processSocketEvent returns, to allow any pre-Socket
    //   resources to be tidied up.
    virtual void processSocketEvent(network::Socket* sock) = 0;

    // checkTimeouts() allows the server to check socket timeouts, etc.  The
    //   return value is the number of milliseconds to wait before
    //   checkTimeouts() should be called again.  If this number is zero then
    //   there is no timeout and checkTimeouts() should be called the next time
    //   an event occurs.
    virtual int checkTimeouts() = 0;
  };

}

#endif // __NETWORK_SOCKET_H__
