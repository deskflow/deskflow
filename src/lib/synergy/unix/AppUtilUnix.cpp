/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2002 Chris Schoeneman
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "synergy/unix/AppUtilUnix.h"
#include "synergy/ArgsBase.h"
#include <thread>

#if WINAPI_XWINDOWS
#include "synergy/unix/X11LayoutsParser.h"
#elif WINAPI_CARBON
#include <Carbon/Carbon.h>
#else
#error Platform not supported.
#endif
#include "base/Log.h"
#include "base/log_outputters.h"

#if WINAPI_XWINDOWS
#include <libnotify/notify.h>
#endif

AppUtilUnix::AppUtilUnix(IEventQueue* events)
{
}

AppUtilUnix::~AppUtilUnix()
{
}

int
standardStartupStatic(int argc, char** argv)
{
    return AppUtil::instance().app().standardStartup(argc, argv);
}

int
AppUtilUnix::run(int argc, char** argv)
{
    return app().runInner(argc, argv, NULL, &standardStartupStatic);
}

void
AppUtilUnix::startNode()
{
    app().startNode();
}

std::vector<String>
AppUtilUnix::getKeyboardLayoutList()
{
    std::vector<String> layoutLangCodes;

#if WINAPI_XWINDOWS
    layoutLangCodes = X11LayoutsParser::getX11LanguageList();
#elif WINAPI_CARBON
    CFStringRef keys[] = { kTISPropertyInputSourceCategory };
    CFStringRef values[] = { kTISCategoryKeyboardInputSource };
    CFDictionaryRef dict = CFDictionaryCreate(NULL, (const void **)keys, (const void **)values, 1, NULL, NULL);
    CFArrayRef kbds = TISCreateInputSourceList(dict, false);

    for (CFIndex i = 0; i < CFArrayGetCount(kbds); ++i) {
        TISInputSourceRef keyboardLayout = (TISInputSourceRef)CFArrayGetValueAtIndex(kbds, i);
        auto layoutLanguages = (CFArrayRef)TISGetInputSourceProperty(keyboardLayout, kTISPropertyInputSourceLanguages);
        char temporaryCString[128] = {0};
        for(CFIndex index = 0; index < CFArrayGetCount(layoutLanguages) && layoutLanguages; index++) {
            auto languageCode = (CFStringRef)CFArrayGetValueAtIndex(layoutLanguages, index);
            if(!languageCode ||
               !CFStringGetCString(languageCode, temporaryCString, 128, kCFStringEncodingUTF8)) {
                continue;
            }

            std::string langCode(temporaryCString);
            if(langCode.size() == 2 &&
               std::find(layoutLangCodes.begin(), layoutLangCodes.end(), langCode) == layoutLangCodes.end()){
                layoutLangCodes.push_back(langCode);
            }

            //Save only first language code
            break;
        }
    }
#endif

    return layoutLangCodes;
}
	
void
AppUtilUnix::showNotification(const String & title, const String & text) const
{
    LOG((CLOG_DEBUG "Showing notification. Title: \"%s\". Text: \"%s\"", title.c_str(), text.c_str()));
#if WINAPI_XWINDOWS
    if (!notify_init("Synergy"))
    {
        LOG((CLOG_INFO "Failed to initialize libnotify"));
        return;
    }

    auto notification = notify_notification_new (title.c_str(), text.c_str(), nullptr);
    if (notification == nullptr)
    {
        LOG((CLOG_INFO "Failed to create notification"));
        return;
    }
    notify_notification_set_timeout(notification, 10000);

    if (!notify_notification_show(notification, nullptr))
    {
        LOG((CLOG_INFO "Failed to show notification"));
    }

    g_object_unref(G_OBJECT(notification));
    notify_uninit();

#elif WINAPI_CARBON
    // synergys and synergyc are not allowed to send native notifications on MacOS
    // instead ask main synergy process to show them instead
    LOG((CLOG_INFO "OSX Notification: %s|%s", title.c_str(), text.c_str()));
#endif
}
