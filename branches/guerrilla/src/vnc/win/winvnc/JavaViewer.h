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

// -=- JavaViewer.h

// Custom HTTPServer-derived class which serves the Java VNC Viewer
// to clients, using resource files compiled in to the WinVNC executable.

#ifndef WINVNC_JAVA_VIEWER
#define WINVNC_JAVA_VIEWER

#include <rfb/HTTPServer.h>
#include <rfb/VNCServerST.h>
#include <rdr/SubstitutingInStream.h>

namespace winvnc {

  class JavaViewerServer : public rfb::HTTPServer, public rdr::Substitutor {
  public:
    JavaViewerServer(rfb::VNCServerST* desktop);
    virtual ~JavaViewerServer();

    virtual rdr::InStream* getFile(const char* name, const char** contentType,
                                   int* contentLength, time_t* lastModified);

    // rdr::Substitutor callback
    virtual char* substitute(const char* varName);

    void setRFBport(int port) {
      rfbPort = port;
    }
  protected:
    int rfbPort;
    rfb::VNCServerST* server;
  };

};

#endif

