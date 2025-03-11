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

#include <Psapi.h>

#include <array>
#include <filesystem>

// See table of the compiler versions and the matching runtime DLL versions:
// https://dev.to/yumetodo/list-of-mscver-and-mscfullver-8nd
#if _MSC_VER >= 1942 // Visual Studio 2022 Update 12 (v17.12.4)
const auto kRequiredMajor = 14;
const auto kRequiredMinor = 42;
#elif _MSC_VER >= 1920 // Visual Studio 2019 Update 7 (v16.7)
const auto kRequiredMajor = 14;
const auto kRequiredMinor = 27;
#else
#pragma message("MSC version: " STRINGIFY(_MSC_VER))
#error "Unsupported MSC version"
#endif

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
  if (!GetModuleFileNameA(NULL, buffer.data(), MAX_PATH)) {
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

ArchMiscWindows::Dialogs *ArchMiscWindows::s_dialogs = NULL;
DWORD ArchMiscWindows::s_busyState = 0;
ArchMiscWindows::STES_t ArchMiscWindows::s_stes = NULL;
HICON ArchMiscWindows::s_largeIcon = NULL;
HICON ArchMiscWindows::s_smallIcon = NULL;
HINSTANCE ArchMiscWindows::s_instanceWin32 = NULL;

void ArchMiscWindows::cleanup()
{
  delete s_dialogs;
}

void ArchMiscWindows::init()
{
  // stop windows system error dialogs from showing.
  SetErrorMode(SEM_FAILCRITICALERRORS);

  s_dialogs = new Dialogs;
}

void ArchMiscWindows::setIcons(HICON largeIcon, HICON smallIcon)
{
  s_largeIcon = largeIcon;
  s_smallIcon = smallIcon;
}

void ArchMiscWindows::getIcons(HICON &largeIcon, HICON &smallIcon)
{
  largeIcon = s_largeIcon;
  smallIcon = s_smallIcon;
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
  // ignore if parent is NULL
  if (key == NULL) {
    return NULL;
  }

  // open next key
  HKEY newKey;
  LSTATUS result = RegOpenKeyEx(key, keyName, 0, KEY_WRITE | KEY_QUERY_VALUE, &newKey);
  if (result != ERROR_SUCCESS && create) {
    DWORD disp;
    result = RegCreateKeyEx(key, keyName, 0, NULL, 0, KEY_WRITE | KEY_QUERY_VALUE, NULL, &newKey, &disp);
  }
  if (result != ERROR_SUCCESS) {
    RegCloseKey(key);
    return NULL;
  }

  // switch to new key
  RegCloseKey(key);
  return newKey;
}

HKEY ArchMiscWindows::openKey(HKEY key, const TCHAR *const *keyNames, bool create)
{
  for (size_t i = 0; key != NULL && keyNames[i] != NULL; ++i) {
    // open next key
    key = openKey(key, keyNames[i], create);
  }
  return key;
}

void ArchMiscWindows::closeKey(HKEY key)
{
  assert(key != NULL);
  if (key == NULL)
    return;
  RegCloseKey(key);
}

void ArchMiscWindows::deleteKey(HKEY key, const TCHAR *name)
{
  assert(key != NULL);
  assert(name != NULL);
  if (key == NULL || name == NULL)
    return;
  RegDeleteKey(key, name);
}

void ArchMiscWindows::deleteValue(HKEY key, const TCHAR *name)
{
  assert(key != NULL);
  assert(name != NULL);
  if (key == NULL || name == NULL)
    return;
  RegDeleteValue(key, name);
}

void ArchMiscWindows::deleteKeyTree(HKEY key, const TCHAR *name)
{
  assert(key != NULL);
  assert(name != NULL);
  if (key == NULL || name == NULL)
    return;
  RegDeleteTree(key, name);
}

bool ArchMiscWindows::hasValue(HKEY key, const TCHAR *name)
{
  DWORD type;
  LONG result = RegQueryValueEx(key, name, 0, &type, NULL, NULL);
  return (result == ERROR_SUCCESS && (type == REG_DWORD || type == REG_SZ));
}

ArchMiscWindows::EValueType ArchMiscWindows::typeOfValue(HKEY key, const TCHAR *name)
{
  DWORD type;
  LONG result = RegQueryValueEx(key, name, 0, &type, NULL, NULL);
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
  assert(key != NULL);
  if (key == NULL) {
    // TODO: throw exception
    return;
  }
  RegSetValueEx(key, name, 0, REG_SZ, reinterpret_cast<const BYTE *>(value.c_str()), (DWORD)value.size() + 1);
}

void ArchMiscWindows::setValue(HKEY key, const TCHAR *name, DWORD value)
{
  assert(key != NULL);
  if (key == NULL) {
    // TODO: throw exception
    return;
  }
  RegSetValueEx(key, name, 0, REG_DWORD, reinterpret_cast<CONST BYTE *>(&value), sizeof(DWORD));
}

void ArchMiscWindows::setValueBinary(HKEY key, const TCHAR *name, const std::string &value)
{
  assert(key != NULL);
  assert(name != NULL);
  if (key == NULL || name == NULL) {
    // TODO: throw exception
    return;
  }
  RegSetValueEx(key, name, 0, REG_BINARY, reinterpret_cast<const BYTE *>(value.data()), (DWORD)value.size());
}

std::string ArchMiscWindows::readBinaryOrString(HKEY key, const TCHAR *name, DWORD type)
{
  // get the size of the string
  DWORD actualType;
  DWORD size = 0;
  LONG result = RegQueryValueEx(key, name, 0, &actualType, NULL, &size);
  if (result != ERROR_SUCCESS || actualType != type) {
    return std::string();
  }

  // if zero size then return empty string
  if (size == 0) {
    return std::string();
  }

  // allocate space
  char *buffer = new char[size];

  // read it
  result = RegQueryValueEx(key, name, 0, &actualType, reinterpret_cast<BYTE *>(buffer), &size);
  if (result != ERROR_SUCCESS || actualType != type) {
    delete[] buffer;
    return std::string();
  }

  // clean up and return value
  if (type == REG_SZ && buffer[size - 1] == '\0') {
    // don't include terminating nul;  std::string will add one.
    --size;
  }
  std::string value(buffer, size);
  delete[] buffer;
  return value;
}

std::string ArchMiscWindows::readValueString(HKEY key, const TCHAR *name)
{
  return readBinaryOrString(key, name, REG_SZ);
}

std::string ArchMiscWindows::readValueBinary(HKEY key, const TCHAR *name)
{
  return readBinaryOrString(key, name, REG_BINARY);
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

void ArchMiscWindows::addDialog(HWND hwnd)
{
  s_dialogs->insert(hwnd);
}

void ArchMiscWindows::removeDialog(HWND hwnd)
{
  s_dialogs->erase(hwnd);
}

bool ArchMiscWindows::processDialog(MSG *msg)
{
  for (Dialogs::const_iterator index = s_dialogs->begin(); index != s_dialogs->end(); ++index) {
    if (IsDialogMessage(*index, msg)) {
      return true;
    }
  }
  return false;
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
  if (s_stes == NULL) {
    HINSTANCE kernel = LoadLibrary("kernel32.dll");
    if (kernel != NULL) {
      s_stes = reinterpret_cast<STES_t>(GetProcAddress(kernel, "SetThreadExecutionState"));
    }
    if (s_stes == NULL) {
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

  if (s_stes == NULL) {
    HINSTANCE kernel = LoadLibrary("kernel32.dll");
    if (kernel != NULL) {
      s_stes = reinterpret_cast<STES_t>(GetProcAddress(kernel, "SetThreadExecutionState"));
    }
    if (s_stes == NULL) {
      s_stes = &ArchMiscWindows::dummySetThreadExecutionState;
    }
  }

  s_stes(ES_DISPLAY_REQUIRED);

  // restore the original execution states
  setThreadExecutionState(s_busyState);
}

bool ArchMiscWindows::wasLaunchedAsService()
{
  std::string name;
  if (!getParentProcessName(name)) {
    LOG((CLOG_ERR "cannot determine if process was launched as service"));
    return false;
  }

  return (name == "services.exe");
}

bool ArchMiscWindows::getParentProcessName(std::string &name)
{
  PROCESSENTRY32 parentEntry;
  if (!getParentProcessEntry(parentEntry)) {
    LOG((CLOG_ERR "could not get entry for parent process"));
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
    LOG((CLOG_ERR "could not get process snapshot (error: %i)", GetLastError()));
    return FALSE;
  }

  entry.dwSize = sizeof(PROCESSENTRY32);

  // get the first process, and if we can't do that then it's
  // unlikely we can go any further
  BOOL gotEntry = Process32First(snapshot, &entry);
  if (!gotEntry) {
    LOG((CLOG_ERR "could not get first process entry (error: %i)", GetLastError()));
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
  assert(s_instanceWin32 != NULL);
  return s_instanceWin32;
}

void ArchMiscWindows::setInstanceWin32(HINSTANCE instance)
{
  assert(instance != NULL);
  s_instanceWin32 = instance;
}

std::string ArchMiscWindows::getActiveDesktopName()
{
  HDESK desk = OpenInputDesktop(0, TRUE, GENERIC_READ);
  if (desk == nullptr) {
    LOG((CLOG_ERR "could not open input desktop"));
    throw XArch(new XArchEvalWindows());
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
      if (_stricmp(loadedModuleName.data(), moduleName) == 0) {
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

  if (currentMajor < kRequiredMajor || currentMinor < kRequiredMinor) {
    const auto message = deskflow::string::sprintf(
        "Installed Microsoft Visual C++ Runtime v%d.%d.%d is outdated.\n\n"
        "Minimum required version: v%d.%d\n\n"
        "Please update to the latest Microsoft Visual C++ Redistributable.",
        currentMajor, currentMinor, currentBuild, kRequiredMajor, kRequiredMinor
    );
    MessageBoxA(nullptr, message.c_str(), "Dependency Error", MB_ICONERROR | MB_OK);
    exit(1);
  }
}
