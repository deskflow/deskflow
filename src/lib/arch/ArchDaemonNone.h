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

#pragma once

#include "arch/IArchDaemon.h"

#define ARCH_DAEMON ArchDaemonNone

//! Dummy implementation of IArchDaemon
/*!
This class implements IArchDaemon for a platform that does not have
daemons.  The install and uninstall functions do nothing, the query
functions return false, and \c daemonize() simply calls the passed
function and returns its result.
*/
class ArchDaemonNone : public IArchDaemon {
public:
    ArchDaemonNone ();
    virtual ~ArchDaemonNone ();

    // IArchDaemon overrides
    virtual void installDaemon (const char* name, const char* description,
                                const char* pathname, const char* commandLine,
                                const char* dependencies);
    virtual void uninstallDaemon (const char* name);
    virtual int daemonize (const char* name, DaemonFunc func);
    virtual bool canInstallDaemon (const char* name);
    virtual bool isDaemonInstalled (const char* name);
    virtual void installDaemon ();
    virtual void uninstallDaemon ();
    virtual std::string commandLine () const;
};
