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

// -=- Service.cxx

#include <rfb_win32/Service.h>
#include <rfb_win32/MsgWindow.h>
#include <rfb_win32/DynamicFn.h>
#include <rfb_win32/ModuleFileName.h>
#include <rfb_win32/Registry.h>
#include <rfb_win32/OSVersion.h>
#include <rfb/Threading.h>
#include <logmessages/messages.h>
#include <rdr/Exception.h>
#include <rfb/LogWriter.h>


using namespace rdr;
using namespace rfb;
using namespace win32;

static LogWriter vlog("Service");


// - Internal service implementation functions

Service* service = 0;

VOID WINAPI serviceHandler(DWORD control) {
  switch (control) {
  case SERVICE_CONTROL_INTERROGATE:
    vlog.info("cmd: report status");
    service->setStatus();
    return;
  case SERVICE_CONTROL_PARAMCHANGE:
    vlog.info("cmd: param change");
    service->readParams();
    return;
  case SERVICE_CONTROL_SHUTDOWN:
    vlog.info("cmd: OS shutdown");
    service->osShuttingDown();
    return;
  case SERVICE_CONTROL_STOP:
    vlog.info("cmd: stop");
    service->setStatus(SERVICE_STOP_PENDING);
    service->stop();
    return;
  };
  vlog.debug("cmd: unknown %lu", control);
}


// -=- Message window derived class used under Win9x to implement stopService

#define WM_SMSG_SERVICE_STOP WM_USER

class ServiceMsgWindow : public MsgWindow {
public:
  ServiceMsgWindow(const TCHAR* name) : MsgWindow(name) {}
  LRESULT processMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_SMSG_SERVICE_STOP:
      service->stop();
      return TRUE;
    }
    return MsgWindow::processMessage(msg, wParam, lParam);
  }

  static const TCHAR* baseName;
};

const TCHAR* ServiceMsgWindow::baseName = _T("ServiceWindow:");


// -=- Service main procedure, used under WinNT/2K/XP by the SCM

VOID WINAPI serviceProc(DWORD dwArgc, LPTSTR* lpszArgv) {
  vlog.debug("entering %s serviceProc", service->getName());
  vlog.info("registering handler...");
  service->status_handle = RegisterServiceCtrlHandler(service->getName(), serviceHandler);
  if (!service->status_handle) {
    DWORD err = GetLastError();
    vlog.error("failed to register handler: %lu", err);
    ExitProcess(err);
  }
  vlog.debug("registered handler (%lx)", service->status_handle);
  service->setStatus(SERVICE_START_PENDING);
  vlog.debug("entering %s serviceMain", service->getName());
  service->status.dwWin32ExitCode = service->serviceMain(dwArgc, lpszArgv);
  vlog.debug("leaving %s serviceMain", service->getName());
  service->setStatus(SERVICE_STOPPED);
}


// -=- Service

Service::Service(const TCHAR* name_) : name(name_) {
  vlog.debug("Service");
  status_handle = 0;
  status.dwControlsAccepted = SERVICE_CONTROL_INTERROGATE | SERVICE_ACCEPT_SHUTDOWN | SERVICE_ACCEPT_STOP;
  status.dwServiceType = SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS;
  status.dwWin32ExitCode = NO_ERROR;
  status.dwServiceSpecificExitCode = 0;
  status.dwCheckPoint = 0;
  status.dwWaitHint = 30000;
  status.dwCurrentState = SERVICE_STOPPED;
}

void
Service::start() {
  if (osVersion.isPlatformNT) {
    SERVICE_TABLE_ENTRY entry[2];
    entry[0].lpServiceName = (TCHAR*)name;
    entry[0].lpServiceProc = serviceProc;
    entry[1].lpServiceName = NULL;
    entry[1].lpServiceProc = NULL;
    vlog.debug("entering dispatcher");
    if (!SetProcessShutdownParameters(0x100, 0))
      vlog.error("unable to set shutdown parameters: %d", GetLastError());
    service = this;
    if (!StartServiceCtrlDispatcher(entry))
      throw SystemException("unable to start service", GetLastError());
  } else {

    // - Create the service window, so the service can be stopped
    TCharArray wndName(_tcslen(getName()) + _tcslen(ServiceMsgWindow::baseName) + 1);
    _tcscpy(wndName.buf, ServiceMsgWindow::baseName);
    _tcscat(wndName.buf, getName());
    ServiceMsgWindow service_window(wndName.buf);

    // - Locate the RegisterServiceProcess function
	  typedef DWORD (WINAPI * _RegisterServiceProcess_proto)(DWORD, DWORD);
    DynamicFn<_RegisterServiceProcess_proto> _RegisterServiceProcess(_T("kernel32.dll"), "RegisterServiceProcess");
    if (!_RegisterServiceProcess.isValid())
      throw Exception("unable to find RegisterServiceProcess");

    // - Run the service
    (*_RegisterServiceProcess)(NULL, 1);
    service = this;
    serviceMain(0, 0);
	  (*_RegisterServiceProcess)(NULL, 0);
  }
}

void
Service::setStatus() {
  setStatus(status.dwCurrentState);
}

void
Service::setStatus(DWORD state) {
  if (!osVersion.isPlatformNT)
    return;
  if (status_handle == 0) {
    vlog.debug("warning - cannot setStatus");
    return;
  }
  status.dwCurrentState = state;
  status.dwCheckPoint++;
  if (!SetServiceStatus(status_handle, &status)) {
    status.dwCurrentState = SERVICE_STOPPED;
    status.dwWin32ExitCode = GetLastError();
    vlog.error("unable to set service status:%u", status.dwWin32ExitCode);
  }
  vlog.debug("set status to %u(%u)", state, status.dwCheckPoint);
}

Service::~Service() {
  vlog.debug("~Service");
  service = 0;
}


// Find out whether this process is running as the WinVNC service
bool thisIsService() {
  return service && (service->status.dwCurrentState != SERVICE_STOPPED);
}


// -=- Desktop handling code

// Switch the current thread to the specified desktop
static bool
switchToDesktop(HDESK desktop) {
  HDESK old_desktop = GetThreadDesktop(GetCurrentThreadId());
  if (!SetThreadDesktop(desktop)) {
    vlog.debug("switchToDesktop failed:%u", GetLastError());
    return false;
  }
  if (!CloseDesktop(old_desktop))
    vlog.debug("unable to close old desktop:%u", GetLastError());
  return true;
}

// Determine whether the thread's current desktop is the input one
static bool
inputDesktopSelected() {
  HDESK current = GetThreadDesktop(GetCurrentThreadId());
	HDESK input = OpenInputDesktop(0, FALSE,
  	DESKTOP_CREATEMENU | DESKTOP_CREATEWINDOW |
		DESKTOP_ENUMERATE | DESKTOP_HOOKCONTROL |
		DESKTOP_WRITEOBJECTS | DESKTOP_READOBJECTS |
		DESKTOP_SWITCHDESKTOP | GENERIC_WRITE);
  if (!input) {
    vlog.debug("unable to OpenInputDesktop(1):%u", GetLastError());
    return false;
  }

  DWORD size;
  char currentname[256];
  char inputname[256];

  if (!GetUserObjectInformation(current, UOI_NAME, currentname, 256, &size)) {
    vlog.debug("unable to GetUserObjectInformation(1):%u", GetLastError());
    CloseDesktop(input);
    return false;
  }
  if (!GetUserObjectInformation(input, UOI_NAME, inputname, 256, &size)) {
    vlog.debug("unable to GetUserObjectInformation(2):%u", GetLastError());
    CloseDesktop(input);
    return false;
  }
  if (!CloseDesktop(input))
    vlog.debug("unable to close input desktop:%u", GetLastError());

  // *** vlog.debug("current=%s, input=%s", currentname, inputname);
  bool result = strcmp(currentname, inputname) == 0;
  return result;
}

// Switch the current thread into the input desktop
static bool
selectInputDesktop() {
  // - Open the input desktop
  HDESK desktop = OpenInputDesktop(0, FALSE,
		DESKTOP_CREATEMENU | DESKTOP_CREATEWINDOW |
		DESKTOP_ENUMERATE | DESKTOP_HOOKCONTROL |
		DESKTOP_WRITEOBJECTS | DESKTOP_READOBJECTS |
		DESKTOP_SWITCHDESKTOP | GENERIC_WRITE);
  if (!desktop) {
    vlog.debug("unable to OpenInputDesktop(2):%u", GetLastError());
    return false;
  }

  // - Switch into it
  if (!switchToDesktop(desktop)) {
    CloseDesktop(desktop);
    return false;
  }

  // ***
  DWORD size = 256;
  char currentname[256];
  if (GetUserObjectInformation(desktop, UOI_NAME, currentname, 256, &size)) {
    vlog.debug("switched to %s", currentname);
  }
  // ***

  vlog.debug("switched to input desktop");

  return true;
}


// -=- Access points to desktop-switching routines

bool
rfb::win32::desktopChangeRequired() {
  if (!osVersion.isPlatformNT)
    return false;

  return !inputDesktopSelected();
}

bool
rfb::win32::changeDesktop() {
  if (!osVersion.isPlatformNT)
    return true;
  if (osVersion.cannotSwitchDesktop)
    return false;

  return selectInputDesktop();
}


// -=- Ctrl-Alt-Del emulation

class CADThread : public Thread {
public:
  CADThread() : Thread("CtrlAltDel Emulator"), result(false) {}
  virtual void run() {
	  HDESK old_desktop = GetThreadDesktop(GetCurrentThreadId());

    if (switchToDesktop(OpenDesktop(_T("Winlogon"), 0, FALSE, DESKTOP_CREATEMENU | DESKTOP_CREATEWINDOW |
		  DESKTOP_ENUMERATE | DESKTOP_HOOKCONTROL |
		  DESKTOP_WRITEOBJECTS | DESKTOP_READOBJECTS |
      DESKTOP_SWITCHDESKTOP | GENERIC_WRITE))) {
	    PostMessage(HWND_BROADCAST, WM_HOTKEY, 0, MAKELONG(MOD_ALT | MOD_CONTROL, VK_DELETE));
      switchToDesktop(old_desktop);
      result = true;
    }
  }
  bool result;
};

bool
rfb::win32::emulateCtrlAltDel() {
  if (!osVersion.isPlatformNT)
    return false;

  CADThread* cad_thread = new CADThread();
  vlog.debug("emulate Ctrl-Alt-Del");
  if (cad_thread) {
    cad_thread->start();
    cad_thread->join();
    bool result = cad_thread->result;
    delete cad_thread;
    return result;
  }
  return false;
}


// -=- Application Event Log target Logger class

class Logger_EventLog : public Logger {
public:
  Logger_EventLog(const TCHAR* srcname) : Logger("EventLog") {
    eventlog = RegisterEventSource(NULL, srcname);
    if (!eventlog)
      printf("Unable to open event log:%ld\n", GetLastError());
  }
  ~Logger_EventLog() {
    if (eventlog)
      DeregisterEventSource(eventlog);
  }

  virtual void write(int level, const char *logname, const char *message) {
    if (!eventlog) return;
    TStr log(logname), msg(message);
    const TCHAR* strings[] = {log, msg};
    WORD type = EVENTLOG_INFORMATION_TYPE;
    if (level == 0) type = EVENTLOG_ERROR_TYPE;
    if (!ReportEvent(eventlog, type, 0, VNC4LogMessage, NULL, 2, 0, strings, NULL)) {
      // *** It's not at all clear what is the correct behaviour if this fails...
      printf("ReportEvent failed:%ld\n", GetLastError());
    }
  }

protected:
  HANDLE eventlog;
};

static Logger_EventLog* logger = 0;

bool rfb::win32::initEventLogLogger(const TCHAR* srcname) {
  if (logger)
    return false;
  if (osVersion.isPlatformNT) {
    logger = new Logger_EventLog(srcname);
    logger->registerLogger();
    return true;
  } else {
    return false;
  }
}


// -=- Registering and unregistering the service

bool rfb::win32::registerService(const TCHAR* name, const TCHAR* desc,
                                 int argc, const char* argv[]) {

  // - Initialise the default service parameters
  const TCHAR* defaultcmdline;
  if (osVersion.isPlatformNT)
    defaultcmdline = _T("-service");
  else
    defaultcmdline = _T("-noconsole -service");

  // - Get the full pathname of our executable
  ModuleFileName buffer;

  // - Calculate the command-line length
  int cmdline_len = _tcslen(buffer.buf) + 4;
  int i;
  for (i=0; i<argc; i++) {
    cmdline_len += strlen(argv[i]) + 3;
  }

  // - Add the supplied extra parameters to the command line
  TCharArray cmdline(cmdline_len+_tcslen(defaultcmdline));
  _stprintf(cmdline.buf, _T("\"%s\" %s"), buffer.buf, defaultcmdline);
  for (i=0; i<argc; i++) {
    _tcscat(cmdline.buf, _T(" \""));
    _tcscat(cmdline.buf, TStr(argv[i]));
    _tcscat(cmdline.buf, _T("\""));
  }
    
  // - Register the service

  if (osVersion.isPlatformNT) {

    // - Open the SCM
    ServiceHandle scm = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
    if (!scm)
      throw rdr::SystemException("unable to open Service Control Manager", GetLastError());


    ServiceHandle service = CreateService(scm,
      name, desc, SC_MANAGER_ALL_ACCESS,
      SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS,
      SERVICE_AUTO_START, SERVICE_ERROR_IGNORE,
      cmdline.buf, NULL, NULL, NULL, NULL, NULL);
    if (!service)
      throw rdr::SystemException("unable to create service", GetLastError());

    // - Register the event log source
    RegKey hk, hk2;

    hk2.createKey(HKEY_LOCAL_MACHINE, _T("SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application"));
    hk.createKey(hk2, name);

    for (i=_tcslen(buffer.buf); i>0; i--) {
      if (buffer.buf[i] == _T('\\')) {
        buffer.buf[i+1] = 0;
        break;
      }
    }

    const TCHAR* dllFilename = _T("logmessages.dll");
    TCharArray dllPath(_tcslen(buffer.buf) + _tcslen(dllFilename) + 1);
    _tcscpy(dllPath.buf, buffer.buf);
    _tcscat(dllPath.buf, dllFilename);
 
    hk.setExpandString(_T("EventMessageFile"), dllPath.buf);
    hk.setInt(_T("TypesSupported"), EVENTLOG_ERROR_TYPE | EVENTLOG_INFORMATION_TYPE);

  } else {

    RegKey services;
    services.createKey(HKEY_LOCAL_MACHINE, _T("Software\\Microsoft\\Windows\\CurrentVersion\\RunServices"));
    services.setString(name, cmdline.buf);

  }

  Sleep(500);

  return true;
}

bool rfb::win32::unregisterService(const TCHAR* name) {
  if (osVersion.isPlatformNT) {

    // - Open the SCM
    ServiceHandle scm = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
    if (!scm)
      throw rdr::SystemException("unable to open Service Control Manager", GetLastError());

    // - Create the service
    ServiceHandle service = OpenService(scm, name, SC_MANAGER_ALL_ACCESS);
    if (!service)
      throw rdr::SystemException("unable to locate the service", GetLastError());
    if (!DeleteService(service))
      throw rdr::SystemException("unable to remove the service", GetLastError());

    // - Register the event log source
    RegKey hk;
    hk.openKey(HKEY_LOCAL_MACHINE, _T("SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application"));
    hk.deleteKey(name);

  } else {

		RegKey services;
    services.openKey(HKEY_LOCAL_MACHINE, _T("Software\\Microsoft\\Windows\\CurrentVersion\\RunServices"));
    services.deleteValue(name);

  }

  Sleep(500);

  return true;
}


// -=- Starting and stopping the service

HWND findServiceWindow(const TCHAR* name) {
  TCharArray wndName(_tcslen(ServiceMsgWindow::baseName)+_tcslen(name)+1);
  _tcscpy(wndName.buf, ServiceMsgWindow::baseName);
  _tcscat(wndName.buf, name);
  vlog.debug("searching for %s window", CStr(wndName.buf));
  return FindWindow(0, wndName.buf);
}

bool rfb::win32::startService(const TCHAR* name) {

  if (osVersion.isPlatformNT) {
    // - Open the SCM
    ServiceHandle scm = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (!scm)
      throw rdr::SystemException("unable to open Service Control Manager", GetLastError());

    // - Locate the service
    ServiceHandle service = OpenService(scm, name, SERVICE_START);
    if (!service)
      throw rdr::SystemException("unable to open the service", GetLastError());

    // - Start the service
    if (!StartService(service, 0, NULL))
      throw rdr::SystemException("unable to start the service", GetLastError());
  } else {
    // - Check there is no service window
    if (findServiceWindow(name))
      throw rdr::Exception("the service is already running");

    // - Find the RunServices registry key
		RegKey services;
		services.openKey(HKEY_LOCAL_MACHINE, _T("Software\\Microsoft\\Windows\\CurrentVersion\\RunServices"));

    // - Read the command-line from it
    TCharArray cmdLine = services.getString(name);

    // - Start the service
    PROCESS_INFORMATION proc_info;
    STARTUPINFO startup_info;
    ZeroMemory(&startup_info, sizeof(startup_info));
    startup_info.cb = sizeof(startup_info);
    if (!CreateProcess(0, cmdLine.buf, 0, 0, FALSE, CREATE_NEW_CONSOLE, 0, 0, &startup_info, &proc_info)) {
      throw SystemException("unable to start service", GetLastError());
    }
  }

  Sleep(500);

  return true;
}

bool rfb::win32::stopService(const TCHAR* name) {
  if (osVersion.isPlatformNT) {
    // - Open the SCM
    ServiceHandle scm = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (!scm)
      throw rdr::SystemException("unable to open Service Control Manager", GetLastError());

    // - Locate the service
    ServiceHandle service = OpenService(scm, name, SERVICE_STOP);
    if (!service)
      throw rdr::SystemException("unable to open the service", GetLastError());

    // - Start the service
    SERVICE_STATUS status;
    if (!ControlService(service, SERVICE_CONTROL_STOP, &status))
      throw rdr::SystemException("unable to stop the service", GetLastError());

  } else {
    // - Find the service window
    HWND service_window = findServiceWindow(name);
    if (!service_window)
      throw Exception("unable to locate running service");

    // Tell it to quit
    vlog.debug("sending service stop request");
    if (!SendMessage(service_window, WM_SMSG_SERVICE_STOP, 0, 0))
      throw Exception("unable to stop service");

    // Check it's quitting...
    DWORD process_id = 0;
    HANDLE process = 0;
    if (!GetWindowThreadProcessId(service_window, &process_id))
      throw SystemException("unable to verify service has quit", GetLastError());
    process = OpenProcess(SYNCHRONIZE | PROCESS_TERMINATE, FALSE, process_id);
    if (!process)
      throw SystemException("unable to obtain service handle", GetLastError());
    int retries = 5;
    vlog.debug("checking status");
    while (retries-- && (WaitForSingleObject(process, 1000) != WAIT_OBJECT_0)) {}
    if (!retries) {
      vlog.debug("failed to quit - terminating");
      // May not have quit because of silly Win9x registry watching bug..
      if (!TerminateProcess(process, 1))
        throw SystemException("unable to terminate process!", GetLastError());
      throw Exception("service failed to quit - called TerminateProcess");
    }
  }

  Sleep(500);

  return true;
}

DWORD rfb::win32::getServiceState(const TCHAR* name) {
  if (osVersion.isPlatformNT) {
    // - Open the SCM
    ServiceHandle scm = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (!scm)
      throw rdr::SystemException("unable to open Service Control Manager", GetLastError());

    // - Locate the service
    ServiceHandle service = OpenService(scm, name, SERVICE_INTERROGATE);
    if (!service)
      throw rdr::SystemException("unable to open the service", GetLastError());

    // - Get the service status
    SERVICE_STATUS status;
    if (!ControlService(service, SERVICE_CONTROL_INTERROGATE, (SERVICE_STATUS*)&status))
      throw rdr::SystemException("unable to query the service", GetLastError());

    return status.dwCurrentState;
  } else {
    HWND service_window = findServiceWindow(name);
    return service_window ? SERVICE_RUNNING : SERVICE_STOPPED;
  }
}

char* rfb::win32::serviceStateName(DWORD state) {
  switch (state) {
  case SERVICE_RUNNING: return strDup("Running");
  case SERVICE_STOPPED: return strDup("Stopped");
  case SERVICE_STOP_PENDING: return strDup("Stopping");
  };
  CharArray tmp(32);
  sprintf(tmp.buf, "Unknown (%lu)", state);
  return tmp.takeBuf();
}


bool rfb::win32::isServiceProcess() {
  return service != 0;
}