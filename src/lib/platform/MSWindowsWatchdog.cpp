/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2025 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/MSWindowsWatchdog.h"

#include "arch/Arch.h"
#include "arch/win32/XArchWindows.h"
#include "base/ELevel.h"
#include "base/Log.h"
#include "base/TMethodJob.h"
#include "base/log_outputters.h"
#include "deskflow/App.h"
#include "deskflow/ArgsBase.h"
#include "mt/Thread.h"

#include <Shellapi.h>
#include <UserEnv.h>
#include <sstream>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#define CURRENT_PROCESS_ID 0
#define MAXIMUM_WAIT_TIME 3

namespace {
std::string trimDesktopName(const std::string &nameFromTraces)
{
  std::string name;

  for (const auto &symbol : nameFromTraces) {
    if (std::isalpha(symbol)) {
      name.push_back(symbol);
    } else {
      break;
    }
  }

  return name;
}

} // namespace

enum
{
  kOutputBufferSize = 4096
};

typedef VOID(WINAPI *SendSas)(BOOL asUser);

const char g_activeDesktop[] = {"activeDesktop:"};

MSWindowsWatchdog::MSWindowsWatchdog(bool autoDetectCommand, bool foreground)
    : m_thread(NULL),
      m_autoDetectCommand(autoDetectCommand),
      m_monitoring(true),
      m_commandChanged(false),
      m_outputWritePipe(nullptr),
      m_outputReadPipe(nullptr),
      m_elevateProcess(false),
      m_processFailures(0),
      m_processStarted(false),
      m_fileLogOutputter(NULL),
      m_ready(false),
      m_foreground(foreground)
{
  m_mutex = ARCH->newMutex();
  m_condVar = ARCH->newCondVar();
}

MSWindowsWatchdog::~MSWindowsWatchdog()
{
  if (m_condVar != NULL) {
    ARCH->closeCondVar(m_condVar);
  }

  if (m_mutex != NULL) {
    ARCH->closeMutex(m_mutex);
  }
}

void MSWindowsWatchdog::startAsync()
{
  m_thread = new Thread(new TMethodJob<MSWindowsWatchdog>(this, &MSWindowsWatchdog::mainLoop, nullptr));

  m_outputThread = new Thread(new TMethodJob<MSWindowsWatchdog>(this, &MSWindowsWatchdog::outputLoop, nullptr));
}

void MSWindowsWatchdog::stop()
{
  m_monitoring = false;

  if (!m_thread->wait(5)) {
    LOG((CLOG_WARN "could not stop main thread"));
  }
  delete m_thread;

  if (!m_outputThread->wait(5)) {
    LOG((CLOG_WARN "could not stop output thread"));
  }
  delete m_outputThread;
}

HANDLE
MSWindowsWatchdog::duplicateProcessToken(HANDLE process, LPSECURITY_ATTRIBUTES security)
{
  HANDLE sourceToken;

  BOOL tokenRet = OpenProcessToken(process, TOKEN_ASSIGN_PRIMARY | TOKEN_ALL_ACCESS, &sourceToken);

  if (!tokenRet) {
    LOG((CLOG_ERR "could not open token, process handle: %d", process));
    throw XArch(new XArchEvalWindows());
  }

  LOG((CLOG_DEBUG "got token %i, duplicating", sourceToken));

  HANDLE newToken;
  BOOL duplicateRet = DuplicateTokenEx(
      sourceToken, TOKEN_ASSIGN_PRIMARY | TOKEN_ALL_ACCESS, security, SecurityImpersonation, TokenPrimary, &newToken
  );

  if (!duplicateRet) {
    LOG((CLOG_ERR "could not duplicate token %i", sourceToken));
    throw XArch(new XArchEvalWindows());
  }

  LOG((CLOG_DEBUG "duplicated, new token: %i", newToken));
  return newToken;
}

HANDLE
MSWindowsWatchdog::getUserToken(LPSECURITY_ATTRIBUTES security, bool elevatedToken)
{
  m_session.updateActiveSession();

  // always elevate if we are at the vista/7 login screen. we could also
  // elevate for the uac dialog (consent.exe) but this would be pointless,
  // since deskflow would re-launch as non-elevated after the desk switch,
  // and so would be unusable with the new elevated process taking focus.
  if (elevatedToken || m_session.isProcessInSession("logonui.exe", NULL)) {

    LOG((CLOG_DEBUG "getting elevated token, %s", (elevatedToken ? "elevation required" : "at login screen")));

    HANDLE process;
    if (!m_session.isProcessInSession("winlogon.exe", &process)) {
      throw XMSWindowsWatchdogError("cannot get user token without winlogon.exe");
    }

    try {
      return duplicateProcessToken(process, security);
    } catch (XArch &e) {
      LOG_ERR("failed to duplicate user token from winlogon.exe");
      CloseHandle(process);
      throw e;
    }
  } else {
    LOG((CLOG_DEBUG "getting non-elevated token"));
    return m_session.getUserToken(security);
  }
}

void MSWindowsWatchdog::mainLoop(void *)
{
  LOG_DEBUG("starting watchdog main loop");

  shutdownExistingProcesses();

  // the SendSAS function is used to send a sas (secure attention sequence) to the
  // winlogon process. this is used to switch to the login screen.
  SendSas sendSasFunc = NULL;
  HINSTANCE sasLib = LoadLibrary("sas.dll");
  if (sasLib) {
    LOG_DEBUG("loaded sas.dll, used to simulate ctrl-alt-del");
    sendSasFunc = (SendSas)GetProcAddress(sasLib, "SendSAS");
    if (!sendSasFunc) {
      LOG_ERR("could not find SendSAS function in sas.dll");
      throw XArch(new XArchEvalWindows());
    }
  }

  SECURITY_ATTRIBUTES saAttr;
  saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
  saAttr.bInheritHandle = TRUE;
  saAttr.lpSecurityDescriptor = NULL;

  if (!CreatePipe(&m_outputReadPipe, &m_outputWritePipe, &saAttr, 0)) {
    LOG_ERR("could not create output pipe");
    throw XArch(new XArchEvalWindows());
  }

  // Set the pipe to non-blocking mode, which allows us to stop the output reader thread immediately
  // in order to speed up the shutdown process when the Windows service needs to stop.
  DWORD mode = PIPE_NOWAIT;
  if (!SetNamedPipeHandleState(m_outputReadPipe, &mode, nullptr, nullptr)) {
    LOG_ERR("could not set pipe to non-blocking mode");
    throw XArch(new XArchEvalWindows());
  }

  while (m_monitoring) {
    try {

      if (m_processStarted && getCommand().empty()) {
        LOG((CLOG_DEBUG "process started but command is empty, shutting down"));
        shutdownExistingProcesses();
        continue;
      }

      if (m_processFailures > 1) {
        // increasing backoff period, maximum of 10 seconds.
        // only start sleeping at 1 second to avoid unnecessary delay when the process stops
        // for the first failure (i.e. when the process just stopped running).
        int timeout = m_processFailures < 10 ? m_processFailures : 10;
        LOG_WARN("backing off after failure, wait=%ds, failures=%d", timeout, m_processFailures);
        ARCH->sleep(timeout);
      }

      if (!getCommand().empty()) {
        bool startNeeded = false;

        if (m_processFailures != 0) {
          startNeeded = true;
        } else if (!m_foreground && m_session.hasChanged()) {
          startNeeded = true;
        } else if (m_commandChanged) {
          startNeeded = true;
        }

        if (startNeeded) {
          startProcess();
        }
      } else {
        // prevent backoff when no command is set.
        m_processFailures = 0;
      }

      if (m_processStarted && !isProcessRunning()) {

        m_processFailures++;
        m_processStarted = false;

        LOG((CLOG_WARN "detected application not running, pid=%d", m_process->info().dwProcessId));
      }

      HANDLE sendSasEvent = CreateEvent(NULL, FALSE, FALSE, "Global\\SendSAS");
      if (sendSasEvent != NULL) {

        // use SendSAS event to wait for next session (timeout 1 second).
        if (WaitForSingleObject(sendSasEvent, 1000) == WAIT_OBJECT_0) {
          LOG_DEBUG("calling SendSAS from sas.dll");
          sendSasFunc(FALSE);
        }

        CloseHandle(sendSasEvent);
      } else {
        LOG((CLOG_ERR "could not create SendSAS event"));
      }

      // Sleep for only 100ms rather than 1 second so that the service can shut down faster.
      ARCH->sleep(0.1);

    } catch (std::exception &e) {
      LOG((CLOG_CRIT "failed to launch, error: %s", e.what()));
      m_processFailures++;
      m_processStarted = false;
      continue;
    } catch (...) {
      LOG((CLOG_CRIT "failed to launch, unknown error."));
      m_processFailures++;
      m_processStarted = false;
      continue;
    }
  }

  if (m_process != nullptr) {
    LOG((CLOG_DEBUG "terminated running process on exit"));
    m_process->shutdown();
    m_process.reset();
    m_processStarted = false;
  }

  LOG((CLOG_DEBUG "watchdog main loop finished"));
}

bool MSWindowsWatchdog::isProcessRunning()
{
  if (m_process == nullptr) {
    return false;
  }

  DWORD exitCode;
  GetExitCodeProcess(m_process->info().hProcess, &exitCode);
  return exitCode == STILL_ACTIVE;
}

void MSWindowsWatchdog::setFileLogOutputter(FileLogOutputter *outputter)
{
  m_fileLogOutputter = outputter;
}

void MSWindowsWatchdog::startProcess()
{
  LOG_INFO("daemon starting new process");

  if (m_command.empty()) {
    throw XMSWindowsWatchdogError("cannot start process, command is empty");
  }

  m_commandChanged = false;

  if (m_process != nullptr) {
    LOG((CLOG_DEBUG "closing existing process to make way for new one"));
    m_process->shutdown();
    m_process.reset();
    m_processStarted = false;
  }

  m_process = std::make_unique<deskflow::platform::MSWindowsProcess>(m_command, m_outputWritePipe, m_outputWritePipe);

  BOOL createRet;
  if (m_foreground) {
    LOG_DEBUG("starting command in foreground");
    createRet = m_process->startInForeground();
  } else {
    LOG_DEBUG("starting new process in user session");

    LOG_DEBUG("getting active desktop name");
    const auto activeDesktopName = runActiveDesktopUtility();

    LOG_DEBUG("active desktop name: %s", activeDesktopName.c_str());
    // When we're at a UAC prompt, lock screen, or the login screen, Windows switches to the Winlogon desktop.
    const auto isOnSecureDesktop = activeDesktopName == "Winlogon";

    SECURITY_ATTRIBUTES sa;
    ZeroMemory(&sa, sizeof(SECURITY_ATTRIBUTES));
    HANDLE userToken = getUserToken(&sa, isOnSecureDesktop || m_elevateProcess);

    // set UIAccess to fix Windows 8 GUI interaction
    DWORD uiAccess = 1;
    SetTokenInformation(userToken, TokenUIAccess, &uiAccess, sizeof(DWORD));

    createRet = m_process->startAsUser(userToken, &sa);
  }

  if (!createRet) {
    LOG((CLOG_CRIT "could not launch command"));
    DWORD exitCode = 0;
    GetExitCodeProcess(m_process->info().hProcess, &exitCode);
    LOG((CLOG_ERR "exit code: %d", exitCode));
    throw XArch(new XArchEvalWindows);
  } else {
    // Wait for program to fail. This needs to be 1 second, as the process may take some time to fail.
    LOG_DEBUG("waiting for process start result");
    ARCH->sleep(1);

    if (!isProcessRunning()) {
      m_process.reset();
      throw XMSWindowsWatchdogError("process immediately stopped");
    }

    m_processStarted = true;
    m_processFailures = 0;

    LOG((CLOG_DEBUG "started core process from daemon"));
    LOG(
        (CLOG_DEBUG2 "process info, session=%i, elevated: %s, command=%s", m_session.getActiveSessionId(),
         m_elevateProcess ? "yes" : "no", m_command.c_str())
    );
  }
}

void MSWindowsWatchdog::setCommand(const std::string &command, bool elevate)
{
  LOG_DEBUG("command parameters updated");
  m_command = command;
  m_elevateProcess = elevate;
  m_commandChanged = true;
  m_processFailures = 0;
}

std::string MSWindowsWatchdog::getCommand() const
{
  if (!m_autoDetectCommand) {
    return m_command;
  }

  // seems like a fairly convoluted way to get the process name
  const char *launchName = App::instance().argsBase().m_pname;
  std::string args = ARCH->commandLine();

  // build up a full command line
  std::stringstream cmdTemp;
  cmdTemp << launchName << args;

  std::string cmd = cmdTemp.str();

  size_t i;
  std::string find = "--relaunch";
  while ((i = cmd.find(find)) != std::string::npos) {
    cmd.replace(i, find.length(), "");
  }

  return cmd;
}

void MSWindowsWatchdog::outputLoop(void *)
{
  // +1 char for \0
  CHAR buffer[kOutputBufferSize + 1];

  while (m_monitoring) {

    DWORD bytesRead;
    BOOL success = ReadFile(m_outputReadPipe, buffer, kOutputBufferSize, &bytesRead, NULL);

    if (!success || bytesRead == 0) {
      // Sleep for only 100ms rather than 1 second so that the service can shut down faster.
      ARCH->sleep(0.1);
    } else {
      buffer[bytesRead] = '\0';

      // strip out windows \r chars to prevent extra lines in log file.
      std::string output(buffer);
      if (!output.empty()) {
        size_t pos = 0;
        while ((pos = output.find("\r", pos)) != std::string::npos) {
          output.replace(pos, 1, "");
        }

        // trip ending newline, as file writer will add it's own newline.
        if (output[output.length() - 1] == '\n') {
          output = output.substr(0, output.length() - 1);
        }
      }

      if (m_fileLogOutputter != NULL) {
        m_fileLogOutputter->write(kPRINT, output.c_str());
      }

#if SYSAPI_WIN32
      if (m_foreground) {
        // when in foreground mode (useful for debugging), send the core
        // process output to the VS debug output window.
        // we could use the MSWindowsDebugOutputter, but it's really fiddly to
        // so, and there doesn't seem to be an advantage of doing that.
        OutputDebugString(buffer);
      }
#endif
    }
  }
}

HANDLE openProcessForKill(PROCESSENTRY32 entry)
{
  // pid 0 is 'system idle process'
  if (entry.th32ProcessID == 0)
    return nullptr;

  if (_stricmp(entry.szExeFile, "deskflow-client.exe") != 0 && //
      _stricmp(entry.szExeFile, "deskflow-server.exe") != 0 && //
      _stricmp(entry.szExeFile, "deskflow-core.exe") != 0) {
    return nullptr;
  }

  HANDLE handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, entry.th32ProcessID);
  if (handle == nullptr) {
    LOG((CLOG_ERR "could not open process handle for kill"));
    throw XArch(new XArchEvalWindows);
  }

  // only shut down if not current process (daemon is now the same unified binary).
  if (entry.th32ProcessID == GetCurrentProcessId()) {
    CloseHandle(handle);
    return nullptr;
  }

  return handle;
}

void MSWindowsWatchdog::shutdownExistingProcesses()
{
  LOG_DEBUG("shutting down existing processes");

  // first we need to take a snapshot of the running processes
  HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, CURRENT_PROCESS_ID);
  if (snapshot == INVALID_HANDLE_VALUE) {
    LOG((CLOG_ERR "could not get process snapshot"));
    throw XArch(new XArchEvalWindows);
  }

  PROCESSENTRY32 entry;
  entry.dwSize = sizeof(PROCESSENTRY32);

  // get the first process, and if we can't do that then it's
  // unlikely we can go any further
  BOOL gotEntry = Process32First(snapshot, &entry);
  if (!gotEntry) {
    LOG((CLOG_ERR "could not get first process entry"));
    throw XArch(new XArchEvalWindows);
  }

  // now just iterate until we can find winlogon.exe pid
  DWORD pid = 0;
  while (gotEntry) {

    HANDLE handle = openProcessForKill(entry);
    if (handle) {
      LOG((CLOG_INFO "shutting down process, name=%s, pid=%d", entry.szExeFile, entry.th32ProcessID));
      deskflow::platform::MSWindowsProcess::shutdown(handle, entry.th32ProcessID);
      CloseHandle(handle);
    }

    // now move on to the next entry (if we're not at the end)
    gotEntry = Process32Next(snapshot, &entry);
    if (!gotEntry) {

      DWORD err = GetLastError();
      if (err != ERROR_NO_MORE_FILES) {

        // only worry about error if it's not the end of the snapshot
        LOG((CLOG_ERR "could not get next process entry"));
        throw XArch(new XArchEvalWindows);
      }
    }
  }

  CloseHandle(snapshot);
}

std::string MSWindowsWatchdog::runActiveDesktopUtility()
{
  const auto installDir = ARCH->getInstalledDirectory();
  const auto coreBinPath = installDir + "\\deskflow-server.exe";
  std::string utilityCommand = "\"" + coreBinPath + "\" --active-desktop";

  LOG_DEBUG("starting active desktop utility: %s", utilityCommand.c_str());

  SECURITY_ATTRIBUTES sa;
  ZeroMemory(&sa, sizeof(SECURITY_ATTRIBUTES));
  HANDLE userToken = getUserToken(&sa, true);

  deskflow::platform::MSWindowsProcess process(utilityCommand);
  process.createPipes();

  if (!process.startAsUser(userToken, &sa)) {
    LOG_ERR("could not start active desktop process");
    throw XArch(new XArchEvalWindows());
  }

  LOG_DEBUG("started active desktop process, pid=%d", process.info().dwProcessId);
  if (const auto exitCode = process.waitForExit(); exitCode != kExitSuccess) {
    LOG_ERR("active desktop process, exit code: %d", exitCode);
    throw XMSWindowsWatchdogError("could not get active desktop");
  }

  LOG_DEBUG("reading active desktop std error");
  if (const auto error = process.readStdError(); !error.empty()) {
    LOG_WARN("active desktop process, error: %s", error.c_str());
  }

  LOG_DEBUG("reading active desktop std output");
  auto output = process.readStdOutput();
  if (output.empty()) {
    LOG_ERR("could not get active desktop, no output");
    throw XMSWindowsWatchdogError("could not get active desktop");
  }

  output.erase(output.find_last_not_of("\r\n") + 1);
  return output;
}
