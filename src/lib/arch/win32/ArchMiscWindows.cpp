/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016, 2024 - 2025 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "arch/win32/ArchMiscWindows.h"

#include "arch/win32/ArchDaemonWindows.h"
#include "arch/win32/XArchWindows.h"
#include "base/Log.h"
#include "base/String.h"
#include "common/Constants.h"

#include <Psapi.h>

#include <array>
#include <filesystem>

// Useful for debugging Windows specific bootstrapping code before the logging system is initialized.
// This output can be viewed by attaching a Microsoft debugger or by using the DebugView program.
#define MS_LOG_DEBUG(message, ...)                                                                                     \
  OutputDebugStringA((deskflow::string::sprintf((s_binaryName + ": " + message + "\n").c_str(), __VA_ARGS__)).c_str())

//
// Free functions
//

void errorMessageBox(const char *message, const char *title = "Fatal Error");

std::string getBinaryName()
{
  std::array<char, MAX_PATH> buffer;
  if (!GetModuleFileNameA(nullptr, buffer.data(), MAX_PATH)) {
    errorMessageBox("Failed to get binary name.");
    abort();
  }

  return std::filesystem::path(buffer.data()).filename().string();
}

void errorMessageBox(const char *message, const char *title)
{
  MessageBoxA(nullptr, message, title, MB_ICONERROR | MB_OK);
}

// Used by bootstrap logging to differentiate between daemon and client/server messages.
const std::string s_binaryName = getBinaryName();

//
// ArchMiscWindows
//

DWORD ArchMiscWindows::s_busyState = 0;
ArchMiscWindows::STES_t ArchMiscWindows::s_stes = nullptr;
HICON ArchMiscWindows::s_largeIcon = nullptr;
HICON ArchMiscWindows::s_smallIcon = nullptr;
HINSTANCE ArchMiscWindows::s_instanceWin32 = nullptr;

void ArchMiscWindows::init()
{
  // stop windows system error dialogs from showing.
  SetErrorMode(SEM_FAILCRITICALERRORS);
}

int ArchMiscWindows::runDaemon(RunFunc runFunc)
{
  return ArchDaemonWindows::runDaemon(runFunc);
}

void ArchMiscWindows::daemonRunning(bool running)
{
  ArchDaemonWindows::daemonRunning(running);
}

void ArchMiscWindows::daemonFailed(int result)
{
  ArchDaemonWindows::daemonFailed(result);
}

UINT ArchMiscWindows::getDaemonQuitMessage()
{
  return ArchDaemonWindows::getDaemonQuitMessage();
}

HKEY ArchMiscWindows::openKey(HKEY key, const TCHAR *keyName)
{
  return openKey(key, keyName, false);
}

HKEY ArchMiscWindows::openKey(HKEY key, const TCHAR *const *keyNames)
{
  return openKey(key, keyNames, false);
}

HKEY ArchMiscWindows::addKey(HKEY key, const TCHAR *keyName)
{
  return openKey(key, keyName, true);
}

HKEY ArchMiscWindows::addKey(HKEY key, const TCHAR *const *keyNames)
{
  return openKey(key, keyNames, true);
}

HKEY ArchMiscWindows::openKey(HKEY key, const TCHAR *keyName, bool create)
{
  // ignore if parent is nullptr
  if (key == nullptr) {
    return nullptr;
  }

  // open next key
  HKEY newKey;
  LSTATUS result = RegOpenKeyEx(key, keyName, 0, KEY_WRITE | KEY_QUERY_VALUE, &newKey);
  if (result != ERROR_SUCCESS && create) {
    DWORD disp;
    result = RegCreateKeyEx(key, keyName, 0, nullptr, 0, KEY_WRITE | KEY_QUERY_VALUE, nullptr, &newKey, &disp);
  }
  if (result != ERROR_SUCCESS) {
    RegCloseKey(key);
    return nullptr;
  }

  // switch to new key
  RegCloseKey(key);
  return newKey;
}

HKEY ArchMiscWindows::openKey(HKEY key, const TCHAR *const *keyNames, bool create)
{
  for (size_t i = 0; key != nullptr && keyNames[i] != nullptr; ++i) {
    // open next key
    key = openKey(key, keyNames[i], create);
  }
  return key;
}

void ArchMiscWindows::closeKey(HKEY key)
{
  assert(key != nullptr);
  if (key == nullptr)
    return;
  RegCloseKey(key);
}

void ArchMiscWindows::deleteKey(HKEY key, const TCHAR *name)
{
  assert(key != nullptr);
  assert(name != nullptr);
  if (key == nullptr || name == nullptr)
    return;
  RegDeleteKey(key, name);
}

ArchMiscWindows::EValueType ArchMiscWindows::typeOfValue(HKEY key, const TCHAR *name)
{
  DWORD type;
  LONG result = RegQueryValueEx(key, name, 0, &type, nullptr, nullptr);
  if (result != ERROR_SUCCESS) {
    return kNO_VALUE;
  }
  switch (type) {
  case REG_DWORD:
    return kUINT;

  case REG_SZ:
    return kSTRING;

  case REG_BINARY:
    return kBINARY;

  default:
    return kUNKNOWN;
  }
}

void ArchMiscWindows::setValue(HKEY key, const TCHAR *name, const std::string &value)
{
  assert(key != nullptr);
  if (key == nullptr) {
    // TODO: throw exception
    return;
  }
  RegSetValueEx(key, name, 0, REG_SZ, reinterpret_cast<const BYTE *>(value.c_str()), (DWORD)value.size() + 1);
}

void ArchMiscWindows::setValue(HKEY key, const TCHAR *name, DWORD value)
{
  assert(key != nullptr);
  if (key == nullptr) {
    // TODO: throw exception
    return;
  }
  RegSetValueEx(key, name, 0, REG_DWORD, reinterpret_cast<CONST BYTE *>(&value), sizeof(DWORD));
}

std::wstring ArchMiscWindows::readBinaryOrString(HKEY key, const TCHAR *name, DWORD type)
{
  // get the size of the string
  DWORD actualType;
  DWORD size = 0;
  LONG result = RegQueryValueEx(key, name, 0, &actualType, nullptr, &size);
  if (result != ERROR_SUCCESS || actualType != type) {
    return std::wstring();
  }

  // if zero size then return empty string
  if (size == 0) {
    return std::wstring();
  }

  // allocate space
  wchar_t *buffer = new wchar_t[size];

  // read it
  result = RegQueryValueEx(key, name, 0, &actualType, reinterpret_cast<BYTE *>(buffer), &size);
  if (result != ERROR_SUCCESS || actualType != type) {
    delete[] buffer;
    return std::wstring();
  }

  // clean up and return value
  if (type == REG_SZ && buffer[size - 1] == '\0') {
    // don't include terminating nul;  std::string will add one.
    --size;
  }
  std::wstring value(buffer, size);
  delete[] buffer;
  return value;
}

std::wstring ArchMiscWindows::readValueString(HKEY key, const TCHAR *name)
{
  return readBinaryOrString(key, name, REG_SZ);
}

DWORD
ArchMiscWindows::readValueInt(HKEY key, const TCHAR *name)
{
  DWORD type;
  DWORD value;
  DWORD size = sizeof(value);
  LONG result = RegQueryValueEx(key, name, 0, &type, reinterpret_cast<BYTE *>(&value), &size);
  if (result != ERROR_SUCCESS || type != REG_DWORD) {
    return 0;
  }
  return value;
}

void ArchMiscWindows::addBusyState(DWORD busyModes)
{
  s_busyState |= busyModes;
  setThreadExecutionState(s_busyState);
}

void ArchMiscWindows::removeBusyState(DWORD busyModes)
{
  s_busyState &= ~busyModes;
  setThreadExecutionState(s_busyState);
}

void ArchMiscWindows::setThreadExecutionState(DWORD busyModes)
{
  // look up function dynamically so we work on older systems
  if (s_stes == nullptr) {
    HINSTANCE kernel = LoadLibrary(L"kernel32.dll");
    if (kernel != nullptr) {
      s_stes = reinterpret_cast<STES_t>(GetProcAddress(kernel, "SetThreadExecutionState"));
    }
    if (s_stes == nullptr) {
      s_stes = &ArchMiscWindows::dummySetThreadExecutionState;
    }
  }

  // convert to STES form
  DWORD state = 0;
  if ((busyModes & kSYSTEM) != 0) {
    state |= ES_SYSTEM_REQUIRED;
  }
  if ((busyModes & kDISPLAY) != 0) {
    state |= ES_DISPLAY_REQUIRED;
  }
  if (state != 0) {
    state |= ES_CONTINUOUS;
  }

  // do it
  s_stes(state);
}

DWORD
ArchMiscWindows::dummySetThreadExecutionState(DWORD)
{
  // do nothing
  return 0;
}

void ArchMiscWindows::wakeupDisplay()
{
  // We can't use ::setThreadExecutionState here because it sets
  // ES_CONTINUOUS, which we don't want.

  if (s_stes == nullptr) {
    HINSTANCE kernel = LoadLibrary(L"kernel32.dll");
    if (kernel != nullptr) {
      s_stes = reinterpret_cast<STES_t>(GetProcAddress(kernel, "SetThreadExecutionState"));
    }
    if (s_stes == nullptr) {
      s_stes = &ArchMiscWindows::dummySetThreadExecutionState;
    }
  }

  s_stes(ES_DISPLAY_REQUIRED);

  // restore the original execution states
  setThreadExecutionState(s_busyState);
}

bool ArchMiscWindows::wasLaunchedAsService()
{
  std::wstring name;
  if (!getParentProcessName(name)) {
    LOG_ERR("cannot determine if process was launched as service");
    return false;
  }

  return (name == L"services.exe");
}

bool ArchMiscWindows::getParentProcessName(std::wstring &name)
{
  PROCESSENTRY32 parentEntry;
  if (!getParentProcessEntry(parentEntry)) {
    LOG_ERR("could not get entry for parent process");
    return false;
  }

  name = parentEntry.szExeFile;
  return true;
}

BOOL WINAPI ArchMiscWindows::getSelfProcessEntry(PROCESSENTRY32 &entry)
{
  // get entry from current PID
  return getProcessEntry(entry, GetCurrentProcessId());
}

BOOL WINAPI ArchMiscWindows::getParentProcessEntry(PROCESSENTRY32 &entry)
{
  // get the current process, so we can get parent PID
  PROCESSENTRY32 selfEntry;
  if (!getSelfProcessEntry(selfEntry)) {
    return FALSE;
  }

  // get entry from parent PID
  return getProcessEntry(entry, selfEntry.th32ParentProcessID);
}

BOOL WINAPI ArchMiscWindows::getProcessEntry(PROCESSENTRY32 &entry, DWORD processID)
{
  // first we need to take a snapshot of the running processes
  HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  if (snapshot == INVALID_HANDLE_VALUE) {
    LOG_ERR("could not get process snapshot (error: %i)", GetLastError());
    return FALSE;
  }

  entry.dwSize = sizeof(PROCESSENTRY32);

  // get the first process, and if we can't do that then it's
  // unlikely we can go any further
  BOOL gotEntry = Process32First(snapshot, &entry);
  if (!gotEntry) {
    LOG_ERR("could not get first process entry (error: %i)", GetLastError());
    return FALSE;
  }

  while (gotEntry) {

    if (entry.th32ProcessID == processID) {
      // found current process
      return TRUE;
    }

    // now move on to the next entry (when we reach end, loop will stop)
    gotEntry = Process32Next(snapshot, &entry);
  }

  return FALSE;
}

HINSTANCE
ArchMiscWindows::instanceWin32()
{
  assert(s_instanceWin32 != nullptr);
  return s_instanceWin32;
}

void ArchMiscWindows::setInstanceWin32(HINSTANCE instance)
{
  assert(instance != nullptr);
  s_instanceWin32 = instance;
}

std::wstring ArchMiscWindows::getActiveDesktopName()
{
  HDESK desk = OpenInputDesktop(0, TRUE, GENERIC_READ);
  if (desk == nullptr) {
    LOG_ERR("could not open input desktop");
    throw std::runtime_error(windowsErrorToString(GetLastError()));
  }

  DWORD size;
  GetUserObjectInformation(desk, UOI_NAME, nullptr, 0, &size);
  auto *name = (TCHAR *)alloca(size + sizeof(TCHAR));
  GetUserObjectInformation(desk, UOI_NAME, name, size, &size);
  CloseDesktop(desk);
  return name;
}

HMODULE ArchMiscWindows::findLoadedModule(std::array<const char *, 2> moduleNames)
{
  std::array<HMODULE, 1024> hModules;
  DWORD cbNeeded;

  HANDLE hProcess = GetCurrentProcess();
  if (!EnumProcessModules(hProcess, hModules.data(), sizeof(hModules), &cbNeeded)) {
    errorMessageBox("Failed to enumerate process modules.");
    abort();
  }

  std::string loadedModuleName;
  for (size_t i = 0; i < (cbNeeded / sizeof(HMODULE)); ++i) {
    if (!GetModuleBaseNameA(hProcess, hModules[i], loadedModuleName.data(), sizeof(loadedModuleName))) {
      LOG_WARN("could not get base name of loaded module %d", i);
      continue;
    }

    for (const auto &moduleName : moduleNames) {
      if (_stricmp((loadedModuleName.data()), moduleName) == 0) {
        return hModules[i];
      }
    }
  }

  return nullptr;
}

// Enforcing minimum MSVC runtime version is quite strict, but we have a good reason.
//
// Microsoft lets you run a program with an older version of the runtime DLL than the one it was
// compiled with. This is because the runtime DLLs are supposedly ABI-compatible when the major
// version is the same, so hypothetically MSVC runtime 14.0 is compatible with 14.42.
// However, we have found subtle edge cases such as mutex lock causes access violation.
//
// Example of how Microsoft breaks ABI compatibility between minor runtime versions:
// https://stackoverflow.com/questions/69990339/why-is-stdmutex-so-much-worse-than-stdshared-mutex-in-visual-c
//
// Our CI is set up to build with the latest compiler, so in this case we should insist on at least
// the runtime DLL that was released at the same time as the compiler.
//
// As a developer convenience, we also allow builds on older compilers such as the minimum
// requirements for the current Qt version we support.
void ArchMiscWindows::guardRuntimeVersion() // NOSONAR - `noreturn` is not available
{
  auto hModule = findLoadedModule({"vcruntime140.dll", "vcruntime140d.dll"});
  if (hModule == nullptr) {
    errorMessageBox("Failed to find MSVC runtime DLL.");
    abort();
  }

  MS_LOG_DEBUG("found msvc runtime dll, handle: %p", hModule);

  std::array<char, MAX_PATH> pathBuffer;
  const auto path = pathBuffer.data();
  if (!GetModuleFileNameA(hModule, path, MAX_PATH)) {
    errorMessageBox("Failed to get path of MSVC runtime.");
    abort();
  }

  MS_LOG_DEBUG("msvc runtime dll path: %s", path);

  DWORD handle;
  DWORD size = GetFileVersionInfoSizeA(path, &handle);
  if (size <= 0) {
    errorMessageBox("Failed to get version info size for MSVC runtime.");
    abort();
  }

  MS_LOG_DEBUG("msvc runtime dll version info size: %d", size);

  std::vector<BYTE> versionInfo(size);
  if (!GetFileVersionInfoA(path, handle, size, versionInfo.data())) {
    errorMessageBox("Failed to get file version info for MSVC runtime.");
    abort();
  }

  MS_LOG_DEBUG("msvc runtime dll version info ok, querying values");

  VS_FIXEDFILEINFO *fileInfo = nullptr;
  const auto lplpFileInfo = reinterpret_cast<void **>(&fileInfo); // NOSONAR - Idiomatic Win32
  if (UINT len = 0; !VerQueryValueA(versionInfo.data(), "\\", lplpFileInfo, &len)) {
    errorMessageBox("Failed to query file version info for MSVC runtime.");
    abort();
  }

  const auto currentMajor = HIWORD(fileInfo->dwFileVersionMS);
  const auto currentMinor = LOWORD(fileInfo->dwFileVersionMS);
  const auto currentBuild = HIWORD(fileInfo->dwFileVersionLS);

  MS_LOG_DEBUG("msvc runtime dll version: %d.%d.%d", currentMajor, currentMinor, currentBuild);

  if (currentMajor < kWindowsRuntimeMajor || currentMinor < kWindowsRuntimeMinor) {
    const auto message = deskflow::string::sprintf(
        "Installed Microsoft Visual C++ Runtime v%d.%d.%d is outdated.\n\n"
        "Minimum required version: v%d.%d\n\n"
        "Please update to the latest Microsoft Visual C++ Redistributable.",
        currentMajor, currentMinor, currentBuild, kWindowsRuntimeMajor, kWindowsRuntimeMinor
    );
    MessageBoxA(nullptr, message.c_str(), "Dependency Error", MB_ICONERROR | MB_OK);
    exit(1);
  }
}

bool ArchMiscWindows::isProcessElevated()
{
  LOG_DEBUG("checking if process is elevated");

  HANDLE hToken = nullptr;
  if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
    throw std::runtime_error(windowsErrorToString(GetLastError()));
  }

  TOKEN_ELEVATION elevation;

  try {
    DWORD dwSize = sizeof(TOKEN_ELEVATION);
    if (!GetTokenInformation(hToken, TokenElevation, &elevation, sizeof(elevation), &dwSize)) {
      throw std::runtime_error(windowsErrorToString(GetLastError()));
    }
  } catch (...) {
    CloseHandle(hToken);
    throw;
  }

  const auto isElevated = elevation.TokenIsElevated;
  LOG_DEBUG("process is %s", isElevated ? "elevated" : "not elevated");
  return isElevated;
}
