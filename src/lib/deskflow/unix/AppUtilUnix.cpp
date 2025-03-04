/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "deskflow/unix/AppUtilUnix.h"
#include "deskflow/ArgsBase.h"
#include <filesystem>
#include <thread>

#if WINAPI_XWINDOWS
#include "deskflow/unix/X11LayoutsParser.h"
#include <X11/XKBlib.h>
#elif WINAPI_CARBON
#include <Carbon/Carbon.h>
#include <platform/OSXAutoTypes.h>
#else
#error Platform not supported.
#endif
#include "base/Log.h"
#include "base/log_outputters.h"
#include "common/constants.h"

#if HAVE_LIBNOTIFY
#include <libnotify/notify.h>
#endif

AppUtilUnix::AppUtilUnix(IEventQueue *events)
{
}

AppUtilUnix::~AppUtilUnix()
{
}

int standardStartupStatic(int argc, char **argv)
{
  return AppUtil::instance().app().standardStartup(argc, argv);
}

int AppUtilUnix::run(int argc, char **argv)
{
  return app().runInner(argc, argv, &standardStartupStatic);
}

void AppUtilUnix::startNode()
{
  app().startNode();
}

std::vector<std::string> AppUtilUnix::getKeyboardLayoutList()
{
  std::vector<std::string> layoutLangCodes;

#if WINAPI_XWINDOWS
  // Check /usr/local first used on bsd and some systems
  m_evdev = "/usr/local/share/X11/xkb/rules/evdev.xml";
  if (!std::filesystem::exists(m_evdev))
    m_evdev = "/usr/share/X11/xkb/rules/evdev.xml";
  layoutLangCodes = X11LayoutsParser::getX11LanguageList(m_evdev);

#elif WINAPI_CARBON
  CFStringRef keys[] = {kTISPropertyInputSourceCategory};
  CFStringRef values[] = {kTISCategoryKeyboardInputSource};
  AutoCFDictionary dict(CFDictionaryCreate(NULL, (const void **)keys, (const void **)values, 1, NULL, NULL), CFRelease);
  AutoCFArray kbds(TISCreateInputSourceList(dict.get(), false), CFRelease);

  for (CFIndex i = 0; i < CFArrayGetCount(kbds.get()); ++i) {
    TISInputSourceRef keyboardLayout = (TISInputSourceRef)CFArrayGetValueAtIndex(kbds.get(), i);
    auto layoutLanguages = (CFArrayRef)TISGetInputSourceProperty(keyboardLayout, kTISPropertyInputSourceLanguages);
    char temporaryCString[128] = {0};
    for (CFIndex index = 0; index < CFArrayGetCount(layoutLanguages) && layoutLanguages; index++) {
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

std::string AppUtilUnix::getCurrentLanguageCode()
{
  std::string result = "";
#if WINAPI_XWINDOWS

  auto display = XOpenDisplay(nullptr);
  if (!display) {
    LOG((CLOG_WARN "failed to open x11 default display"));
    return result;
  }

  auto kbdDescr = XkbAllocKeyboard();
  if (!kbdDescr) {
    LOG((CLOG_WARN "failed to get x11 keyboard description"));
    return result;
  }
  XkbGetNames(display, XkbSymbolsNameMask, kbdDescr);

  Atom symNameAtom = kbdDescr->names->symbols;
  auto rawLayouts = std::string(XGetAtomName(display, symNameAtom));

  XkbStateRec state;
  XkbGetState(display, XkbUseCoreKbd, &state);
  auto nedeedGroupIndex = static_cast<int>(state.group);

  size_t groupIdx = 0;
  size_t groupStartI = 0;
  for (size_t strI = 0; strI < rawLayouts.size(); strI++) {
    if (rawLayouts[strI] != '+') {
      continue;
    }

    auto group = rawLayouts.substr(groupStartI, strI - groupStartI);
    if (group.find("group", 0, 5) == std::string::npos && group.find("inet", 0, 4) == std::string::npos &&
        group.find("pc", 0, 2) == std::string::npos) {
      if (nedeedGroupIndex == groupIdx) {
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

  result = X11LayoutsParser::convertLayotToISO(m_evdev, result);

#elif WINAPI_CARBON
  auto layoutLanguages =
      (CFArrayRef)TISGetInputSourceProperty(TISCopyCurrentKeyboardInputSource(), kTISPropertyInputSourceLanguages);
  char temporaryCString[128] = {0};
  for (CFIndex index = 0; index < CFArrayGetCount(layoutLanguages) && layoutLanguages; index++) {
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

void AppUtilUnix::showNotification(const std::string &title, const std::string &text) const
{
#if HAVE_LIBNOTIFY
  LOG((CLOG_INFO "showing notification, title=\"%s\", text=\"%s\"", title.c_str(), text.c_str()));
  if (!notify_init(kAppName)) {
    LOG((CLOG_WARN "failed to initialize libnotify"));
    return;
  }

  auto notification = notify_notification_new(title.c_str(), text.c_str(), nullptr);
  if (notification == nullptr) {
    LOG((CLOG_WARN "failed to create notification"));
    return;
  }
  notify_notification_set_timeout(notification, 10000);

  if (!notify_notification_show(notification, nullptr)) {
    LOG((CLOG_WARN "failed to show notification"));
  }

  g_object_unref(G_OBJECT(notification));
  notify_uninit();

#elif WINAPI_CARBON
  // server and client processes are not allowed to show notifications.
  // MacOS instead ask main deskflow process to show them instead.
  LOG((CLOG_INFO "mac notification: %s|%s", title.c_str(), text.c_str()));
#endif
}
