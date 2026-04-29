/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "wix-custom.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

// Include after Windows.h
#include <MsiQuery.h>
#include <TlHelp32.h>

#include <chrono>
#include <filesystem>
#include <stdio.h>
#include <string>
#include <thread>
#include <vector>

namespace {
// Warning: DLL will crash with error code 1603 if we exceed this.
const auto kLogLineMax = 1024;

// Prefixes log messages with the app name so they're easier to find/filter.
const std::string kLogPrefix = std::string(kAppId) + " installer: ";

// Note: Resized to log line max when used.
static std::string s_logMessageBuffer; // NOSONAR - Must be mutable.

const auto kServiceStopTimeout = std::chrono::seconds(30);
const auto kFileDeleteRetries = 10;
const auto kDeleteRetryDelay = std::chrono::milliseconds(250);

std::string
getMsiPropertyA(const MSIHANDLE hInstall, const char *name)
{
  DWORD size = 0;
  char empty[1] = {'\0'};
  auto result = MsiGetPropertyA(hInstall, name, empty, &size);
  if (result != ERROR_SUCCESS && result != ERROR_MORE_DATA) {
    return {};
  }

  std::string value(size + 1, '\0');
  result = MsiGetPropertyA(hInstall, name, value.data(), &size);
  if (result != ERROR_SUCCESS) {
    return {};
  }

  value.resize(size);
  return value;
}

std::vector<std::pair<std::string, std::string>>
parseCustomActionData(const std::string &data)
{
  std::vector<std::pair<std::string, std::string>> kvPairs;
  size_t start = 0;
  while (start < data.size()) {
    const auto end = data.find(';', start);
    const auto pairText =
        data.substr(start, (end == std::string::npos) ? std::string::npos : (end - start));

    const auto separator = pairText.find('=');
    if (separator != std::string::npos && separator > 0) {
      kvPairs.emplace_back(pairText.substr(0, separator), pairText.substr(separator + 1));
    }

    if (end == std::string::npos) {
      break;
    }
    start = end + 1;
  }

  return kvPairs;
}

std::string
findValue(const std::vector<std::pair<std::string, std::string>> &kvPairs, const std::string &key)
{
  for (const auto &[k, v] : kvPairs) {
    if (k == key) {
      return v;
    }
  }
  return {};
}

std::wstring
toWide(const std::string &text)
{
  if (text.empty()) {
    return {};
  }

  const auto size = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
  if (size <= 0) {
    return {};
  }

  std::wstring wide(size, L'\0');
  MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, wide.data(), size);
  if (!wide.empty() && wide.back() == L'\0') {
    wide.pop_back();
  }
  return wide;
}

std::wstring
joinPath(std::wstring base, const std::wstring &leaf)
{
  if (base.empty()) {
    return {};
  }

  while (!base.empty() && (base.back() == L'\\' || base.back() == L'/')) {
    base.pop_back();
  }

  if (base.empty()) {
    return {};
  }

  return base + L"\\" + leaf;
}

bool
killProcessByName(const wchar_t *processName)
{
  bool terminatedAny = false;
  HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  if (snapshot == INVALID_HANDLE_VALUE) {
    return false;
  }

  PROCESSENTRY32W entry{};
  entry.dwSize = sizeof(entry);
  if (!Process32FirstW(snapshot, &entry)) {
    CloseHandle(snapshot);
    return false;
  }

  do {
    if (_wcsicmp(entry.szExeFile, processName) != 0) {
      continue;
    }

    const auto currentPid = GetCurrentProcessId();
    if (entry.th32ProcessID == 0 || entry.th32ProcessID == currentPid) {
      continue;
    }

    HANDLE process =
        OpenProcess(PROCESS_TERMINATE | PROCESS_QUERY_LIMITED_INFORMATION | SYNCHRONIZE, FALSE, entry.th32ProcessID);
    if (process == nullptr) {
      continue;
    }

    if (TerminateProcess(process, 0)) {
      WaitForSingleObject(process, 3000);
      terminatedAny = true;
    }
    CloseHandle(process);
  } while (Process32NextW(snapshot, &entry));

  CloseHandle(snapshot);
  return terminatedAny;
}

bool
stopAndDeleteService(const wchar_t *serviceName)
{
  SC_HANDLE scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
  if (scm == nullptr) {
    return false;
  }

  SC_HANDLE service = OpenServiceW(
      scm, serviceName, SERVICE_STOP | SERVICE_QUERY_STATUS | DELETE
  );
  if (service == nullptr) {
    CloseServiceHandle(scm);
    return false;
  }

  SERVICE_STATUS_PROCESS status{};
  DWORD bytesNeeded = 0;
  const auto queryOk = QueryServiceStatusEx(
      service, SC_STATUS_PROCESS_INFO, reinterpret_cast<LPBYTE>(&status), sizeof(status), &bytesNeeded
  );

  if (queryOk && status.dwCurrentState != SERVICE_STOPPED && status.dwCurrentState != SERVICE_STOP_PENDING) {
    SERVICE_STATUS unused{};
    ControlService(service, SERVICE_CONTROL_STOP, &unused);
  }

  const auto deadline = std::chrono::steady_clock::now() + kServiceStopTimeout;
  while (std::chrono::steady_clock::now() < deadline) {
    bytesNeeded = 0;
    if (!QueryServiceStatusEx(
            service, SC_STATUS_PROCESS_INFO, reinterpret_cast<LPBYTE>(&status), sizeof(status), &bytesNeeded
        )) {
      break;
    }

    if (status.dwCurrentState == SERVICE_STOPPED) {
      break;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(250));
  }

  const auto deleted = (DeleteService(service) == TRUE);
  CloseServiceHandle(service);
  CloseServiceHandle(scm);
  return deleted;
}

void
deletePathWithRetries(const std::wstring &path)
{
  if (path.empty()) {
    return;
  }

  std::error_code ec;
  for (int i = 0; i < kFileDeleteRetries; ++i) {
    if (!std::filesystem::exists(path, ec)) {
      return;
    }

    ec.clear();
    std::filesystem::remove_all(path, ec);
    if (!ec) {
      return;
    }

    std::this_thread::sleep_for(kDeleteRetryDelay);
  }
}

void
deleteFileWithRetries(const std::wstring &path)
{
  if (path.empty()) {
    return;
  }

  std::error_code ec;
  for (int i = 0; i < kFileDeleteRetries; ++i) {
    if (!std::filesystem::exists(path, ec)) {
      return;
    }

    ec.clear();
    std::filesystem::remove(path, ec);
    if (!ec) {
      return;
    }

    std::this_thread::sleep_for(kDeleteRetryDelay);
  }
}
} // namespace

// This log output can be viewed by using the DebugView program.
#define MS_LOG_DEBUG(message, ...)                                                                                     \
  s_logMessageBuffer.resize(kLogLineMax);                                                                              \
  sprintf(s_logMessageBuffer.data(), message, __VA_ARGS__);                                                            \
  OutputDebugStringA((kLogPrefix + s_logMessageBuffer + "\n").c_str())

extern "C" __declspec(dllexport) UINT __stdcall CheckVCRedist(MSIHANDLE hInstall)
{
  const auto kKeyName = kRegKey;
  const auto kValueName = "Minor";
  const auto kProperty = "VC_REDIST_VERSION_OK";

  MS_LOG_DEBUG("checking for msvc redist v%d.%d", kWindowsRuntimeMajor, kWindowsRuntimeMinor);

  HKEY hKey;
  if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, kKeyName, 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
    MS_LOG_DEBUG("msvc redist registry key not found");
    return ERROR_FUNCTION_FAILED;
  }

  MS_LOG_DEBUG("msvc redist registry key found, querying minor version");

  DWORD minorVersion = 0;
  DWORD size = sizeof(DWORD);
  RegQueryValueExA(hKey, kValueName, nullptr, nullptr, reinterpret_cast<LPBYTE>(&minorVersion), &size);
  RegCloseKey(hKey);

  MS_LOG_DEBUG("msvc redist minor version: %lu", minorVersion);

  if (minorVersion < kWindowsRuntimeMinor) {
    MS_LOG_DEBUG("msvc redist minor version %lu too low, expected >= %d", minorVersion, kWindowsRuntimeMinor);
    // Returning success allows the installer will show a friendly error message.
    return ERROR_SUCCESS;
  }

  MS_LOG_DEBUG("msvc redist version ok, setting: %s", kProperty);
  if (MsiSetPropertyA(hInstall, kProperty, "ok") != ERROR_SUCCESS) {
    MS_LOG_DEBUG("failed to set property: %s", kProperty);
    return ERROR_FUNCTION_FAILED;
  }

  MS_LOG_DEBUG("msvc redist version check successful");
  return ERROR_SUCCESS;
}

extern "C" __declspec(dllexport) UINT __stdcall StopAndRemoveService(MSIHANDLE hInstall)
{
  const auto data = getMsiPropertyA(hInstall, "CustomActionData");
  const auto kv = parseCustomActionData(data);
  const auto serviceName = toWide(findValue(kv, "SERVICE_NAME"));

  if (serviceName.empty()) {
    MS_LOG_DEBUG("%s", "service cleanup skipped: missing service name");
    return ERROR_SUCCESS;
  }

  MS_LOG_DEBUG("stopping/deleting service: %ls", serviceName.c_str());

  const auto daemonKilled = killProcessByName(L"deskflow-daemon.exe");
  const auto coreKilled = killProcessByName(L"deskflow-core.exe");
  const auto guiKilled = killProcessByName(L"deskflow.exe");
  MS_LOG_DEBUG(
      "process cleanup done: daemon=%d core=%d gui=%d", daemonKilled ? 1 : 0, coreKilled ? 1 : 0, guiKilled ? 1 : 0
  );

  const auto deleted = stopAndDeleteService(serviceName.c_str());
  MS_LOG_DEBUG("service delete result: %d", deleted ? 1 : 0);

  return ERROR_SUCCESS;
}

extern "C" __declspec(dllexport) UINT __stdcall CleanupInstalledArtifacts(MSIHANDLE hInstall)
{
  const auto data = getMsiPropertyA(hInstall, "CustomActionData");
  const auto kv = parseCustomActionData(data);

  const auto installRoot = toWide(findValue(kv, "INSTALL_ROOT"));
  const auto commonAppData = toWide(findValue(kv, "COMMON_APPDATA"));
  const auto localAppData = toWide(findValue(kv, "LOCAL_APPDATA"));
  const auto roamingAppData = toWide(findValue(kv, "APPDATA"));

  const std::wstring appName = L"Deskflow";
  const std::wstring appId = toWide(std::string(kAppId));

  if (!installRoot.empty()) {
    MS_LOG_DEBUG("cleanup uninstall path: %ls", installRoot.c_str());
    deletePathWithRetries(installRoot);
  }

  for (const auto &base : {commonAppData, localAppData, roamingAppData}) {
    if (base.empty()) {
      continue;
    }

    deletePathWithRetries(joinPath(base, appName));
    deletePathWithRetries(joinPath(base, appId));
    deleteFileWithRetries(joinPath(base, appName + L".state"));
    deleteFileWithRetries(joinPath(base, appId + L".state"));
  }

  return ERROR_SUCCESS;
}
