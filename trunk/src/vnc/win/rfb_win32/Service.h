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

// -=- Service.h
//
// Win32 service-mode code.
// Derive your service from this code and let it handle the annoying Win32
// service API.
// The underlying implementation takes care of the differences between
// Windows NT and Windows 95 based systems

#ifndef __RFB_WIN32_SERVICE_H__
#define __RFB_WIN32_SERVICE_H__

#include <windows.h>

namespace rfb {

  namespace win32 {

    //
    // -=- Service
    //

    // Application base-class for services.

    class Service {
    public:

      Service(const TCHAR* name_);
      virtual ~Service();

      const TCHAR* getName() {return name;}
      SERVICE_STATUS& getStatus() {return status;}

      void setStatus(DWORD status);
      void setStatus();

      // - Start the service, having initialised it
      void start();

      // - Service main procedure - override to implement a service
      virtual DWORD serviceMain(int argc, TCHAR* argv[]) = 0;

      // - Service control notifications

      // To get notified when the OS is shutting down
      virtual void osShuttingDown() {};

      // To get notified when the service parameters change
      virtual void readParams() {};

      // To cause the serviceMain() routine to return
      virtual void stop() {};

    public:
      SERVICE_STATUS_HANDLE status_handle;
      SERVICE_STATUS status;
    protected:
      const TCHAR* name;
    };

    class ServiceHandle {
    public:
      ServiceHandle(SC_HANDLE h) : handle(h) {}
      ~ServiceHandle() {CloseServiceHandle(handle);}
      operator SC_HANDLE() const {return handle;}
    protected:
      SC_HANDLE handle;
    };

    // -=- Routines used by desktop back-end code to manage desktops/window stations

    //     Returns false under Win9x
    bool desktopChangeRequired();

    //     Returns true under Win9x
    bool changeDesktop();

    // -=- Routines used by the SInput Keyboard class to emulate Ctrl-Alt-Del
    //     Returns false under Win9x
    bool emulateCtrlAltDel();

    // -=- Routines to initialise the Event Log target Logger
    //     Returns false under Win9x
    bool initEventLogLogger(const TCHAR* srcname);

    // -=- Routines to register/unregister the service
    //     These routines also take care of registering the required
    //     event source information, etc.
    // *** should really accept TCHAR argv

    bool registerService(const TCHAR* name, const TCHAR* desc, int argc, const char* argv[]);
    bool unregisterService(const TCHAR* name);

    bool startService(const TCHAR* name);
    bool stopService(const TCHAR* name);

    // -=- Get the state of the named service (one of the NT service state values)
    DWORD getServiceState(const TCHAR* name);

    // -=- Convert a supplied service state value to a printable string e.g. Running, Stopped...
    //     The caller must delete the returned string buffer
    char* serviceStateName(DWORD state);

    // -=- Routine to determine whether the host process is running a service
    bool isServiceProcess();

  };

};

#endif // __RFB_WIN32_SERVICE_NT_H__
