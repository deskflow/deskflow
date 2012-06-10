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

#ifndef __WINVNC_SERVICEMODE_H__
#define __WINVNC_SERVICEMODE_H__

#include <winvnc/VNCServerWin32.h>
#include <rfb_win32/Service.h>

namespace winvnc {

  class VNCServerService : public rfb::win32::Service {
  public:
    VNCServerService(VNCServerWin32& s);

    DWORD serviceMain(int argc, TCHAR* argv[]);
    void stop();

    static const TCHAR* Name;
  protected:
    VNCServerWin32& server;
  };

};

#endif
