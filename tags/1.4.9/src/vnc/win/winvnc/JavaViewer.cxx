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

#include <windows.h>
#include <winvnc/JavaViewer.h>
#include <winvnc/resource.h>
#include <rdr/MemInStream.h>
#include <rfb/LogWriter.h>
#include <rfb/VNCserverST.h>
#include <rfb_win32/TCharArray.h>

#define strcasecmp _stricmp

using namespace winvnc;
using namespace rfb;


static rfb::LogWriter vlog("JavaViewerServer");

JavaViewerServer::JavaViewerServer(rfb::VNCServerST* svr) : server(svr) {
}

JavaViewerServer::~JavaViewerServer() {
}

rdr::InStream* JavaViewerServer::getFile(const char* name,
                                         const char** contentType,
                                         int* contentLength,
                                         time_t* lastModified)
{
  if (strcmp(name, "/") == 0)
    name = "/index.vnc";

  HRSRC resource = FindResource(0, TStr(name), _T("HTTPFILE"));
  if (!resource) return 0;
  HGLOBAL handle = LoadResource(0, resource);
  if (!handle) return 0;
  void* buf = LockResource(handle);
  int len = SizeofResource(0, resource);

  rdr::InStream* is = new rdr::MemInStream(buf, len);
  if (strlen(name) > 4 && strcasecmp(&name[strlen(name)-4], ".vnc") == 0) {
    is = new rdr::SubstitutingInStream(is, this, 20);
    *contentType = "text/html";
  }
  return is;
}

char* JavaViewerServer::substitute(const char* varName)
{
  if (strcmp(varName, "$$") == 0) {
    return rfb::strDup("$");
  }
  if (strcmp(varName, "$PORT") == 0) {
    char* str = new char[10];
    sprintf(str, "%d", rfbPort);
    return str;
  }
  if (strcmp(varName, "$WIDTH") == 0) {
    char* str = new char[10];
    sprintf(str, "%d", server->getDesktopSize().x);
    return str;
  }
  if (strcmp(varName, "$HEIGHT") == 0) {
    char* str = new char[10];
    sprintf(str, "%d", server->getDesktopSize().y);
    return str;
  }
  if (strcmp(varName, "$APPLETWIDTH") == 0) {
    char* str = new char[10];
    sprintf(str, "%d", server->getDesktopSize().x);
    return str;
  }
  if (strcmp(varName, "$APPLETHEIGHT") == 0) {
    char* str = new char[10];
    sprintf(str, "%d", server->getDesktopSize().y + 32);
    return str;
  }
  if (strcmp(varName, "$DESKTOP") == 0) {
    return rfb::strDup(server->getName());
  }
  return 0;
}
