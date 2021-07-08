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

void
AppUtilUnix::showNotification(const String & title, const String & text) const
{
    LOG((CLOG_DEBUG "Showing notification. Title: \"%s\". Text: \"%s\"", title.c_str(), text.c_str()));
#if WINAPI_XWINDOWS
    notify_init("Synergy");
    NotifyNotification* n = notify_notification_new (title.c_str(), text.c_str(), nullptr);
    notify_notification_set_timeout(n, 10000);

    if (!notify_notification_show(n, nullptr))
    {
        LOG((CLOG_INFO "Failed to show notification"));
    }
#elif WINAPI_CARBON
    // synergys and synergyc are not allowed to send native notifications on MacOS
    // instead ask main synergy process to show them instead
    LOG((CLOG_INFO "OSX Notification: %s|%s", title.c_str(), text.c_str()));
#endif
}
