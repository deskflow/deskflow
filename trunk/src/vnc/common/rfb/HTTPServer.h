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

// -=- HTTPServer.h

// Single-threaded HTTP server implementation.
// All I/O is handled by the processSocketEvent routine,
// which is called by the main-loop of the VNC server whenever
// there is an event on an HTTP socket.

#ifndef __RFB_HTTP_SERVER_H__
#define __RFB_HTTP_SERVER_H__

#include <list>

#include <rdr/MemInStream.h>
#include <rfb/UpdateTracker.h>
#include <rfb/Configuration.h>
#include <network/Socket.h>
#include <time.h>

namespace rfb {

  class HTTPServer : public network::SocketServer {
  public:
    // -=- Constructors

    // - HTTPServer(files)
    //   Create an HTTP server which will use the getFile method
    //   to satisfy HTTP GET requests.
    HTTPServer();

    virtual ~HTTPServer();

    // SocketServer interface

    // addSocket()
    //   This causes the server to perform HTTP protocol on the
    //   supplied socket.
    virtual void addSocket(network::Socket* sock, bool outgoing=false);

    // removeSocket()
    //   Could clean up socket-specific resources here.
    virtual void removeSocket(network::Socket* sock);

    // processSocketEvent()
    //   The platform-specific side of the server implementation calls
    //   this method whenever data arrives on one of the active
    //   network sockets.
    virtual void processSocketEvent(network::Socket* sock);

    // Check for socket timeouts
    virtual int checkTimeouts();


    // getSockets() gets a list of sockets.  This can be used to generate an
    // fd_set for calling select().

    virtual void getSockets(std::list<network::Socket*>* sockets);


    // -=- File interface

    // - getFile is passed the path portion of a URL and returns an
    //   InStream containing the data to return.  If the requested
    //   file is available then the contentType should be set to the
    //   type of the file, or left untouched if the file type is to
    //   be determined automatically by HTTPServer.
    //   If the file is not available then null is returned.
    //   Overridden getFile functions should call the default version
    //   if they do not recognise a path name.
    //   NB: The caller assumes ownership of the returned InStream.
    //   NB: The contentType is statically allocated by the getFile impl.
    //   NB: contentType is *guaranteed* to be valid when getFile is called.

    virtual rdr::InStream* getFile(const char* name, const char** contentType,
                                   int* contentLength, time_t* lastModified);

    // - guessContentType is passed the name of a file and returns the
    //   name of an HTTP content type, based on the file's extension.  If
    //   the extension isn't recognised then defType is returned.  This can
    //   be used from getFile to easily default to the supplied contentType,
    //   or by passing zero in to determine whether a type is recognised or
    //   not.

    static const char* guessContentType(const char* name, const char* defType);

  protected:
    class Session;
    std::list<Session*> sessions;
  };
}

#endif

