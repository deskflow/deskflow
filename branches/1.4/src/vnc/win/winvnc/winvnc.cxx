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

// -=- VNC Server 4.0 for Windows (WinVNC4)

#include <string.h>
#ifdef WIN32
#define strcasecmp _stricmp
#endif

#include <winvnc/VNCServerWin32.h>
#include <winvnc/VNCServerService.h>
#include <winvnc/AddNewClientDialog.h>

#include <rfb/Logger_stdio.h>
#include <rfb/Logger_file.h>
#include <rfb/LogWriter.h>
#include <rfb_win32/AboutDialog.h>
#include <rfb_win32/MsgBox.h>
#include <network/TcpSocket.h>

using namespace winvnc;
using namespace rfb;
using namespace win32;

static LogWriter vlog("main");

TStr rfb::win32::AppName("VNC Server");


static bool runAsService = false;
static bool runServer = true;
static bool close_console = false;


//
// -=- processParams
//     Read in the command-line parameters and interpret them.
//

static void programInfo() {
  win32::FileVersionInfo inf;
  _tprintf(_T("%s - %s, Version %s\n"),
    inf.getVerString(_T("ProductName")),
    inf.getVerString(_T("FileDescription")),
    inf.getVerString(_T("FileVersion")));
  printf("%s\n", buildTime);
  _tprintf(_T("%s\n\n"), inf.getVerString(_T("LegalCopyright")));
}

static void programUsage() {
  printf("Command-line options:\n");
  printf("  -connect [<host[::port]>]            - Connect an existing WinVNC server to a listening viewer.\n");
  printf("  -disconnect                          - Disconnect all clients from an existing WinVNC server.\n");
  printf("  -register <options...>               - Register WinVNC server as a system service.\n");
  printf("  -unregister                          - Remove WinVNC server from the list of system services.\n");
  printf("  -start                               - Start the WinVNC server system service.\n");
  printf("  -stop                                - Stop the WinVNC server system service.\n");
  printf("  -status                              - Query the WinVNC service status.\n");
  printf("  -help                                - Provide usage information.\n");
  printf("  -noconsole                           - Run without a console (i.e. no stderr/stdout)\n");
  printf("  <setting>=<value>                    - Set the named configuration parameter.\n");
  printf("    (Parameter values specified on the command-line override those specified by other configuration methods.)\n");
  printf("\nLog names:\n");
  LogWriter::listLogWriters();
  printf("\nLog destinations:\n");
  Logger::listLoggers();
  printf("\nAvailable configuration parameters:\n");
  Configuration::listParams();
}

static void MsgBoxOrLog(const char* msg, bool isError=false) {
  if (close_console) {
    MsgBox(0, TStr(msg), (isError ? MB_ICONERROR : MB_ICONINFORMATION) | MB_OK);
  } else {
    if (isError) {
      try {
        vlog.error(msg);
        return;
      } catch (...) {
      }
    }
    fprintf(stderr, "%s\n", msg);
  }
}

static void processParams(int argc, const char* argv[]) {
  for (int i=1; i<argc; i++) {
    try {

      if (strcasecmp(argv[i], "-connect") == 0) {
        runServer = false;
        CharArray host;
        if (i+1 < argc) {
          host.buf = strDup(argv[i+1]);
          i++;
        } else {
          AddNewClientDialog ancd;
          if (ancd.showDialog())
            host.buf = strDup(ancd.getHostName());
        }
        if (host.buf) {
          HWND hwnd = FindWindow(0, _T("winvnc::IPC_Interface"));
          if (!hwnd)
            throw rdr::Exception("Unable to locate existing VNC Server.");
          COPYDATASTRUCT copyData;
          copyData.dwData = 1; // *** AddNewClient
          copyData.cbData = strlen(host.buf);
          copyData.lpData = (void*)host.buf;
          printf("Sending connect request to VNC Server...\n");
          if (!SendMessage(hwnd, WM_COPYDATA, 0, (LPARAM)&copyData))
            MsgBoxOrLog("Connection failed.", true);
        }
      } else if (strcasecmp(argv[i], "-disconnect") == 0) {
        runServer = false;
        HWND hwnd = FindWindow(0, _T("winvnc::IPC_Interface"));
        if (!hwnd)
          throw rdr::Exception("Unable to locate existing VNC Server.");
        COPYDATASTRUCT copyData;
        copyData.dwData = 2; // *** DisconnectClients
        copyData.lpData = 0;
        copyData.cbData = 0;
        printf("Sending disconnect request to VNC Server...\n");
        if (!SendMessage(hwnd, WM_COPYDATA, 0, (LPARAM)&copyData))
          MsgBoxOrLog("Failed to disconnect clients.", true);
      } else if (strcasecmp(argv[i], "-start") == 0) {
        printf("Attempting to start service...\n");
        runServer = false;
        if (rfb::win32::startService(VNCServerService::Name))
          MsgBoxOrLog("Started service successfully");
      } else if (strcasecmp(argv[i], "-stop") == 0) {
        printf("Attempting to stop service...\n");
        runServer = false;
        if (rfb::win32::stopService(VNCServerService::Name))
          MsgBoxOrLog("Stopped service successfully");
      } else if (strcasecmp(argv[i], "-status") == 0) {
        printf("Querying service status...\n");
        runServer = false;
        DWORD state = rfb::win32::getServiceState(VNCServerService::Name);
        CharArray stateStr(rfb::win32::serviceStateName(state));
        const char* stateMsg = "The %s Service is in the %s state.";
        CharArray result(strlen(stateStr.buf) + _tcslen(VNCServerService::Name) + strlen(stateMsg) + 1);
        sprintf(result.buf, stateMsg, (const char*)CStr(VNCServerService::Name), stateStr.buf);
        MsgBoxOrLog(result.buf);
      } else if (strcasecmp(argv[i], "-service") == 0) {
        printf("Run in service mode\n");
        runAsService = true;

      } else if (strcasecmp(argv[i], "-register") == 0) {
        printf("Attempting to register service...\n");
        runServer = false;
        int j = i;
        i = argc;
        if (rfb::win32::registerService(VNCServerService::Name,
                                        _T("VNC Server Version 4"),
                                        argc-(j+1), &argv[j+1]))
          MsgBoxOrLog("Registered service successfully");
      } else if (strcasecmp(argv[i], "-unregister") == 0) {
        printf("Attempting to unregister service...\n");
        runServer = false;
        if (rfb::win32::unregisterService(VNCServerService::Name))
          MsgBoxOrLog("Unregistered service successfully");

      } else if (strcasecmp(argv[i], "-noconsole") == 0) {
        close_console = true;
        vlog.info("closing console");
        if (!FreeConsole())
          vlog.info("unable to close console:%u", GetLastError());

      } else if ((strcasecmp(argv[i], "-help") == 0) ||
        (strcasecmp(argv[i], "--help") == 0) ||
        (strcasecmp(argv[i], "-h") == 0) ||
        (strcasecmp(argv[i], "/?") == 0)) {
        runServer = false;
        programUsage();
        break;

      } else {
        // Try to process <option>=<value>, or -<bool>
        if (Configuration::setParam(argv[i], true))
          continue;
        // Try to process -<option> <value>
        if ((argv[i][0] == '-') && (i+1 < argc)) {
          if (Configuration::setParam(&argv[i][1], argv[i+1], true)) {
            i++;
            continue;
          }
        }
        // Nope.  Show them usage and don't run the server
        runServer = false;
        programUsage();
        break;
      }

    } catch (rdr::Exception& e) {
      MsgBoxOrLog(e.str(), true);
    }
  }
}


//
// -=- main
//

int vncmain(int argc, const char* argv[]) {
  int result = 0;

  try {
    // - Initialise the available loggers
    //freopen("\\\\drupe\\tjr\\WinVNC4.log","ab",stderr);
    //setbuf(stderr, 0);
    initStdIOLoggers();
    initFileLogger("C:\\temp\\WinVNC4.log");
    rfb::win32::initEventLogLogger(VNCServerService::Name);

    // - By default, just log errors to stderr
    logParams.setParam("*:stderr:0");

    // - Print program details and process the command line
    //programInfo();
    processParams(argc, argv);

    // - Run the server if required
    if (runServer) {
      // Start the network subsystem and run the server
      VNCServerWin32 server;

      if (runAsService) {
        printf("Starting Service-Mode VNC Server.\n");
        VNCServerService service(server);
        service.start();
        result = service.getStatus().dwWin32ExitCode;
      } else {
        printf("Starting User-Mode VNC Server.\n");
        result = server.run();
      }
    }

    vlog.debug("WinVNC service destroyed");
  } catch (rdr::Exception& e) {
    MsgBoxOrLog(e.str(), true);
  }

  vlog.debug("WinVNC process quitting");
  return result;
}
