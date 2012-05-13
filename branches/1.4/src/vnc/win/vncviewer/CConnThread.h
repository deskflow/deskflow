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

// -=- CConnThread.h

// CConn-managing Thread implementation.

#ifndef __RFB_WIN32_CCONN_THREAD_H__
#define __RFB_WIN32_CCONN_THREAD_H__

#include <network/Socket.h>
#include <rfb/Threading.h>
#include <rfb/util.h>

namespace rfb {

  namespace win32 {

    class CConnThread : public Thread {
    public:
      CConnThread();
      CConnThread(const char* hostOrConfig, bool isConfig=false);
      CConnThread(network::Socket* sock, bool reverse=false);
      ~CConnThread();

      void run();

      // Special getMessage call that returns FALSE if message is WM_QUIT,
      // OR if there are no more CConnThreads running.
      static BOOL getMessage(MSG* msg, HWND hwnd, UINT minMsg, UINT maxMsg);
    protected:
      CharArray hostOrConfig;
      bool isConfig;
      network::Socket* sock;
      bool reverse;
    };

  };

};

#endif // __RFB_WIN32_CCONN_THREAD_H__
