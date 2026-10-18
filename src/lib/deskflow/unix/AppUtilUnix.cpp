/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "deskflow/unix/AppUtilUnix.h"

#include "base/Log.h"
#include "common/PlatformInfo.h"

#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
#include "platform/WaylandKeyboardManager.h"
#endif

#if WINAPI_XWINDOWS
#include <X11/XKBlib.h>
#include <deskflow/unix/XkbLayoutsParser.h>
#elif defined(Q_OS_MACOS)
#include <Carbon/Carbon.h>
#include <platform/OSXAutoTypes.h>
#endif

AppUtilUnix::AppUtilUnix(const IEventQueue *)
{
  // do nothing
}

int startStatic()
{
  return AppUtil::instance().app().start();
}

int AppUtilUnix::run()
{
  return app().runInner(&startStatic);
}

std::vector<std::string> AppUtilUnix::getKeyboardLayoutList()
{
  std::vector<std::string> layoutLangCodes;
#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
  if (deskflow::platform::isWayland()) {
    auto &manager = deskflow::platform::WaylandKeyboardManager::instance();
    manager.waitForInitialProbe();
    for (const auto &language : manager.snapshot().languages) {
      if (!language.empty() &&
          std::find(layoutLangCodes.begin(), layoutLangCodes.end(), language) == layoutLangCodes.end())
        layoutLangCodes.push_back(language);
    }
    return layoutLangCodes;
  }
#endif

#if WINAPI_XWINDOWS
  layoutLangCodes = XkbLayoutsParser::getXkbLanguageList();

#elif defined(Q_OS_MACOS)
  CFStringRef keys[] = {kTISPropertyInputSourceCategory};
  CFStringRef values[] = {kTISCategoryKeyboardInputSource};
  AutoCFDictionary dict(
      CFDictionaryCreate(nullptr, (const void **)keys, (const void **)values, 1, nullptr, nullptr), CFRelease
  );
  AutoCFArray kbds(nullptr, CFRelease);
  {
    std::lock_guard<std::mutex> lock(g_tisMutex);
    kbds = AutoCFArray(TISCreateInputSourceList(dict.get(), false), CFRelease);
  }

  for (CFIndex i = 0; i < CFArrayGetCount(kbds.get()); ++i) {
    TISInputSourceRef keyboardLayout = (TISInputSourceRef)CFArrayGetValueAtIndex(kbds.get(), i);
    CFArrayRef layoutLanguages = nullptr;
    {
      std::lock_guard<std::mutex> lock(g_tisMutex);
      layoutLanguages = (CFArrayRef)TISGetInputSourceProperty(keyboardLayout, kTISPropertyInputSourceLanguages);
    }
    char temporaryCString[128] = {0};
    for (CFIndex index = 0; layoutLanguages && index < CFArrayGetCount(layoutLanguages); index++) {
      auto languageCode = (CFStringRef)CFArrayGetValueAtIndex(layoutLanguages, index);
      if (!languageCode || !CFStringGetCString(languageCode, temporaryCString, 128, kCFStringEncodingUTF8)) {
        continue;
      }

      std::string langCode(temporaryCString);
      if (langCode.size() == 2 &&
          std::find(layoutLangCodes.begin(), layoutLangCodes.end(), langCode) == layoutLangCodes.end()) {
        layoutLangCodes.push_back(langCode);
      }

      // Save only first language code
      break;
    }
  }
#endif

  return layoutLangCodes;
}

// TODO: read the layout for x and wayland via xkbcommon
std::string AppUtilUnix::getCurrentLanguageCode()
{
  std::string result = "";
#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
  if (deskflow::platform::isWayland()) {
    const auto snapshot = deskflow::platform::WaylandKeyboardManager::instance().snapshot();
    if (snapshot.group >= 0 && static_cast<size_t>(snapshot.group) < snapshot.languages.size())
      return snapshot.languages[snapshot.group];
    return {};
  }
#endif

#if WINAPI_XWINDOWS

  auto display = XOpenDisplay(nullptr);
  if (!display) {
    LOG_WARN("failed to open x11 default display");
    return result;
  }

  auto kbdDescr = XkbAllocKeyboard();
  if (!kbdDescr) {
    LOG_WARN("failed to get x11 keyboard description");
    return result;
  }
  XkbGetNames(display, XkbSymbolsNameMask, kbdDescr);

  Atom symNameAtom = kbdDescr->names->symbols;
  auto rawLayouts = std::string(XGetAtomName(display, symNameAtom));

  XkbStateRec state;
  XkbGetState(display, XkbUseCoreKbd, &state);
  auto neededGroupIndex = static_cast<int>(state.group);

  size_t groupIdx = 0;
  size_t groupStartI = 0;
  for (size_t strI = 0; strI < rawLayouts.size(); strI++) {
    if (rawLayouts[strI] != '+') {
      continue;
    }

    if (auto group = rawLayouts.substr(groupStartI, strI - groupStartI);
        group.find("group", 0, 5) == std::string::npos && group.find("inet", 0, 4) == std::string::npos &&
        group.find("pc", 0, 2) == std::string::npos) {
      if (neededGroupIndex == groupIdx) {
        result = group.substr(0, std::min(group.find('(', 0), group.find(':', 0)));
        break;
      }
      groupIdx++;
    }

    groupStartI = strI + 1;
  }

  XkbFreeNames(kbdDescr, XkbSymbolsNameMask, true);
  XFree(kbdDescr);
  XCloseDisplay(display);

  result = XkbLayoutsParser::convertLayoutToISO(result);

#elif defined(Q_OS_MACOS)
  AutoTISInputSourceRef source(nullptr, CFRelease);
  CFArrayRef layoutLanguages = nullptr;
  {
    std::lock_guard<std::mutex> lock(g_tisMutex);
    source = AutoTISInputSourceRef(TISCopyCurrentKeyboardInputSource(), CFRelease);
    if (source)
      layoutLanguages = (CFArrayRef)TISGetInputSourceProperty(source.get(), kTISPropertyInputSourceLanguages);
  }
  char temporaryCString[128] = {0};
  for (CFIndex index = 0; layoutLanguages && index < CFArrayGetCount(layoutLanguages); index++) {
    auto languageCode = (CFStringRef)CFArrayGetValueAtIndex(layoutLanguages, index);
    if (!languageCode || !CFStringGetCString(languageCode, temporaryCString, 128, kCFStringEncodingUTF8)) {
      continue;
    }

    result = std::string(temporaryCString);
    break;
  }
#endif
  return result;
}
