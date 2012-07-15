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

// -=- LaunchProcess.h

// Helper class to launch a names process from the same directory as
// the current process executable resides in.

#ifndef __RFB_WIN32_LAUNCHPROCESS_H__
#define __RFB_WIN32_LAUNCHPROCESS_H__

#include <windows.h>
#include <rfb_win32/TCharArray.h>

namespace rfb {

  namespace win32 {

    class LaunchProcess {
    public:
      LaunchProcess(const TCHAR* exeName_, const TCHAR* params);
      ~LaunchProcess();

      // start() starts the specified process with the supplied
      //   command-line.
      //   If userToken is INVALID_HANDLE_VALUE then starts the process
      //   as the current user, otherwise as the specified user.
      //   If createConsole is true then CREATE_CONSOLE_WINDOW is passed
      //   as an extra flag to the process creation call.
      void start(HANDLE userToken, bool createConsole=false);

      // Detatch from the child process. After detatching from a child
      //   process, no other methods should be called on the object
      //   that started it
      void detach();

      // Wait for the process to quit, up to the specified timeout, and
      //   close the handles to it once it has quit.
      //   If the process quits within the timeout then true is returned
      //   and returnCode is set. If it has not quit then false is returned.
      //   If an error occurs then an exception will be thrown.
      bool await(DWORD timeoutMs=INFINITE);

      PROCESS_INFORMATION procInfo;
      DWORD returnCode;
    protected:
      TCharArray exeName;
      TCharArray params;
    };


  };

};

#endif
