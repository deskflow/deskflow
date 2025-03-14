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
#include "mt/Thread.h"
#include "platform/MSWindowsHandle.h"

#include <Shellapi.h>
#include <UserEnv.h>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

//
// Free functions
//

std::string trimOutputBuffer(const CHAR *buffer)
{
  // strip out windows \r chars to prevent extra lines in log file.
  std::string output(buffer);
  if (output.empty()) {
    LOG_DEBUG1("output buffer is empty");
    return output;
  }

  size_t pos = 0;
  while ((pos = output.find("\r", pos)) != std::string::npos) {
    output.replace(pos, 1, "");
  }

  // trip ending newline, as file writer will add it's own newline.
  if (output[output.length() - 1] == '\n') {
    output = output.substr(0, output.length() - 1);
  }

  return output;
}

HANDLE openProcessForKill(const PROCESSENTRY32 &entry)
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
    LOG_ERR("could not open process handle for kill");
    throw XArch(new XArchEvalWindows);
  }

  // only shut down if not current process (daemon is now the same unified binary).
  if (entry.th32ProcessID == GetCurrentProcessId()) {
    CloseHandle(handle);
    return nullptr;
  }

  return handle;
}

//
// MSWindowsWatchdog
//

MSWindowsWatchdog::MSWindowsWatchdog(bool foreground, FileLogOutputter &fileLogOutputter)
    : m_fileLogOutputter(fileLogOutputter),
      m_foreground(foreground)
{
  initSasFunc();
  initOutputReadPipe();
}

void MSWindowsWatchdog::startAsync()
{
  m_mainThread = std::make_unique<Thread>(new TMethodJob(this, &MSWindowsWatchdog::mainLoop, nullptr));
  m_outputThread = std::make_unique<Thread>(new TMethodJob(this, &MSWindowsWatchdog::outputLoop, nullptr));
  m_sasThread = std::make_unique<Thread>(new TMethodJob(this, &MSWindowsWatchdog::sasLoop, nullptr));
}

void MSWindowsWatchdog::stop()
{
  const auto kThreadWaitSeconds = 5;

  m_running = false;

  if (!m_mainThread->wait(kThreadWaitSeconds)) {
    LOG_WARN("could not stop main thread");
  }

  if (!m_outputThread->wait(kThreadWaitSeconds)) {
    LOG_WARN("could not stop output thread");
  }

  if (!m_sasThread->wait(kThreadWaitSeconds)) {
    LOG_WARN("could not stop sas thread");
  }
}

HANDLE
MSWindowsWatchdog::duplicateProcessToken(HANDLE process, LPSECURITY_ATTRIBUTES security)
{
  HANDLE sourceToken;

  BOOL tokenRet = OpenProcessToken(process, TOKEN_ASSIGN_PRIMARY | TOKEN_ALL_ACCESS, &sourceToken);

  if (!tokenRet) {
    LOG_ERR("could not open token, process handle: %d", process);
    throw XArch(new XArchEvalWindows());
  }

  LOG_DEBUG("got token %i, duplicating", sourceToken);

  HANDLE newToken;
  BOOL duplicateRet = DuplicateTokenEx(
      sourceToken, TOKEN_ASSIGN_PRIMARY | TOKEN_ALL_ACCESS, security, SecurityImpersonation, TokenPrimary, &newToken
  );

  if (!duplicateRet) {
    LOG_ERR("could not duplicate token %i", sourceToken);
    throw XArch(new XArchEvalWindows());
  }

  LOG_DEBUG("duplicated, new token: %i", newToken);
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
  if (elevatedToken || m_session.isProcessInSession("logonui.exe", nullptr)) {

    LOG_DEBUG("getting elevated token, %s", (elevatedToken ? "elevation required" : "at login screen"));

    HANDLE process;
    if (!m_session.isProcessInSession("winlogon.exe", &process)) {
      throw XArch("cannot get user token without winlogon.exe");
    }

    try {
      return duplicateProcessToken(process, security);
    } catch (XArch &e) {
      LOG_ERR("failed to duplicate user token from winlogon.exe");
      CloseHandle(process);
      throw e;
    }
  } else {
    LOG_DEBUG("getting non-elevated token");
    return m_session.getUserToken(security);
  }
}

void MSWindowsWatchdog::mainLoop(void *)
{
  shutdownExistingProcesses();

  LOG_DEBUG("starting watchdog main loop");
  while (m_running) {
    LOG_DEBUG3("locking process state mutex in watchdog main loop");
    std::unique_lock lock(m_processStateMutex);

    if (!m_command.empty() && !m_foreground && m_session.hasChanged()) {
      LOG_DEBUG("session changed, queueing process start");
      m_processState = ProcessState::StartPending;
      m_nextStartTime.reset();
    }

    switch (m_processState) {
      using enum ProcessState;

    case Idle:
      LOG_DEBUG3("watchdog process state idle");
      break;

    case StartScheduled: {
      LOG_DEBUG3("watchdog process start scheduled");
      if (m_nextStartTime.has_value() && m_nextStartTime.value() <= ARCH->time()) {
        LOG_DEBUG("start time reached, queueing process start");
        m_processState = StartPending;
      }
    } break;

    case StartPending: {
      LOG_DEBUG("watchdog starting new process");
      try {
        startProcess();
        m_startFailures = 0;
        m_processState = Running;
      } catch (std::exception &e) { // NOSONAR - Catching all exceptions
        handleStartError(e.what());
        m_processState = StartPending;
      } catch (...) { // NOSONAR - Catching remaining exceptions
        handleStartError();
        m_processState = StartPending;
      }
    } break;

    case Running: {
      LOG_DEBUG3("watchdog process in running state");
      if (!isProcessRunning()) {
        LOG_WARN("detected application not running, pid=%d", m_process->info().dwProcessId);
        m_processState = StartPending;
      }
    } break;

    case StopPending: {
      LOG_DEBUG("watchdog stopping current process");
      if (m_process != nullptr) {
        m_process->shutdown();
        m_process.reset();
      } else {
        LOG_WARN("no process to stop");
      }
      shutdownExistingProcesses();
      m_processState = Idle;
    } break;
    }

    LOG_DEBUG3("unlocking process state mutex in watchdog main loop");
    lock.unlock();

    // Sleep for only 100ms rather than 1 second so that the service can shut down faster.
    LOG_DEBUG3("watchdog main loop sleeping");
    ARCH->sleep(0.1);
  }

  LOG_DEBUG("watchdog main loop finished");

  if (m_process != nullptr) {
    LOG_DEBUG("terminated running process on exit");
    m_process->shutdown();
    m_process.reset();
  }

  shutdownExistingProcesses();
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

void MSWindowsWatchdog::startProcess()
{
  if (m_command.empty()) {
    throw XArch("cannot start process, command is empty");
  }

  if (m_process != nullptr) {
    LOG_DEBUG("closing existing process to make way for new one");
    m_process->shutdown();
    m_process.reset();
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
    DWORD exitCode = 0;
    GetExitCodeProcess(m_process->info().hProcess, &exitCode);
    LOG_ERR("daemon failed to run command, exit code: %d", exitCode);
    throw XArch(new XArchEvalWindows);
  } else {
    // Wait for program to fail. This needs to be 1 second, as the process may take some time to fail.
    LOG_DEBUG("watchdog waiting for process start result");
    ARCH->sleep(1);

    if (!isProcessRunning()) {
      m_process.reset();
      throw XArch("process immediately stopped");
    }

    LOG_DEBUG("started core process from watchdog");
    LOG_DEBUG2(
        "process info, session=%i, elevated=%s, command: %s", //
        m_session.getActiveSessionId(), m_elevateProcess ? "yes" : "no", m_command.c_str()
    );
  }
}

void MSWindowsWatchdog::setProcessConfig(const std::string_view &command, bool elevate)
{
  LOG_DEBUG1("locking process state mutex for watchdog config change");
  std::lock_guard lock(m_processStateMutex);

  LOG_DEBUG("setting watchdog process config");
  m_command = command;
  m_elevateProcess = elevate;

  if (m_command.empty()) {
    LOG_DEBUG("command cleared, queueing process stop");
    m_processState = ProcessState::StopPending;
  } else {
    LOG_DEBUG("command changed, queueing process start");
    m_processState = ProcessState::StartPending;
    m_nextStartTime.reset();
  }
}

void MSWindowsWatchdog::outputLoop(void *)
{
  const auto kOutputBufferSize = 4096;

  // +1 char for \0
  CHAR buffer[kOutputBufferSize + 1];

  while (m_running) {

    DWORD bytesRead;
    BOOL success = ReadFile(m_outputReadPipe, buffer, kOutputBufferSize, &bytesRead, nullptr);

    if (!success || bytesRead == 0) {
      // Sleep for only 100ms rather than 1 second so that the service can shut down faster.
      ARCH->sleep(0.1);
    } else {
      buffer[bytesRead] = '\0';

      // strip out windows \r chars to prevent extra lines in log file.
      std::string output = trimOutputBuffer(buffer);
      m_fileLogOutputter.write(kPRINT, output.c_str());

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

void MSWindowsWatchdog::shutdownExistingProcesses()
{
  LOG_DEBUG("shutting down any existing processes");

  const auto kAllProcesses = 0;

  // first we need to take a snapshot of the running processes
  HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, kAllProcesses);
  if (snapshot == INVALID_HANDLE_VALUE) {
    LOG_ERR("could not get process snapshot");
    throw XArch(new XArchEvalWindows);
  }

  PROCESSENTRY32 entry;
  entry.dwSize = sizeof(PROCESSENTRY32);

  // get the first process, and if we can't do that then it's
  // unlikely we can go any further
  BOOL gotEntry = Process32First(snapshot, &entry);
  if (!gotEntry) {
    LOG_ERR("could not get first process entry");
    throw XArch(new XArchEvalWindows);
  }

  // now just iterate until we can find winlogon.exe pid
  while (gotEntry) {

    if (HANDLE handle = openProcessForKill(entry); handle != nullptr) {
      LOG_DEBUG("shutting down process, name=%s, pid=%d", entry.szExeFile, entry.th32ProcessID);
      deskflow::platform::MSWindowsProcess::shutdown(handle, entry.th32ProcessID);
      CloseHandle(handle);
    }

    // now move on to the next entry (if we're not at the end)
    gotEntry = Process32Next(snapshot, &entry);
    if (!gotEntry) {

      DWORD err = GetLastError();
      if (err != ERROR_NO_MORE_FILES) {

        // only worry about error if it's not the end of the snapshot
        LOG_ERR("could not get next process entry");
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
    throw XArch("could not get active desktop");
  }

  LOG_DEBUG("reading active desktop std error");
  if (const auto error = process.readStdError(); !error.empty()) {
    LOG_WARN("active desktop process, error: %s", error.c_str());
  }

  LOG_DEBUG("reading active desktop std output");
  auto output = process.readStdOutput();
  if (output.empty()) {
    LOG_ERR("could not get active desktop, no output");
    throw XArch("could not get active desktop");
  }

  output.erase(output.find_last_not_of("\r\n") + 1);
  return output;
}

void MSWindowsWatchdog::handleStartError(const std::string_view &message)
{
  const auto kStartDelaySeconds = 1;

  m_startFailures++;

  if (!message.empty()) {
    LOG_CRIT("failed to launch, error: %s", message.data());
  } else {
    LOG_CRIT("failed to launch, unknown error");
  }

  // When there has been more than one consecutive failure, slow down the retry rate.
  if (m_startFailures > 1) {
    m_nextStartTime = ARCH->time() + kStartDelaySeconds;
    LOG_WARN("start failed %d times, delaying start", m_startFailures);
    LOG_DEBUG("start delay, seconds=%d, time=%f", kStartDelaySeconds, m_nextStartTime.value());
  } else {
    LOG_INFO("retrying process start immediately");
    m_nextStartTime.reset();
  }
}

std::string MSWindowsWatchdog::processStateToString(MSWindowsWatchdog::ProcessState state)
{
  switch (state) {
    using enum MSWindowsWatchdog::ProcessState;

  case Idle:
    return "Idle";
  case StartScheduled:
    return "StartScheduled";
  case StartPending:
    return "StartPending";
  case StopPending:
    return "StopPending";
  case Running:
    return "Running";
  }

  return "Unknown";
}

void MSWindowsWatchdog::initOutputReadPipe()
{
  SECURITY_ATTRIBUTES saAttr;
  saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
  saAttr.bInheritHandle = TRUE;
  saAttr.lpSecurityDescriptor = nullptr;

  if (!CreatePipe(&m_outputReadPipe, &m_outputWritePipe, &saAttr, 0)) {
    LOG_ERR("could not create output pipe");
    throw XArch(new XArchEvalWindows());
  }

  // Set the pipe to non-blocking mode, which allows us to stop the output reader thread immediately
  // in order to speed up the shutdown process when the Windows service needs to stop.
  if (DWORD mode = PIPE_NOWAIT; !SetNamedPipeHandleState(m_outputReadPipe, &mode, nullptr, nullptr)) {
    LOG_ERR("could not set pipe to non-blocking mode");
    throw XArch(new XArchEvalWindows());
  }
}

void MSWindowsWatchdog::initSasFunc()
{
  // the SendSAS function is used to send a sas (secure attention sequence) to the
  // winlogon process. this is used to switch to the login screen.
  HINSTANCE sasLib = LoadLibrary("sas.dll");
  if (!sasLib) {
    LOG_ERR("could not load sas.dll");
    throw XArch(new XArchEvalWindows());
  }

  LOG_DEBUG("loaded sas.dll, used to simulate ctrl-alt-del");
  m_sendSasFunc = (SendSas)GetProcAddress(sasLib, "SendSAS");
  if (!m_sendSasFunc) {
    LOG_ERR("could not find SendSAS function in sas.dll");
    throw XArch(new XArchEvalWindows());
  }

  LOG_DEBUG("found SendSAS function in sas.dll");
}

void MSWindowsWatchdog::sasLoop(void *) // NOSONAR - Thread entry point signature
{
  LOG_DEBUG3("watchdog creating sas event");

  if (m_sendSasFunc == nullptr) {
    throw XArch("SendSAS function not initialized");
  }

  while (m_running) {
    if (m_processState != ProcessState::Running) {
      LOG_DEBUG2("watchdog not running, skipping SendSAS");
      ARCH->sleep(1);
      continue;
    }

    // Create a an event so that other processes can tell the daemon to call the `SendSAS` function.
    MSWindowsHandle sendSasEvent(CreateEvent(nullptr, FALSE, FALSE, kSendSasEventName));
    if (sendSasEvent.get() == nullptr) {
      XArchEvalWindows error;
      LOG_ERR("could not create SAS event, error: %s", error.eval().c_str());
      ARCH->sleep(1);
      continue;
    }

    // Wait for the Core client to tell the daemon to call the `SendSAS` function.
    if (WaitForSingleObject(sendSasEvent.get(), 1000) == WAIT_OBJECT_0) {

      // The SoftwareSASGeneration registry key must be set for this to work:
      //   Set-ItemProperty -Path HKLM:Software\Microsoft\Windows\CurrentVersion\Policies\System
      //     -Name SoftwareSASGeneration -Value 1
      LOG_DEBUG("calling SendSAS to simulate ctrl+alt+del");
      m_sendSasFunc(FALSE);
    }
  }
}
