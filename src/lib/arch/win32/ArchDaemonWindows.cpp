/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "arch/win32/ArchDaemonWindows.h"

#include "arch/Arch.h"
#include "arch/ArchException.h"
#include "arch/win32/ArchMiscWindows.h"
#include "arch/win32/XArchWindows.h"
#include "base/Log.h"
#include "common/Constants.h"

//
// ArchDaemonWindows
//

ArchDaemonWindows *ArchDaemonWindows::s_daemon = nullptr;

ArchDaemonWindows::ArchDaemonWindows() : m_daemonThreadID(0)
{
  m_quitMessage = RegisterWindowMessage(L"DeskflowDaemonExit");
}

int ArchDaemonWindows::runDaemon(RunFunc runFunc)
{
  assert(s_daemon != nullptr);
  return s_daemon->doRunDaemon(runFunc);
}

void ArchDaemonWindows::daemonRunning(bool running)
{
  if (s_daemon != nullptr) {
    s_daemon->doDaemonRunning(running);
  }
}

UINT ArchDaemonWindows::getDaemonQuitMessage()
{
  if (s_daemon != nullptr) {
    return s_daemon->doGetDaemonQuitMessage();
  } else {
    return 0;
  }
}

void ArchDaemonWindows::daemonFailed(int result)
{
  assert(s_daemon != nullptr);
  throw ArchDaemonRunException(result);
}

int ArchDaemonWindows::daemonize(DaemonFunc const &func)
{
  assert(func != nullptr);

  // save daemon function
  m_daemonFunc = func;

  // construct the service entry
  SERVICE_TABLE_ENTRY entry[2];
  entry[0].lpServiceName = const_cast<wchar_t *>(QString(kAppName).toStdWString().c_str());
  entry[0].lpServiceProc = &ArchDaemonWindows::serviceMainEntry;
  entry[1].lpServiceName = nullptr;
  entry[1].lpServiceProc = nullptr;

  // hook us up to the service control manager.  this won't return
  // (if successful) until the processes have terminated.
  s_daemon = this;
  if (StartServiceCtrlDispatcher(entry) == 0) {
    // StartServiceCtrlDispatcher failed
    s_daemon = nullptr;
    throw ArchDaemonFailedException(windowsErrorToString(GetLastError()));
  }

  s_daemon = nullptr;
  return m_daemonResult;
}

HKEY ArchDaemonWindows::openNTServicesKey()
{
  static const wchar_t *s_keyNames[] = {_T("SYSTEM"), _T("CurrentControlSet"), _T("Services"), nullptr};

  return ArchMiscWindows::addKey(HKEY_LOCAL_MACHINE, s_keyNames);
}

bool ArchDaemonWindows::isRunState(DWORD state)
{
  switch (state) {
  case SERVICE_START_PENDING:
  case SERVICE_CONTINUE_PENDING:
  case SERVICE_RUNNING:
    return true;

  default:
    return false;
  }
}

int ArchDaemonWindows::doRunDaemon(RunFunc run)
{
  // should only be called from DaemonFunc
  assert(m_serviceMutex != nullptr);
  assert(run != nullptr);

  // create message queue for this thread
  MSG dummy;
  PeekMessage(&dummy, nullptr, 0, 0, PM_NOREMOVE);

  int result = 0;
  ARCH->lockMutex(m_serviceMutex);
  m_daemonThreadID = GetCurrentThreadId();
  while (m_serviceState != SERVICE_STOPPED) {
    // wait until we're told to start
    while (!isRunState(m_serviceState) && m_serviceState != SERVICE_STOP_PENDING) {
      ARCH->waitCondVar(m_serviceCondVar, m_serviceMutex, -1.0);
    }

    // run unless told to stop
    if (m_serviceState != SERVICE_STOP_PENDING) {
      ARCH->unlockMutex(m_serviceMutex);
      try {
        result = run();
      } catch (...) {
        ARCH->lockMutex(m_serviceMutex);
        setStatusError(0);
        m_serviceState = SERVICE_STOPPED;
        setStatus(m_serviceState);
        ARCH->broadcastCondVar(m_serviceCondVar);
        ARCH->unlockMutex(m_serviceMutex);
        throw;
      }
      ARCH->lockMutex(m_serviceMutex);
    }

    // notify of new state
    if (m_serviceState == SERVICE_PAUSE_PENDING) {
      m_serviceState = SERVICE_PAUSED;
    } else {
      m_serviceState = SERVICE_STOPPED;
    }
    setStatus(m_serviceState);
    ARCH->broadcastCondVar(m_serviceCondVar);
  }
  ARCH->unlockMutex(m_serviceMutex);
  return result;
}

void ArchDaemonWindows::doDaemonRunning(bool running)
{
  ARCH->lockMutex(m_serviceMutex);
  if (running) {
    m_serviceState = SERVICE_RUNNING;
    setStatus(m_serviceState);
    ARCH->broadcastCondVar(m_serviceCondVar);
  }
  ARCH->unlockMutex(m_serviceMutex);
}

UINT ArchDaemonWindows::doGetDaemonQuitMessage()
{
  return m_quitMessage;
}

void ArchDaemonWindows::setStatus(DWORD state)
{
  setStatus(state, 0, 0);
}

void ArchDaemonWindows::setStatus(DWORD state, DWORD step, DWORD waitHint)
{
  assert(s_daemon != nullptr);

  LOG_DEBUG("setting service status: state=%d, step=%d, waitHint=%d", state, step, waitHint);

  SERVICE_STATUS status;
  status.dwServiceType = SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS;
  status.dwCurrentState = state;
  status.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_PAUSE_CONTINUE | SERVICE_ACCEPT_SHUTDOWN;
  status.dwWin32ExitCode = NO_ERROR;
  status.dwServiceSpecificExitCode = 0;
  status.dwCheckPoint = step;
  status.dwWaitHint = waitHint;
  SetServiceStatus(s_daemon->m_statusHandle, &status);
}

void ArchDaemonWindows::setStatusError(DWORD error)
{
  assert(s_daemon != nullptr);

  SERVICE_STATUS status;
  status.dwServiceType = SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS;
  status.dwCurrentState = SERVICE_STOPPED;
  status.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_PAUSE_CONTINUE | SERVICE_ACCEPT_SHUTDOWN;
  status.dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;
  status.dwServiceSpecificExitCode = error;
  status.dwCheckPoint = 0;
  status.dwWaitHint = 0;
  SetServiceStatus(s_daemon->m_statusHandle, &status);
}

void ArchDaemonWindows::serviceMain(DWORD argc, LPTSTR *argvIn)
{
  using ArgList = std::vector<LPWSTR>;
  using Arguments = std::vector<std::wstring>;
  const wchar_t **argv = const_cast<const wchar_t **>(argvIn);

  // create synchronization objects
  m_serviceMutex = ARCH->newMutex();
  m_serviceCondVar = ARCH->newCondVar();

  // register our service handler function
  m_statusHandle = RegisterServiceCtrlHandler(argv[0], &ArchDaemonWindows::serviceHandlerEntry);
  if (m_statusHandle == 0) {
    // cannot start as service
    m_daemonResult = -1;
    ARCH->closeCondVar(m_serviceCondVar);
    ARCH->closeMutex(m_serviceMutex);
    return;
  }

  // tell service control manager that we're starting
  m_serviceState = SERVICE_START_PENDING;
  setStatus(m_serviceState, 0, 10000);

  std::wstring commandLine;

  // if no arguments supplied then try getting them from the registry.
  // the first argument doesn't count because it's the service name.
  Arguments args;
  ArgList myArgv;
  if (argc <= 1) {
    // read command line
    HKEY key = openNTServicesKey();
    key = ArchMiscWindows::openKey(key, argvIn[0]);
    key = ArchMiscWindows::openKey(key, _T("Parameters"));
    if (key != nullptr) {
      commandLine = ArchMiscWindows::readValueString(key, _T("CommandLine"));
    }

    // if the command line isn't empty then parse and use it
    if (!commandLine.empty()) {
      // parse, honoring double quoted substrings
      std::wstring::size_type i = commandLine.find_first_not_of(_T(" \t"));
      while (i != std::string::npos && i != commandLine.size()) {
        // find end of string
        std::wstring::size_type e;
        if (commandLine[i] == '\"') {
          // quoted.  find closing quote.
          ++i;
          e = commandLine.find(_T("\""), i);

          // whitespace must follow closing quote
          if (e == std::wstring::npos ||
              (e + 1 != commandLine.size() && commandLine[e + 1] != ' ' && commandLine[e + 1] != '\t')) {
            args.clear();
            break;
          }

          // extract
          args.push_back(commandLine.substr(i, e - i));
          i = e + 1;
        } else {
          // unquoted.  find next whitespace.
          e = commandLine.find_first_of(_T(" \t"), i);
          if (e == std::wstring::npos) {
            e = commandLine.size();
          }

          // extract
          args.push_back(commandLine.substr(i, e - i));
          i = e + 1;
        }

        // next argument
        i = commandLine.find_first_not_of(_T(" \t"), i);
      }

      // service name goes first
      myArgv.push_back(LPWSTR(argv[0]));

      // get pointers
      for (size_t j = 0; j < args.size(); ++j) {
        myArgv.push_back(LPWSTR(args[j].c_str()));
      }

      // adjust argc/argv
      argc = (DWORD)myArgv.size();
      argv = const_cast<const wchar_t **>(&myArgv[0]);
    }
  }

  m_commandLine = QString::fromStdWString(commandLine);

  try {
    // invoke daemon function
    m_daemonResult = m_daemonFunc();
  } catch (ArchDaemonRunException &e) {
    setStatusError(e.m_result);
    m_daemonResult = -1;
  } catch (...) {
    setStatusError(1);
    m_daemonResult = -1;
  }

  // clean up
  ARCH->closeCondVar(m_serviceCondVar);
  ARCH->closeMutex(m_serviceMutex);

  // we're going to exit now, so set status to stopped
  m_serviceState = SERVICE_STOPPED;
  setStatus(m_serviceState, 0, 10000);
}

void WINAPI ArchDaemonWindows::serviceMainEntry(DWORD argc, LPTSTR *argv)
{
  s_daemon->serviceMain(argc, argv);
}

void ArchDaemonWindows::serviceHandler(DWORD ctrl)
{
  assert(m_serviceMutex != nullptr);
  assert(m_serviceCondVar != nullptr);

  ARCH->lockMutex(m_serviceMutex);

  // ignore request if service is already stopped
  if (s_daemon == nullptr || m_serviceState == SERVICE_STOPPED) {
    if (s_daemon != nullptr) {
      setStatus(m_serviceState);
    }
    ARCH->unlockMutex(m_serviceMutex);
    return;
  }

  switch (ctrl) {
  case SERVICE_CONTROL_PAUSE:
    m_serviceState = SERVICE_PAUSE_PENDING;
    setStatus(m_serviceState, 0, 5000);
    PostThreadMessage(m_daemonThreadID, m_quitMessage, 0, 0);
    while (isRunState(m_serviceState)) {
      ARCH->waitCondVar(m_serviceCondVar, m_serviceMutex, -1.0);
    }
    break;

  case SERVICE_CONTROL_CONTINUE:
    // FIXME -- maybe should flush quit messages from queue
    m_serviceState = SERVICE_CONTINUE_PENDING;
    setStatus(m_serviceState, 0, 5000);
    ARCH->broadcastCondVar(m_serviceCondVar);
    break;

  case SERVICE_CONTROL_STOP:
  case SERVICE_CONTROL_SHUTDOWN:
    m_serviceState = SERVICE_STOP_PENDING;
    setStatus(m_serviceState, 0, 5000);
    PostThreadMessage(m_daemonThreadID, m_quitMessage, 0, 0);
    ARCH->broadcastCondVar(m_serviceCondVar);
    while (isRunState(m_serviceState)) {
      ARCH->waitCondVar(m_serviceCondVar, m_serviceMutex, -1.0);
    }
    break;

  default:
    // unknown service command
    // fall through

  case SERVICE_CONTROL_INTERROGATE:
    setStatus(m_serviceState);
    break;
  }

  ARCH->unlockMutex(m_serviceMutex);
}

void WINAPI ArchDaemonWindows::serviceHandlerEntry(DWORD ctrl)
{
  s_daemon->serviceHandler(ctrl);
}

void ArchDaemonWindows::start(const QString &name)
{
  // open service manager
  SC_HANDLE mgr = OpenSCManager(nullptr, nullptr, GENERIC_READ);
  if (mgr == nullptr) {
    throw ArchDaemonFailedException(windowsErrorToString(GetLastError()));
  }

  // open the service
  SC_HANDLE service = OpenService(mgr, name.toStdWString().c_str(), SERVICE_START);

  if (service == nullptr) {
    CloseServiceHandle(mgr);
    throw ArchDaemonFailedException(windowsErrorToString(GetLastError()));
  }

  // start the service
  if (!StartService(service, 0, nullptr)) {
    throw ArchDaemonFailedException(windowsErrorToString(GetLastError()));
  }
}

void ArchDaemonWindows::stop(const QString &name)
{
  // open service manager
  SC_HANDLE mgr = OpenSCManager(nullptr, nullptr, GENERIC_READ);
  if (mgr == nullptr) {
    throw ArchDaemonFailedException(windowsErrorToString(GetLastError()));
  }

  // open the service
  SC_HANDLE service = OpenService(mgr, name.toStdWString().c_str(), SERVICE_STOP | SERVICE_QUERY_STATUS);

  if (service == nullptr) {
    CloseServiceHandle(mgr);
    throw ArchDaemonFailedException(windowsErrorToString(GetLastError()));
  }

  // ask the service to stop, asynchronously
  SERVICE_STATUS ss;
  if (!ControlService(service, SERVICE_CONTROL_STOP, &ss)) {
    DWORD dwErrCode = GetLastError();
    if (dwErrCode != ERROR_SERVICE_NOT_ACTIVE) {
      throw ArchDaemonFailedException(windowsErrorToString(GetLastError()));
    }
  }
}
