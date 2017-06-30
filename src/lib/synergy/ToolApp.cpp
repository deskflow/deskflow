/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2014-2016 Symless Ltd.
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

#include "synergy/ToolApp.h"

#include "synergy/ArgParser.h"
#include "arch/Arch.h"
#include "base/Log.h"
#include "base/String.h"

#include <iostream>
#include <sstream>

#if SYSAPI_WIN32
#include "platform/MSWindowsSession.h"
#endif

#define JSON_URL "https://symless.com/account/json/"

enum { kErrorOk, kErrorArgs, kErrorException, kErrorUnknown };

UInt32
ToolApp::run (int argc, char** argv) {
    if (argc <= 1) {
        std::cerr << "no args" << std::endl;
        return kErrorArgs;
    }

    try {
        ArgParser argParser (this);
        bool result = argParser.parseToolArgs (m_args, argc, argv);

        if (!result) {
            m_bye (kExitArgs);
        }

        if (m_args.m_printActiveDesktopName) {
#if SYSAPI_WIN32
            MSWindowsSession session;
            String name = session.getActiveDesktopName ();
            if (name.empty ()) {
                LOG ((CLOG_CRIT "failed to get active desktop name"));
                return kExitFailed;
            } else {
                String output = synergy::string::sprintf ("activeDesktop:%s",
                                                          name.c_str ());
                LOG ((CLOG_INFO "%s", output.c_str ()));
            }
#endif
        } else if (m_args.m_loginAuthenticate) {
            loginAuth ();
        } else if (m_args.m_getInstalledDir) {
            std::cout << ARCH->getInstalledDirectory () << std::endl;
        } else if (m_args.m_getProfileDir) {
            std::cout << ARCH->getProfileDirectory () << std::endl;
        } else if (m_args.m_getArch) {
            std::cout << ARCH->getPlatformName () << std::endl;
        } else if (m_args.m_notifyUpdate) {
            notifyUpdate ();
        } else if (m_args.m_notifyActivation) {
            notifyActivation ();
        } else {
            throw XSynergy ("Nothing to do");
        }
    } catch (std::exception& e) {
        LOG ((CLOG_CRIT "An error occurred: %s\n", e.what ()));
        return kExitFailed;
    } catch (...) {
        LOG ((CLOG_CRIT "An unknown error occurred.\n"));
        return kExitFailed;
    }

#if WINAPI_XWINDOWS
    // HACK: avoid sigsegv on linux
    m_bye (kErrorOk);
#endif

    return kErrorOk;
}

void
ToolApp::help () {
}

void
ToolApp::loginAuth () {
    String credentials;
    std::cin >> credentials;

    std::vector<String> parts = synergy::string::splitString (credentials, ':');
    size_t count              = parts.size ();

    if (count == 2) {
        String email    = parts[0];
        String password = parts[1];

        std::stringstream ss;
        ss << JSON_URL << "auth/";
        ss << "?email=" << ARCH->internet ().urlEncode (email);
        ss << "&password=" << password;

        std::cout << ARCH->internet ().get (ss.str ()) << std::endl;
    } else {
        throw XSynergy ("Invalid credentials.");
    }
}

void
ToolApp::notifyUpdate () {
    String data;
    std::cin >> data;

    std::vector<String> parts = synergy::string::splitString (data, ':');
    size_t count              = parts.size ();

    if (count == 3) {
        std::stringstream ss;
        ss << JSON_URL << "notify/update";
        ss << "?from=" << parts[0];
        ss << "&to=" << parts[1];
        ss << "&serial=" << parts[2];

        std::cout << ARCH->internet ().get (ss.str ()) << std::endl;
    } else {
        throw XSynergy ("Invalid update data.");
    }
}

void
ToolApp::notifyActivation () {
    String info;
    std::cin >> info;

    std::vector<String> parts = synergy::string::splitString (info, ':');
    size_t count              = parts.size ();

    if (count == 3 || count == 4) {
        String action   = parts[0];
        String identity = parts[1];
        String macHash  = parts[2];
        String os;

        if (count == 4) {
            os = parts[3];
        } else {
            os = ARCH->getOSName ();
        }

        std::stringstream ss;
        ss << JSON_URL << "notify/";
        ss << "?action=" << action;
        ss << "&identity=" << ARCH->internet ().urlEncode (identity);
        ss << "&mac=" << ARCH->internet ().urlEncode (macHash);
        ss << "&os=" << ARCH->internet ().urlEncode (ARCH->getOSName ());
        ss << "&arch="
           << ARCH->internet ().urlEncode (ARCH->getPlatformName ());

        try {
            std::cout << ARCH->internet ().get (ss.str ()) << std::endl;
        } catch (std::exception& e) {
            LOG ((CLOG_NOTE "An error occurred during notification: %s\n",
                  e.what ()));
        } catch (...) {
            LOG (
                (CLOG_NOTE "An unknown error occurred during notification.\n"));
        }
    } else {
        LOG ((CLOG_NOTE "notification failed"));
    }
}
