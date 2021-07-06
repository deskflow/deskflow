/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2004 Chris Schoeneman
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

#include "arch/unix/ArchSystemUnix.h"

#include <sys/utsname.h>
#include <stdio.h>
#include <array>
#include <memory>
#include <string>
#include <QtDBus>

//
// ArchSystemUnix
//

ArchSystemUnix::ArchSystemUnix()
{
    // do nothing
}

ArchSystemUnix::~ArchSystemUnix()
{
    // do nothing
}

std::string
ArchSystemUnix::getOSName() const
{
#if defined(HAVE_SYS_UTSNAME_H)
    struct utsname info;
    if (uname(&info) == 0) {
        std::string msg;
        msg += info.sysname;
        msg += " ";
        msg += info.release;
        return msg;
    }
#endif
    return "Unix";
}

std::string
ArchSystemUnix::getPlatformName() const
{
#if defined(HAVE_SYS_UTSNAME_H)
    struct utsname info;
    if (uname(&info) == 0) {
        return std::string(info.machine);
    }
#endif
    return "unknown";
}

std::string
ArchSystemUnix::setting(const std::string&) const
{
    return "";
}

void
ArchSystemUnix::setting(const std::string&, const std::string&) const
{
}

std::string
ArchSystemUnix::getLibsUsed(void) const
{
    return "not implemented.\nuse lsof on shell";
}

std::string
ArchSystemUnix::runCommand(const std::string& cmd)
{
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), &pclose);
    if (!pipe)
    {
        return "";
    }

    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
    {
        result += buffer.data();
    }
    return result;
}

bool
ArchSystemUnix::DBusInhibitScreenCall(bool state)
{
    static const std::array<QString, 2> services =
    {
        "org.freedesktop.ScreenSaver",
        "org.gnome.SessionManager"
    };
    static const std::array<QString, 2> paths =
    {
        "/org/freedesktop/ScreenSaver",
        "/org/gnome/SessionManager"
    };

    static std::array<uint, 2> cookies;

    QDBusConnection bus = QDBusConnection::sessionBus();
    if (bus.isConnected())
    {
        for (int i = 0; i < services.size() ; i++)
        {
            QDBusInterface screenSaverInterface( services[i], paths[i],services[i], bus);

            if (!screenSaverInterface.isValid())
                continue;

            QDBusReply<uint> reply;

            if(state)
            {
                if (cookies[i])
                    return false;

                reply = screenSaverInterface.call("Inhibit", "Synergy", "Sleep is manually prevented by the Synergy preferences");
                if (reply.isValid())
                    cookies[i] = reply.value();
                else
                    return false;
            }
            else
            {
                reply  = screenSaverInterface.call("UnInhibit", cookies[i]);
                cookies[i] = 0;
                if (!reply.isValid())
                    return false;
            }
        }
    }
    return true;
}

bool
ArchSystemUnix::disableSleep()
{
    return DBusInhibitScreenCall(true);
}

bool
ArchSystemUnix::enableSleep()
{
    return DBusInhibitScreenCall(false);
}
