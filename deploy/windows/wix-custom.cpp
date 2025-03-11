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

#include <stdio.h>
#include <string>

namespace {
// Warning: DLL will crash with error code 1603 if we exceed this.
const auto kLogLineMax = 1024;

// Prefixes log messages with the app name so they're easier to find/filter.
const std::string kLogPrefix = std::string(kAppId) + " installer: ";

// Note: Resized to log line max when used.
static std::string s_logMessageBuffer; // NOSONAR - Must be mutable.
} // namespace

// This log output can be viewed by using the DebugView program.
#define MS_LOG_DEBUG(message, ...)                                                                                     \
  s_logMessageBuffer.resize(kLogLineMax);                                                                              \
  sprintf(s_logMessageBuffer.data(), message, __VA_ARGS__);                                                            \
  OutputDebugStringA((kLogPrefix + s_logMessageBuffer + "\n").c_str())

extern "C" __declspec(dllexport) UINT __stdcall CheckVCRedist(MSIHANDLE hInstall)
{
  const auto kKeyName = TEXT("SOFTWARE\\Microsoft\\VisualStudio\\14.0\\VC\\Runtimes\\x64");
  const auto kValueName = TEXT("Minor");
  const auto kProperty = "VC_REDIST_VERSION_OK";

  MS_LOG_DEBUG("checking for msvc redist v%d.%d", kWindowsRuntimeMajor, kWindowsRuntimeMinor);

  HKEY hKey;
  if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, kKeyName, 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
    MS_LOG_DEBUG("msvc redist registry key not found");
    return ERROR_FUNCTION_FAILED;
  }

  MS_LOG_DEBUG("msvc redist registry key found, querying minor version");

  DWORD minorVersion = 0;
  DWORD size = sizeof(DWORD);
  RegQueryValueEx(hKey, kValueName, nullptr, nullptr, (LPBYTE)&minorVersion, &size);
  RegCloseKey(hKey);

  MS_LOG_DEBUG("msvc redist minor version: %lu", minorVersion);

  if (minorVersion < kWindowsRuntimeMinor) {
    MS_LOG_DEBUG("msvc redist minor version %lu too low, expected >= %d", minorVersion, kWindowsRuntimeMinor);
    // Returning success allows the installer will show a friendly error message.
    return ERROR_SUCCESS;
  }

  MS_LOG_DEBUG("msvc redist version ok, setting: %s", kProperty);
  if (MsiSetProperty(hInstall, kProperty, "ok") != ERROR_SUCCESS) {
    MS_LOG_DEBUG("failed to set property: %s", kProperty);
    return ERROR_FUNCTION_FAILED;
  }

  MS_LOG_DEBUG("msvc redist version check successful");
  return ERROR_SUCCESS;
}
