/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2025 Synergy App Ltd
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/MSWindowsWatchdog.h"

#include "arch/Arch.h"
#include "arch/win32/XArchWindows.h"
#include "base/Log.h"
#include "base/LogOutputters.h"
#include "base/TMethodJob.h"
#include "common/Constants.h"
#include "common/LogLevel.h"
#include "common/CoordinationLocalStatus.h"
#include "common/FleetCursor.h"
#include "deskflow/App.h"
#include "mt/Thread.h"
#include "platform/MSWindowsHandle.h"

#include <Shellapi.h>
#include <UserEnv.h>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <shlobj.h>
#include <tchar.h>
#include <tlhelp32.h>

#include <QStringDecoder>

#include <algorithm>

//
// Free functions
//

HANDLE openProcessForKill(const PROCESSENTRY32 &entry)
{
  // pid 0 is 'system idle process'
  if (entry.th32ProcessID == 0)
    return nullptr;

  if (_wcsicmp(entry.szExeFile, L"deskflow-client.exe") != 0 && //
      _wcsicmp(entry.szExeFile, L"deskflow-server.exe") != 0 && //
      _wcsicmp(entry.szExeFile, L"deskflow-core.exe") != 0) {
    return nullptr;
  }

  const DWORD desiredAccess = PROCESS_TERMINATE | PROCESS_QUERY_LIMITED_INFORMATION | SYNCHRONIZE;
  HANDLE handle = OpenProcess(desiredAccess, FALSE, entry.th32ProcessID);
  if (handle == nullptr) {
    LOG_WARN(
        "could not open process handle for kill, pid=%u, error=%s", entry.th32ProcessID,
        windowsErrorToString(GetLastError()).c_str()
    );
    return nullptr;
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
    throw std::runtime_error(windowsErrorToString(GetLastError()));
  }

  LOG_DEBUG("got token %i, duplicating", sourceToken);

  HANDLE newToken;
  BOOL duplicateRet = DuplicateTokenEx(
      sourceToken, TOKEN_ASSIGN_PRIMARY | TOKEN_ALL_ACCESS, security, SecurityImpersonation, TokenPrimary, &newToken
  );

  if (!duplicateRet) {
    CloseHandle(sourceToken);
    LOG_ERR("could not duplicate token %i", sourceToken);
    throw std::runtime_error(windowsErrorToString(GetLastError()));
  }

  CloseHandle(sourceToken);
  LOG_DEBUG("duplicated, new token: %i", newToken);
  return newToken;
}

HANDLE
MSWindowsWatchdog::getUserToken(LPSECURITY_ATTRIBUTES security, bool elevatedToken)
{
  m_session.updateActiveSession();

  if (elevatedToken) {

    LOG_DEBUG("getting elevated token");

    HANDLE process;
    if (!m_session.isProcessInSession(L"winlogon.exe", &process)) {
      throw std::runtime_error("cannot get user token without winlogon.exe");
    }
    if (process == nullptr) {
      throw std::runtime_error("found winlogon.exe but failed to open process handle");
    }

    try {
      HANDLE token = duplicateProcessToken(process, security);
      if (process != nullptr) {
        CloseHandle(process);
      }
      return token;
    } catch (...) {
      LOG_ERR("failed to duplicate user token from winlogon.exe");
      if (process != nullptr) {
        CloseHandle(process);
      }
      throw;
    }
  } else {
    LOG_DEBUG("getting non-elevated token");
    return m_session.getUserToken(security);
  }
}

void MSWindowsWatchdog::mainLoop(const void *)
{
  using enum ProcessState;

  shutdownExistingProcesses();

  LOG_DEBUG("starting watchdog main loop");
  while (m_running) {
    LOG_VERBOSE("locking process state mutex in watchdog main loop");
    std::unique_lock lock(m_processStateMutex);

    // Auto-elevate: relaunch when the session changes, or (when elevation is
    // enabled) when the secure/login desktop appears or clears, so the core's
    // integrity matches the desktop it must inject on. Secure-desktop changes
    // are debounced so a brief consent.exe/LogonUI flicker does not kill the
    // core and drop every peer in the coordination mesh.
    if (m_processState == Running && !m_command.empty() && !m_foreground) {
      if (m_session.hasChanged()) {
        LOG_DEBUG("session changed, queueing process start");
        m_processState = StartPending;
        m_nextStartTime.reset();
        m_pendingElevated.reset();
        m_pendingElevatedSince.reset();
      } else if (m_elevateProcess) {
        const bool secureNow = wantsElevatedCore();
        if (secureNow == m_lastElevated) {
          m_pendingElevated.reset();
          m_pendingElevatedSince.reset();
        } else if (!m_pendingElevated.has_value() || m_pendingElevated.value() != secureNow) {
          m_pendingElevated = secureNow;
          m_pendingElevatedSince = Arch::time();
          LOG_DEBUG(
              "secure-desktop transition pending (target elevated=%s), debouncing %.1fs",
              secureNow ? "yes" : "no", kSecureDesktopDebounceSeconds
          );
        } else if (Arch::time() - m_pendingElevatedSince.value() >= kSecureDesktopDebounceSeconds) {
          LOG_DEBUG("secure-desktop transition stable, queueing process start");
          m_processState = StartPending;
          m_nextStartTime.reset();
          m_pendingElevated.reset();
          m_pendingElevatedSince.reset();
        }
      }
    }

    switch (m_processState) {
    case Idle:
      LOG_VERBOSE("watchdog process state idle");
      break;

    case StartScheduled: {
      LOG_VERBOSE("watchdog process start scheduled");
      if (m_nextStartTime.has_value() && m_nextStartTime.value() <= Arch::time()) {
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
        m_processState = handleStartError(e.what());
      } catch (...) { // NOSONAR - Catching remaining exceptions
        m_processState = handleStartError();
      }
    } break;

    case Running: {
      LOG_VERBOSE("watchdog process in running state");
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

    LOG_VERBOSE("unlocking process state mutex in watchdog main loop");
    lock.unlock();

    // Sleep for only 100ms rather than 1 second so that the service can shut down faster.
    LOG_VERBOSE("watchdog main loop sleeping");
    Arch::sleep(0.1);
  }

  LOG_DEBUG("watchdog main loop finished");

  if (m_process != nullptr) {
    LOG_DEBUG("terminating running process on exit");
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
    throw std::runtime_error("cannot start process, command is empty");
  }

  if (m_process != nullptr) {
    LOG_DEBUG("closing existing process to make way for new one");
    // Short timeout: on a relaunch (esp. auto-elevate desktop transitions) the
    // core is being replaced immediately, so don't wait the full graceful
    // window -- a slow exit there delays reaching the secure desktop for UAC.
    m_process->shutdown(2);
    m_process.reset();
  }

  m_process = std::make_unique<deskflow::platform::MSWindowsProcess>(m_command, m_outputWritePipe, m_outputWritePipe);

  // Auto-elevate: when elevation is enabled, only run the core SYSTEM while the
  // secure/login desktop is active; otherwise run at the user's (medium)
  // integrity so user-level hook tools (PowerToys, Mouser) can see its input.
  const bool elevate = m_elevateProcess && wantsElevatedCore();
  m_lastElevated = elevate;

  LOG_INFO("running command (%s): %ls", elevate ? "elevated" : "not elevated", m_command.c_str());

  BOOL createRet;
  if (m_foreground) {
    LOG_DEBUG("starting command in foreground");
    createRet = m_process->startInForeground();
  } else {
    LOG_DEBUG("starting new process in user session");

    SECURITY_ATTRIBUTES sa;
    ZeroMemory(&sa, sizeof(SECURITY_ATTRIBUTES));
    HANDLE userToken = getUserToken(&sa, elevate);

    // UIAccess lets the core drive higher-integrity / secure-desktop UI, but a
    // UIAccess token also raises the core's *injected* input above normal
    // user-level hooks -- so PowerToys/Keyboard Manager (UIAccess=0) can't see
    // deskflow's input. Only grant it when elevated (secure/login desktop, where
    // those hooks are out of the picture anyway); on the normal desktop keep a
    // plain token so user-level hook tools intercept deskflow input.
    if (elevate) {
      DWORD uiAccess = 1;
      SetTokenInformation(userToken, TokenUIAccess, &uiAccess, sizeof(DWORD));
    }

    createRet = m_process->startAsUser(userToken, &sa);
  }

  if (!createRet) {
    DWORD exitCode = 0;
    if (GetExitCodeProcess(m_process->info().hProcess, &exitCode)) {
      LOG_ERR("daemon failed to run command, exit code: %d", exitCode);
    } else {
      LOG_ERR("daemon failed to run command, unknown exit code");
      throw std::runtime_error(windowsErrorToString(GetLastError()));
    }
  } else {
    // Wait for program to fail. This needs to be 1 second, as the process may take some time to fail.
    LOG_DEBUG("watchdog waiting for process start result");
    Arch::sleep(1);

    if (!isProcessRunning()) {
      m_process.reset();
      throw std::runtime_error("process immediately stopped");
    }

    LOG_DEBUG("started core process from watchdog");
    LOG_VERBOSE(
        "process info, session=%i, elevated=%s, command: %s", //
        m_session.getActiveSessionId(), elevate ? "yes" : "no", m_command.c_str()
    );
  }
}

bool MSWindowsWatchdog::secureDesktopActive()
{
  // session-0 daemon can't query session 1's input desktop directly, so detect
  // the secure/login desktop by the processes that own it: consent.exe (UAC
  // elevation prompt) and LogonUI.exe (lock/login screen).
  MSWindowsHandle snapshot(CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0));
  if (snapshot.get() == INVALID_HANDLE_VALUE) {
    return false;
  }

  PROCESSENTRY32 entry;
  entry.dwSize = sizeof(PROCESSENTRY32);
  if (!Process32First(snapshot.get(), &entry)) {
    return false;
  }

  do {
    if (_wcsicmp(entry.szExeFile, L"consent.exe") == 0 || _wcsicmp(entry.szExeFile, L"LogonUI.exe") == 0) {
      return true;
    }
  } while (Process32Next(snapshot.get(), &entry));

  return false;
}

bool MSWindowsWatchdog::wantsElevatedCore()
{
  if (!secureDesktopActive()) {
    return false;
  }
  if (m_coordPort == 0 || m_selfName.empty()) {
    return true;
  }
  const auto fleet = deskflow::common::pollLocalFleetStatus(m_coordPort, 500);
  if (!fleet.has_value()) {
    return true;
  }
  if (fleet->cursorHost.isEmpty()) {
    return true;
  }
  return deskflow::common::cursorHostIsLocal(m_selfName, fleet->cursorHost.toStdString());
}

void MSWindowsWatchdog::setProcessConfig(const std::string_view &command, bool elevate)
{
  LOG_VERBOSE("locking process state mutex for watchdog config change");
  std::scoped_lock lock{m_processStateMutex};

  LOG_DEBUG("setting watchdog process config");
  m_command = std::wstring(command.begin(), command.end());
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

void MSWindowsWatchdog::setElevationContext(const std::string &selfName, uint16_t coordPort)
{
  std::scoped_lock lock{m_processStateMutex};
  m_selfName = selfName;
  m_coordPort = coordPort;
}

void MSWindowsWatchdog::outputLoop(const void *)
{
  static constexpr DWORD kBufSize = 4096;

  BYTE raw[kBufSize];
  DWORD bytesRead = 0;

  // Warning: Manual decoding while we still use Win32 APIs for process launching and output reading.
  // Using a byte decoder should help to prevent mojibake when we get a partial UTF-8 sequence in a read chunk.
  // In future when we move to Qt process APIs, this can all be simplified.
  QStringDecoder decoder(QStringDecoder::Utf8);

  while (m_running) {
    const BOOL ok = ::ReadFile(m_outputReadPipe, raw, kBufSize, &bytesRead, nullptr);

    if (!ok || bytesRead == 0) {
      const DWORD err = ::GetLastError();
      if (err != NO_ERROR && err != ERROR_NO_DATA) {
        LOG_WARN("could not read from output pipe, error: %s", windowsErrorToString(err).c_str());
      }

      // Retry immediately when nothing to read or we get a transient error like ERROR_NO_DATA (pipe is non-blocking).
      Arch::sleep(0.1);
      continue;
    }

    const QString decoded =
        decoder.decode(QByteArray::fromRawData(reinterpret_cast<const char *>(raw), int(bytesRead)));

    // The file log outputter adds its own newlines, so trim the decoded string to avoid double newlines.
    const auto trimmed = decoded.trimmed();
    m_fileLogOutputter.write(LogLevel::Level::Print, trimmed);

    if (m_foreground) {
      // Doesn't add it's own newlines, so use the original ones from the process output.
      ::OutputDebugString(decoded.toStdWString().c_str());
    }
  }
}

void MSWindowsWatchdog::shutdownExistingProcesses()
{
  LOG_DEBUG("shutting down any existing processes");

  const auto kAllProcesses = 0;

  // first we need to take a snapshot of the running processes
  MSWindowsHandle snapshot(CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, kAllProcesses));
  if (snapshot.get() == INVALID_HANDLE_VALUE) {
    LOG_ERR("could not get process snapshot");
    throw std::runtime_error(windowsErrorToString(GetLastError()));
  }

  PROCESSENTRY32 entry;
  entry.dwSize = sizeof(PROCESSENTRY32);

  // get the first process, and if we can't do that then it's
  // unlikely we can go any further
  BOOL gotEntry = Process32First(snapshot.get(), &entry);
  if (!gotEntry) {
    LOG_ERR("could not get first process entry");
    throw std::runtime_error(windowsErrorToString(GetLastError()));
  }

  // now just iterate until we can find winlogon.exe pid
  while (gotEntry) {

    if (HANDLE handle = openProcessForKill(entry); handle != nullptr) {
      LOG_DEBUG("shutting down process, name=%s, pid=%d", entry.szExeFile, entry.th32ProcessID);
      deskflow::platform::MSWindowsProcess::shutdown(handle, entry.th32ProcessID);
      CloseHandle(handle);
    }

    // now move on to the next entry (if we're not at the end)
    gotEntry = Process32Next(snapshot.get(), &entry);
    if (!gotEntry) {

      DWORD err = GetLastError();
      if (err != ERROR_NO_MORE_FILES) {

        // only worry about error if it's not the end of the snapshot
        LOG_ERR("could not get next process entry");
        throw std::runtime_error(windowsErrorToString(GetLastError()));
      }
    }
  }
}

MSWindowsWatchdog::ProcessState MSWindowsWatchdog::handleStartError(const std::string_view &message)
{
  m_startFailures++;

  if (!message.empty()) {
    LOG_CRIT("daemon failed to start process, error: %s", message.data());
  } else {
    LOG_CRIT("daemon failed to start process, unknown error");
  }

  // Exponential backoff so a crash loop on one node does not hammer the mesh.
  if (m_startFailures > 1) {
    const int delaySeconds = std::min(1 << (m_startFailures - 2), 30);
    m_nextStartTime = Arch::time() + delaySeconds;
    LOG_WARN("start failed %d times, delaying start %ds", m_startFailures, delaySeconds);
    LOG_DEBUG("start delay, seconds=%d, time=%f", delaySeconds, m_nextStartTime.value());
    return ProcessState::StartScheduled;
  }

  LOG_INFO("retrying process start immediately");
  m_nextStartTime.reset();
  return ProcessState::StartPending;
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
    throw std::runtime_error(windowsErrorToString(GetLastError()));
  }

  // Set the pipe to non-blocking mode, which allows us to stop the output reader thread immediately
  // in order to speed up the shutdown process when the Windows service needs to stop.
  if (DWORD mode = PIPE_NOWAIT; !SetNamedPipeHandleState(m_outputReadPipe, &mode, nullptr, nullptr)) {
    LOG_ERR("could not set pipe to non-blocking mode");
    throw std::runtime_error(windowsErrorToString(GetLastError()));
  }
}

void MSWindowsWatchdog::initSasFunc()
{
  // the SendSAS function is used to send a sas (secure attention sequence) to the
  // winlogon process. this is used to switch to the login screen.
  HINSTANCE sasLib = LoadLibrary(L"sas.dll");
  if (!sasLib) {
    LOG_ERR("could not load sas.dll");
    throw std::runtime_error(windowsErrorToString(GetLastError()));
  }

  LOG_DEBUG("loaded sas.dll, used to simulate ctrl-alt-del");
  m_sendSasFunc = (SendSas)GetProcAddress(sasLib, "SendSAS");
  if (!m_sendSasFunc) {
    LOG_ERR("could not find SendSAS function in sas.dll");
    throw std::runtime_error(windowsErrorToString(GetLastError()));
  }

  LOG_DEBUG("found SendSAS function in sas.dll");
}

void MSWindowsWatchdog::sasLoop(const void *) // NOSONAR - Thread entry point signature
{
  LOG_VERBOSE("watchdog creating sas event");

  if (m_sendSasFunc == nullptr) {
    throw std::runtime_error("SendSAS function not initialized");
  }

  while (m_running) {
    if (m_processState != ProcessState::Running) {
      LOG_VERBOSE("watchdog not running, skipping SendSAS");
      Arch::sleep(1);
      continue;
    }

    // Create a an event so that other processes can tell the daemon to call the `SendSAS` function.
    MSWindowsHandle sendSasEvent(CreateEvent(nullptr, FALSE, FALSE, LPCWSTR(kSendSasEventName)));
    if (sendSasEvent.get() == nullptr) {
      LOG_ERR("could not create SAS event, error: %s", windowsErrorToString(GetLastError()).c_str());
      Arch::sleep(1);
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
