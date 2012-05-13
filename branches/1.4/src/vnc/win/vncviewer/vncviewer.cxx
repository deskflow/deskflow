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

// -=- VNC Viewer for Win32

#include <string.h>
#ifdef WIN32
#define strcasecmp _stricmp
#endif
#include <list>

#include <vncviewer/resource.h>
#include <vncviewer/CConn.h>
#include <vncviewer/CConnThread.h>
#include <vncviewer/OptionsDialog.h>
#include <vncviewer/ListenServer.h>
#include <vncviewer/ListenTrayIcon.h>
#include <network/TcpSocket.h>
#include <rfb/Logger_stdio.h>
#include <rfb/Logger_file.h>
#include <rfb/LogWriter.h>
#include <rfb/Exception.h>
#include <rfb_win32/RegConfig.h>
#include <rfb_win32/MsgBox.h>

#ifdef _DIALOG_CAPTURE
#include <extra/LoadBMP.h>
#endif

using namespace rfb;
using namespace rfb::win32;
using namespace rdr;
using namespace network;

static LogWriter vlog("main");

TStr rfb::win32::AppName("VNC Viewer");


#ifdef _DIALOG_CAPTURE
BoolParameter captureDialogs("CaptureDialogs", "", false);
#endif

//
// -=- Listener
//     Class to handle listening on a particular port for incoming connections
//     from servers, and spawning of clients
//

static BoolParameter acceptIncoming("Listen", "Accept incoming connections from VNC servers.", false);


//
// -=- AboutDialog global values
//

const WORD rfb::win32::AboutDialog::DialogId = IDD_ABOUT;
const WORD rfb::win32::AboutDialog::Copyright = IDC_COPYRIGHT;
const WORD rfb::win32::AboutDialog::Version = IDC_VERSION;
const WORD rfb::win32::AboutDialog::BuildTime = IDC_BUILDTIME;
const WORD rfb::win32::AboutDialog::Description = IDC_DESCRIPTION;


//
// -=- processParams
//     Read in the command-line parameters and interpret them.
//

void
programInfo() {
  win32::FileVersionInfo inf;
  _tprintf(_T("%s - %s, Version %s\n"),
    inf.getVerString(_T("ProductName")),
    inf.getVerString(_T("FileDescription")),
    inf.getVerString(_T("FileVersion")));
  printf("%s\n", buildTime);
  _tprintf(_T("%s\n\n"), inf.getVerString(_T("LegalCopyright")));
}

void
programUsage() {
  printf("usage: vncviewer <options> <hostname>[:<display>]\n");
  printf("Command-line options:\n");
  printf("  -help                                - Provide usage information.\n");
  printf("  -config <file>                       - Load connection settings from VNC Viewer 3.3 settings file\n");
  printf("  -console                             - Run with a console window visible.\n");
  printf("  <setting>=<value>                    - Set the named configuration parameter.\n");
  printf("    (Parameter values specified on the command-line override those specified by other configuration methods.)\n");
  printf("\nLog names:\n");
  LogWriter::listLogWriters();
  printf("\nLog destinations:\n");
  Logger::listLoggers();
  printf("\nParameters:\n");
  Configuration::listParams();
  printf("Press Enter/Return key to continue\n");
  char c = getchar();
  exit(1);
}


bool print_usage = false;
bool close_console = true;
std::list<char*> hosts;
std::list<char*> configFiles;

void
processParams(int argc, char* argv[]) {
  for (int i=1; i<argc; i++) {
    try {

      if (strcasecmp(argv[i], "-console") == 0) {
        close_console = false;

      } else if (((strcasecmp(argv[i], "-config") == 0) ||
                  (strcasecmp(argv[i], "/config") == 0)) && (i < argc-1)) {
        configFiles.push_back(strDup(argv[i+1]));
        i++;

      } else if ((strcasecmp(argv[i], "-help") == 0) ||
                 (strcasecmp(argv[i], "--help") == 0) ||
                 (strcasecmp(argv[i], "-h") == 0) ||
                 (strcasecmp(argv[i], "/?") == 0)) {
        print_usage = true;
        close_console = false;
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
        // If it's -<option> then it's not recognised - error
        // If it's <host> then add it to the list to connect to.
        if ((argv[i][0] == '-') || (argv[i][0] == '/')) {
          const char* fmt = "The option %s was not recognized.  Use -help to see VNC Viewer usage";
          CharArray tmp(strlen(argv[i])+strlen(fmt)+1);
          sprintf(tmp.buf, fmt, argv[i]);
          MsgBox(0, TStr(tmp.buf), MB_ICONSTOP | MB_OK);
          exit(1);
        } else if (strContains(argv[i], '\\')) {
          configFiles.push_back(strDup(argv[i]));
        } else {
          hosts.push_back(strDup(argv[i]));
        }
      }

    } catch (rdr::Exception& e) {
      vlog.error(e.str());
    }
  }
}


//
// -=- main
//

//int WINAPI WinMain(HINSTANCE inst, HINSTANCE prevInst, char* cmdLine, int cmdShow) {
int vncClientMain() {
  //try {

    // - Initialise the available loggers
    initStdIOLoggers();
    //initFileLogger("C:\\temp\\vncviewer4.log");

    // - By default, just log errors to stderr
    logParams.setDefault("*:stderr:0");

    // - Process the command-line
    /*int argc = __argc;
    char** argv = __argv;
    processParams(argc, argv);*/

    // - By default the console will be closed
    /*if (close_console) {
      if (!FreeConsole())
        vlog.info("unable to close console:%u", GetLastError());
    } else {
      AllocConsole();
      freopen("CONIN$","rb",stdin);
      freopen("CONOUT$","wb",stdout);
      freopen("CONOUT$","wb",stderr);
      setbuf(stderr, 0);
    }*/

#ifdef _DIALOG_CAPTURE
    if (captureDialogs) {
      CConn::userConfigKey.openKey(HKEY_CURRENT_USER, _T("Software\\RealVNC\\VNCViewer4"));
      OptionsDialog::global.showDialog(0, true);
      return 0;
    }
#endif

    // - If no clients are specified, bring up a connection dialog
    if (configFiles.empty() && hosts.empty() && !acceptIncoming && !print_usage)
      hosts.push_back(0);

    //programInfo();

    // - Connect to the clients
    if (!configFiles.empty() || !hosts.empty() || acceptIncoming) {
      // - Configure the registry configuration reader
      win32::RegConfigThread config;
      config.start(HKEY_CURRENT_USER, _T("Software\\RealVNC\\VNCViewer4"));

      // - Tell the rest of VNC Viewer where to write config data to
      CConn::userConfigKey.createKey(HKEY_CURRENT_USER, _T("Software\\RealVNC\\VNCViewer4"));

      if (acceptIncoming) {
        int port = 5500;

        // Listening viewer
        if (hosts.size() > 1)
          programUsage();
        if (!hosts.empty())
          port = atoi(hosts.front());  

        // Show the tray icon & menu
        ListenTrayIcon tray;

        // Listen for reverse connections
        network::TcpListener sock(port);
        ListenServer listener(&sock);

        // Run the view manager
        // Also processes the tray icon if necessary
        MSG msg;
        while (GetMessage(&msg, NULL, 0, 0) > 0) {
          TranslateMessage(&msg);
          DispatchMessage(&msg);
        }
      } else {
        // Read each config file in turn
        while (!configFiles.empty()) {
          char* filename = configFiles.front();
          Thread* connThread = new CConnThread(filename, true);
          strFree(filename);
          configFiles.pop_front();
        }

        // Connect to each client in turn
        while (!hosts.empty()) {
          char* hostinfo = hosts.front();
          Thread* connThread = new CConnThread(hostinfo);
          strFree(hostinfo);
          hosts.pop_front();
        }

        // Run the view manager
        MSG msg;
        while (CConnThread::getMessage(&msg, NULL, 0, 0) > 0) {
          TranslateMessage(&msg);
          DispatchMessage(&msg);
        }

        vlog.debug("quitting viewer");
      }

    }

    // - If necessary, print the program's usage info
    if (print_usage)
      programUsage();

    if (!close_console) {
      printf("Press Enter/Return key to continue\n");
      char c = getchar();
    }

  //} catch (rdr::Exception& e) {
    //MsgBox(0, TStr(e.str()), MB_ICONSTOP | MB_OK);
  //}

  return 0;
}
