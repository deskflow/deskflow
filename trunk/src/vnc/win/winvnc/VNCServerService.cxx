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

// -=- WinVNC Version 4.0 Service-Mode implementation

#include <winvnc/VNCServerService.h>
#include <rfb_win32/OSVersion.h>

using namespace winvnc;
using namespace rfb;
using namespace win32;

const TCHAR* winvnc::VNCServerService::Name = _T("WinVNC4");


VNCServerService::VNCServerService(VNCServerWin32& s)
  : Service(Name), server(s) {
  // - Set the service-mode logging defaults
  //   These will be overridden by the Log option in the
  //   registry, if present.
  if (osVersion.isPlatformNT)
    logParams.setParam("*:EventLog:0,Connections:EventLog:100");
  else
    logParams.setParam("*:file:0,Connections:file:100");
}


DWORD VNCServerService::serviceMain(int argc, TCHAR* argv[]) {
  setStatus(SERVICE_RUNNING);
  int result = server.run();
  setStatus(SERVICE_STOP_PENDING);
  return result;
}

void VNCServerService::stop() {
  server.stop();
}
