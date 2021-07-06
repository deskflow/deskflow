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
#include <dbus/dbus.h>
#include <qt5/QtDBus/QtDBus>

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
ArchSystemUnix::DBusCall(const char* service, const char* path, const char* param)
{
    DBusError dbus_error;
    DBusConnection * dbus_conn = nullptr;
    DBusMessage * dbus_msg = nullptr;
    DBusMessage * dbus_reply = nullptr;
    const char * dbus_result = nullptr;

    // Initialize D-Bus error
    ::dbus_error_init(&dbus_error);

    // Connect to D-Bus
    if ( nullptr == (dbus_conn = ::dbus_bus_get(DBUS_BUS_SYSTEM, &dbus_error)) ) {
        ::perror(dbus_error.name);
        ::perror(dbus_error.message);

    // Compose remote procedure call
    } else if ( nullptr == (dbus_msg = ::dbus_message_new_method_call(service, path, service, param)) ) {
        dbus_connection_unref(dbus_conn);
        ::perror("ERROR: ::dbus_message_new_method_call - Unable to allocate memory for the message!");

    // Invoke remote procedure call, block for response
    } else if ( nullptr == (dbus_reply = ::dbus_connection_send_with_reply_and_block(dbus_conn, dbus_msg, DBUS_TIMEOUT_USE_DEFAULT, &dbus_error)) ) {
        ::dbus_message_unref(dbus_msg);
        ::dbus_connection_unref(dbus_conn);
        ::perror(dbus_error.name);
        ::perror(dbus_error.message);

    // Parse response
    } else if ( !::dbus_message_get_args(dbus_reply, &dbus_error, DBUS_TYPE_STRING, &dbus_result, DBUS_TYPE_INVALID) ) {
        ::dbus_message_unref(dbus_msg);
        ::dbus_message_unref(dbus_reply);
        ::dbus_connection_unref(dbus_conn);
        ::perror(dbus_error.name);
        ::perror(dbus_error.message);

    // Work with the results of the remote procedure call
    } else {
        //std::cout << "Connected to D-Bus as \"" << ::dbus_bus_get_unique_name(dbus_conn) << "\"." << std::endl;
        //std::cout << "Introspection Result:" << std::endl;
        //std::cout << std::endl << dbus_result << std::endl << std::endl;
        ::dbus_message_unref(dbus_msg);
        ::dbus_message_unref(dbus_reply);

        /*
         * Applications must not close shared connections -
         * see dbus_connection_close() docs. This is a bug in the application.
         */
        //::dbus_connection_close(dbus_conn);

        // When using the System Bus, unreference
        // the connection instead of closing it
        ::dbus_connection_unref(dbus_conn);
        return true;
    }
    return false;
}

bool
ArchSystemUnix::disableSleep()
{
    return
        DBusCall("org.freedesktop.ScreenSaver", "/org/freedesktop/ScreenSaver", "Inhibit") ||
        DBusCall("org.gnome.SessionManager",    "/org/gnome/SessionManager",    "Inhibit");
}

bool
ArchSystemUnix::enableSleep()
{
    return
        DBusCall("org.freedesktop.ScreenSaver", "/org/freedesktop/ScreenSaver", "UnInhibit") ||
        DBusCall("org.gnome.SessionManager",    "/org/gnome/SessionManager",    "UnInhibit");
}
