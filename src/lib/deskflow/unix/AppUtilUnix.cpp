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

#if WINAPI_XWINDOWS
#include <X11/XKBlib.h>
#include <deskflow/unix/XkbLayoutsParser.h>
#elif defined(Q_OS_MACOS)
#include <Carbon/Carbon.h>
#include <platform/OSXAutoTypes.h>

#include <algorithm>
#include <cassert>
#include <mutex>
#include <pthread.h>
#include <utility>
#endif

#if defined(Q_OS_MACOS)
class MacInputSourceCache
{
public:
  MacInputSourceCache() : m_notificationCenter(CFNotificationCenterGetDistributedCenter())
  {
    assert(pthread_main_np());
    refreshKeyboardLayouts();
    refreshCurrentLanguage();
    CFNotificationCenterAddObserver(
        m_notificationCenter, this, &MacInputSourceCache::handleNotification,
        kTISNotifySelectedKeyboardInputSourceChanged, nullptr, CFNotificationSuspensionBehaviorDeliverImmediately
    );
    CFNotificationCenterAddObserver(
        m_notificationCenter, this, &MacInputSourceCache::handleNotification,
        kTISNotifyEnabledKeyboardInputSourcesChanged, nullptr, CFNotificationSuspensionBehaviorDeliverImmediately
    );
  }

  ~MacInputSourceCache()
  {
    CFNotificationCenterRemoveEveryObserver(m_notificationCenter, this);
  }

  std::vector<std::string> keyboardLayouts() const
  {
    std::lock_guard lock(m_mutex);
    return m_keyboardLayouts;
  }

  std::string currentLanguage() const
  {
    std::lock_guard lock(m_mutex);
    return m_currentLanguage;
  }

private:
  static void
  handleNotification(CFNotificationCenterRef, void *observer, CFNotificationName name, const void *, CFDictionaryRef)
  {
    auto *cache = static_cast<MacInputSourceCache *>(observer);
    if (!pthread_main_np()) {
      LOG_WARN("ignoring input source notification delivered outside the main thread");
      return;
    }

    if (CFEqual(name, kTISNotifySelectedKeyboardInputSourceChanged)) {
      cache->refreshCurrentLanguage();
    } else if (CFEqual(name, kTISNotifyEnabledKeyboardInputSourcesChanged)) {
      cache->refreshKeyboardLayouts();
      cache->refreshCurrentLanguage();
    }
  }

  void refreshKeyboardLayouts()
  {
    assert(pthread_main_np());
    std::vector<std::string> layouts;
    CFStringRef keys[] = {kTISPropertyInputSourceCategory};
    CFStringRef values[] = {kTISCategoryKeyboardInputSource};
    AutoCFDictionary dict(
        CFDictionaryCreate(nullptr, (const void **)keys, (const void **)values, 1, nullptr, nullptr), CFRelease
    );
    AutoCFArray sources(TISCreateInputSourceList(dict.get(), false), CFRelease);

    for (CFIndex i = 0; sources && i < CFArrayGetCount(sources.get()); ++i) {
      auto source = (TISInputSourceRef)CFArrayGetValueAtIndex(sources.get(), i);
      auto languages = (CFArrayRef)TISGetInputSourceProperty(source, kTISPropertyInputSourceLanguages);
      char buffer[128] = {0};
      for (CFIndex index = 0; languages && index < CFArrayGetCount(languages); ++index) {
        auto language = (CFStringRef)CFArrayGetValueAtIndex(languages, index);
        if (!language || !CFStringGetCString(language, buffer, sizeof(buffer), kCFStringEncodingUTF8)) {
          continue;
        }

        std::string code(buffer);
        if (code.size() == 2 && std::find(layouts.begin(), layouts.end(), code) == layouts.end()) {
          layouts.push_back(std::move(code));
        }
        break;
      }
    }

    std::lock_guard lock(m_mutex);
    m_keyboardLayouts = std::move(layouts);
  }

  void refreshCurrentLanguage()
  {
    assert(pthread_main_np());
    std::string result;
    AutoTISInputSourceRef source(TISCopyCurrentKeyboardInputSource(), CFRelease);
    auto languages =
        source ? (CFArrayRef)TISGetInputSourceProperty(source.get(), kTISPropertyInputSourceLanguages) : nullptr;
    char buffer[128] = {0};
    for (CFIndex index = 0; languages && index < CFArrayGetCount(languages); ++index) {
      auto language = (CFStringRef)CFArrayGetValueAtIndex(languages, index);
      if (language && CFStringGetCString(language, buffer, sizeof(buffer), kCFStringEncodingUTF8)) {
        result = buffer;
        break;
      }
    }

    std::lock_guard lock(m_mutex);
    m_currentLanguage = std::move(result);
  }

  CFNotificationCenterRef m_notificationCenter;
  mutable std::mutex m_mutex;
  std::vector<std::string> m_keyboardLayouts;
  std::string m_currentLanguage;
};
#endif

AppUtilUnix::AppUtilUnix(const IEventQueue *)
{
#if defined(Q_OS_MACOS)
  m_inputSourceCache = std::make_unique<MacInputSourceCache>();
#endif
}

AppUtilUnix::~AppUtilUnix() = default;

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

#if WINAPI_XWINDOWS
  layoutLangCodes = XkbLayoutsParser::getXkbLanguageList();

#elif defined(Q_OS_MACOS)
  layoutLangCodes = m_inputSourceCache->keyboardLayouts();
#endif

  return layoutLangCodes;
}

// TODO: read the layout for x and wayland via xkbcommon
std::string AppUtilUnix::getCurrentLanguageCode()
{
  std::string result = "";
  if (deskflow::platform::isWayland())
    return result;

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
  result = m_inputSourceCache->currentLanguage();
#endif
  return result;
}
