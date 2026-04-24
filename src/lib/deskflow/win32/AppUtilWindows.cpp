/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 - 2025 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "deskflow/win32/AppUtilWindows.h"

#include "arch/Arch.h"
#include "arch/win32/ArchDaemonWindows.h"
#include "arch/win32/ArchMiscWindows.h"
#include "arch/win32/XArchWindows.h"
#include "base/Event.h"
#include "base/IEventQueue.h"
#include "base/Log.h"
#include "base/LogOutputters.h"
#include "common/Constants.h"
#include "deskflow/App.h"
#include "deskflow/DeskflowException.h"
#include "deskflow/Screen.h"
#include "mt/Thread.h"
#include "platform/MSWindowsScreen.h"

#include <Windows.h>
#include <conio.h>
#include <string>

#include <algorithm>
#include <cwchar>
#include <cwctype>
#include <iomanip>
#include <optional>
#include <set>
#include <sstream>
#include <string_view>

namespace {

std::string wideCharToUtf8(const std::wstring &value)
{
  if (value.empty()) {
    return {};
  }

  const auto size = WideCharToMultiByte(CP_UTF8, 0, value.c_str(), -1, nullptr, 0, nullptr, nullptr);
  if (size <= 1) {
    return {};
  }

  std::string result(static_cast<size_t>(size), '\0');
  if (WideCharToMultiByte(CP_UTF8, 0, value.c_str(), -1, result.data(), size, nullptr, nullptr) == 0) {
    return {};
  }

  result.resize(static_cast<size_t>(size - 1));
  return result;
}

std::wstring utf8ToWideChar(const std::string_view value)
{
  if (value.empty()) {
    return {};
  }

  const auto size =
      MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0);
  if (size <= 0) {
    return {};
  }

  std::wstring result(static_cast<size_t>(size), L'\0');
  if (MultiByteToWideChar(
          CP_UTF8, 0, value.data(), static_cast<int>(value.size()), result.data(), size
      ) == 0) {
    return {};
  }

  return result;
}

std::wstring localeNameFromLcid(LCID lcid)
{
  const auto size = LCIDToLocaleName(lcid, nullptr, 0, 0);
  if (size <= 1) {
    return {};
  }

  std::wstring buffer(static_cast<size_t>(size), L'\0');
  if (LCIDToLocaleName(lcid, buffer.data(), size, 0) == 0) {
    return {};
  }

  buffer.resize(wcslen(buffer.c_str()));
  return buffer;
}

bool looksLikeKlid(const std::wstring_view layoutId)
{
  return layoutId.size() == 8 &&
         std::all_of(layoutId.begin(), layoutId.end(), [](wchar_t c) { return std::iswxdigit(c) != 0; });
}

std::wstring toUpper(const std::wstring &value)
{
  std::wstring result = value;
  std::transform(result.begin(), result.end(), result.begin(), [](wchar_t c) {
    return static_cast<wchar_t>(std::towupper(c));
  });
  return result;
}

std::optional<std::wstring> queryRegistryString(HKEY root, const std::wstring &keyPath, const wchar_t *valueName)
{
  HKEY key = nullptr;
  if (RegOpenKeyExW(root, keyPath.c_str(), 0, KEY_READ, &key) != ERROR_SUCCESS) {
    return std::nullopt;
  }

  DWORD type = 0;
  DWORD bytes = 0;
  const auto queryResult = RegQueryValueExW(key, valueName, nullptr, &type, nullptr, &bytes);
  if (queryResult != ERROR_SUCCESS || (type != REG_SZ && type != REG_EXPAND_SZ) || bytes < sizeof(wchar_t)) {
    RegCloseKey(key);
    return std::nullopt;
  }

  std::wstring buffer(bytes / sizeof(wchar_t), L'\0');
  if (RegQueryValueExW(
          key, valueName, nullptr, &type, reinterpret_cast<LPBYTE>(buffer.data()), &bytes
      ) != ERROR_SUCCESS) {
    RegCloseKey(key);
    return std::nullopt;
  }

  RegCloseKey(key);
  buffer.resize(wcslen(buffer.c_str()));
  return buffer;
}

std::wstring resolveSubstitutedLayoutId(const std::wstring &layoutId)
{
  if (!looksLikeKlid(layoutId)) {
    return {};
  }

  const auto upperLayoutId = toUpper(layoutId);
  const auto substitute =
      queryRegistryString(HKEY_CURRENT_USER, L"Keyboard Layout\\Substitutes", upperLayoutId.c_str());

  if (substitute && looksLikeKlid(*substitute)) {
    return toUpper(*substitute);
  }

  return upperLayoutId;
}

std::vector<std::string> getPreloadedKeyboardLayouts()
{
  std::vector<std::pair<int, std::string>> orderedLayouts;

  HKEY key = nullptr;
  if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Keyboard Layout\\Preload", 0, KEY_READ, &key) != ERROR_SUCCESS) {
    return {};
  }

  for (DWORD index = 0;; ++index) {
    wchar_t valueName[256] = {};
    DWORD valueNameSize = static_cast<DWORD>(sizeof(valueName) / sizeof(valueName[0]));

    wchar_t valueData[KL_NAMELENGTH] = {};
    DWORD valueDataBytes = sizeof(valueData);
    DWORD type = 0;

    const auto result = RegEnumValueW(
        key,
        index,
        valueName,
        &valueNameSize,
        nullptr,
        &type,
        reinterpret_cast<LPBYTE>(valueData),
        &valueDataBytes
    );

    if (result == ERROR_NO_MORE_ITEMS) {
      break;
    }

    if (result != ERROR_SUCCESS || type != REG_SZ) {
      continue;
    }

    std::wstring layoutId(valueData);
    layoutId = resolveSubstitutedLayoutId(layoutId);
    if (!looksLikeKlid(layoutId)) {
      continue;
    }

    long order = std::wcstol(valueName, nullptr, 10);
    if (order <= 0) {
      order = static_cast<long>(index) + 1000;
    }

    orderedLayouts.emplace_back(static_cast<int>(order), wideCharToUtf8(layoutId));
  }

  RegCloseKey(key);

  std::sort(orderedLayouts.begin(), orderedLayouts.end(), [](const auto &lhs, const auto &rhs) {
    return lhs.first < rhs.first;
  });

  std::vector<std::string> result;
  std::set<std::string> seen;
  for (const auto &[order, layoutId] : orderedLayouts) {
    (void)order;
    if (seen.insert(layoutId).second) {
      result.push_back(layoutId);
    }
  }

  return result;
}

std::string rawKeyboardLayoutId(HKL layout)
{
  std::ostringstream stream;
  stream << std::uppercase << std::hex << std::setw(8) << std::setfill('0')
         << (static_cast<unsigned long>(reinterpret_cast<UINT_PTR>(layout)) & 0xffffffffUL);
  return stream.str();
}

std::wstring getCurrentKeyboardLayoutName()
{
  GUITHREADINFO gti = {sizeof(GUITHREADINFO)};
  DWORD targetThreadId = GetCurrentThreadId();

  if (GetGUIThreadInfo(0, &gti) && gti.hwndActive) {
    targetThreadId = GetWindowThreadProcessId(gti.hwndActive, nullptr);
  }

  const DWORD currentThreadId = GetCurrentThreadId();
  const bool attach =
      targetThreadId != 0 && targetThreadId != currentThreadId &&
      AttachThreadInput(currentThreadId, targetThreadId, TRUE) != FALSE;

  wchar_t buffer[KL_NAMELENGTH] = {};
  const bool ok = GetKeyboardLayoutNameW(buffer) != 0;

  if (attach) {
    AttachThreadInput(currentThreadId, targetThreadId, FALSE);
  }

  if (!ok || buffer[0] == L'\0') {
    return {};
  }

  return resolveSubstitutedLayoutId(buffer);
}

} // namespace

AppUtilWindows::AppUtilWindows(IEventQueue *events) : m_events(events), m_exitMode(kExitModeNormal)
{
  if (SetConsoleCtrlHandler((PHANDLER_ROUTINE)consoleHandler, TRUE) == FALSE) {
    throw std::runtime_error(windowsErrorToString(GetLastError()));
  }

  m_eventThread = std::thread(&AppUtilWindows::eventLoop, this);

  LOG_DEBUG("waiting for event thread to start");
  std::unique_lock lock(m_eventThreadStartedMutex);
  m_eventThreadStartedCond.wait(lock, [this] { return m_eventThreadRunning; });
  LOG_DEBUG("event thread started");
}

AppUtilWindows::~AppUtilWindows()
{
  m_eventThreadRunning = false;
  m_eventThread.join();
}

BOOL WINAPI AppUtilWindows::consoleHandler(DWORD)
{
  LOG_INFO("got shutdown signal");
  IEventQueue *events = AppUtil::instance().app().getEvents();
  events->addEvent(Event(EventTypes::Quit));
  return TRUE;
}

static int mainLoopStatic()
{
  return AppUtil::instance().app().mainLoop();
}

int AppUtilWindows::daemonNTMainLoop()
{
  app().initApp();
  return ArchDaemonWindows::runDaemon(mainLoopStatic);
}

void AppUtilWindows::exitApp(int code)
{
  switch (m_exitMode) {
  case kExitModeDaemon:
    ArchDaemonWindows::daemonFailed(code);
    break;
  default:
    throw ExitAppException(code);
  }
}

int daemonNTMainLoopStatic()
{
  return AppUtilWindows::instance().daemonNTMainLoop();
}

int AppUtilWindows::daemonNTStartup()
{
  SystemLogger sysLogger(app().daemonName(), false);
  m_exitMode = kExitModeDaemon;
  return ARCH->daemonize(daemonNTMainLoopStatic);
}

static int daemonNTStartupStatic()
{
  return AppUtilWindows::instance().daemonNTStartup();
}

static int foregroundStartupStatic()
{
  return AppUtil::instance().app().start();
}

int AppUtilWindows::run()
{
  ArchMiscWindows::setInstanceWin32(GetModuleHandle(nullptr));
  MSWindowsScreen::init(ArchMiscWindows::instanceWin32());
  Thread::getCurrentThread().setPriority(-14);

  StartupFunc startup;
  if (ArchMiscWindows::wasLaunchedAsService()) {
    startup = &daemonNTStartupStatic;
  } else {
    startup = &foregroundStartupStatic;
  }

  return app().runInner(startup);
}

AppUtilWindows &AppUtilWindows::instance()
{
  return (AppUtilWindows &)AppUtil::instance();
}

void AppUtilWindows::startNode()
{
  app().startNode();
}


static std::string resolveKeyboardLayoutKlid(HKL layout)
{
  if (layout == nullptr) {
    return {};
  }

  const HKL previousLayout = ActivateKeyboardLayout(layout, 0);

  wchar_t klid[KL_NAMELENGTH] = {};
  if (!GetKeyboardLayoutNameW(klid)) {
    if (previousLayout != nullptr) {
      ActivateKeyboardLayout(previousLayout, 0);
    }
    return {};
  }

  if (previousLayout != nullptr) {
    ActivateKeyboardLayout(previousLayout, 0);
  }

  char result[KL_NAMELENGTH] = {};
  WideCharToMultiByte(CP_UTF8, 0, klid, -1, result, sizeof(result), nullptr, nullptr);
  return result;
}

std::vector<std::string> AppUtilWindows::getKeyboardLayoutList()
{
  std::vector<std::string> layouts;

  const auto activeLayout = resolveKeyboardLayoutKlid(getCurrentKeyboardLayout());
  if (!activeLayout.empty()) {
    layouts.push_back(activeLayout);
  }

  LOG_INFO("resolved active windows keyboard layout:");
  for (const auto &layout : layouts) {
    LOG_INFO("  - %s", layout.c_str());
  }

  return layouts;
}

std::string AppUtilWindows::getCurrentLanguageCode()
{
  const auto activeLayout = resolveKeyboardLayoutKlid(getCurrentKeyboardLayout());
  if (activeLayout.empty()) {
    LOG_WARN("failed to determine active Windows keyboard KLID");
  }
  return activeLayout;
}

HKL AppUtilWindows::getCurrentKeyboardLayout() const
{
  HKL layout = nullptr;

  GUITHREADINFO gti = {sizeof(GUITHREADINFO)};
  if (GetGUIThreadInfo(0, &gti) && gti.hwndActive) {
    layout = GetKeyboardLayout(GetWindowThreadProcessId(gti.hwndActive, nullptr));
  } else {
    LOG_WARN("failed to determine current keyboard layout");
  }

  return layout;
}

std::string AppUtilWindows::normalizeKeyboardLayoutId(std::string_view layoutId)
{
  const auto wideLayoutId = utf8ToWideChar(layoutId);
  if (!looksLikeKlid(wideLayoutId)) {
    return std::string(layoutId);
  }

  const auto resolved = resolveSubstitutedLayoutId(wideLayoutId);
  if (resolved.empty()) {
    return std::string(layoutId);
  }

  return wideCharToUtf8(resolved);
}

std::string AppUtilWindows::formatKeyboardLayoutForLog(std::string_view layoutId)
{
  const auto normalizedLayoutId = normalizeKeyboardLayoutId(layoutId);
  const auto localeTag = getLocaleTagFromLayoutId(normalizedLayoutId);
  const auto displayName = getLayoutDisplayName(normalizedLayoutId);

  std::string result = normalizedLayoutId;
  if (!localeTag.empty()) {
    result += " [" + localeTag + "]";
  }
  if (!displayName.empty()) {
    result += " ";
    result += displayName;
  }

  return result;
}

std::string AppUtilWindows::getLocaleTagFromLayoutId(std::string_view layoutId)
{
  const auto normalizedLayoutId = normalizeKeyboardLayoutId(layoutId);
  const auto wideLayoutId = utf8ToWideChar(normalizedLayoutId);
  if (!looksLikeKlid(wideLayoutId)) {
    return {};
  }

  const auto layoutValue = static_cast<unsigned long>(std::wcstoul(wideLayoutId.c_str(), nullptr, 16));
  const auto localeName = localeNameFromLcid(MAKELCID(static_cast<WORD>(layoutValue & 0xffffu), SORT_DEFAULT));
  return wideCharToUtf8(localeName);
}

std::string AppUtilWindows::getLayoutDisplayName(std::string_view layoutId)
{
  const auto normalizedLayoutId = normalizeKeyboardLayoutId(layoutId);
  const auto wideLayoutId = utf8ToWideChar(normalizedLayoutId);
  if (!looksLikeKlid(wideLayoutId)) {
    return {};
  }

  const auto keyPath = std::wstring(L"SYSTEM\\CurrentControlSet\\Control\\Keyboard Layouts\\") + wideLayoutId;

  if (const auto text = queryRegistryString(HKEY_LOCAL_MACHINE, keyPath, L"Layout Text");
      text && !text->empty()) {
    return wideCharToUtf8(*text);
  }

  if (const auto name = queryRegistryString(HKEY_LOCAL_MACHINE, keyPath, L"Layout Display Name");
      name && !name->empty()) {
    return wideCharToUtf8(*name);
  }

  return {};
}

void AppUtilWindows::eventLoop()
{
  HANDLE hCloseEvent = CreateEvent(nullptr, TRUE, FALSE, kCloseEventName);
  if (!hCloseEvent) {
    LOG_CRIT("failed to create event for windows event loop");
    throw std::runtime_error(windowsErrorToString(GetLastError()));
  }

  LOG_DEBUG("windows event loop running");
  {
    std::scoped_lock lock{m_eventThreadStartedMutex};
    m_eventThreadRunning = true;
  }
  m_eventThreadStartedCond.notify_one();

  while (m_eventThreadRunning) {
    DWORD closeEventResult = MsgWaitForMultipleObjects(1, &hCloseEvent, FALSE, 100, QS_ALLINPUT);

    if (closeEventResult == WAIT_OBJECT_0) {
      LOG_DEBUG("windows event loop received close event");
      m_events->addEvent(Event(EventTypes::Quit));
      m_eventThreadRunning = false;
    } else if (closeEventResult == WAIT_OBJECT_0 + 1) {
      MSG msg;
      while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
      }
    }
  }

  CloseHandle(hCloseEvent);
  LOG_DEBUG("windows event loop finished");
}
