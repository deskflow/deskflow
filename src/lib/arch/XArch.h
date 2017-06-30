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

#include "common/common.h"
#include "common/stdstring.h"
#include "common/stdexcept.h"

//! Generic thread exception
/*!
Exceptions derived from this class are used by the multithreading
library to perform stack unwinding when a thread terminates.  These
exceptions must always be rethrown by clients when caught.
*/
class XThread {};

//! Thread exception to cancel
/*!
Thrown to cancel a thread.  Clients must not throw this type, but
must rethrow it if caught (by XThreadCancel, XThread, or ...).
*/
class XThreadCancel : public XThread {};

/*!
\def RETHROW_XTHREAD
Convenience macro to rethrow an XThread exception but ignore other
exceptions.  Put this in your catch (...) handler after necessary
cleanup but before leaving or returning from the handler.
*/
#define RETHROW_XTHREAD                                                        \
    try {                                                                      \
        throw;                                                                 \
    } catch (XThread&) {                                                       \
        throw;                                                                 \
    } catch (...) {                                                            \
    }

//! Lazy error message string evaluation
/*!
This class encapsulates platform dependent error string lookup.
Platforms subclass this type, taking an appropriate error code
type in the c'tor and overriding eval() to return the error
string for that error code.
*/
class XArchEval {
public:
    XArchEval () {
    }
    virtual ~XArchEval () _NOEXCEPT {
    }

    virtual std::string eval () const = 0;
};

//! Generic exception architecture dependent library
class XArch : public std::runtime_error {
public:
    XArch (XArchEval* adopted) : std::runtime_error (adopted->eval ()) {
        delete adopted;
    }
    XArch (const std::string& msg) : std::runtime_error (msg) {
    }
    virtual ~XArch () _NOEXCEPT {
    }
};

// Macro to declare XArch derived types
#define XARCH_SUBCLASS(name_, super_)                                          \
    class name_ : public super_ {                                              \
    public:                                                                    \
        name_ (XArchEval* adoptedEvaluator) : super_ (adoptedEvaluator) {      \
        }                                                                      \
        name_ (const std::string& msg) : super_ (msg) {                        \
        }                                                                      \
    }

//! Generic network exception
/*!
Exceptions derived from this class are used by the networking
library to indicate various errors.
*/
XARCH_SUBCLASS (XArchNetwork, XArch);

//! Operation was interrupted
XARCH_SUBCLASS (XArchNetworkInterrupted, XArchNetwork);

//! Network insufficient permission
XARCH_SUBCLASS (XArchNetworkAccess, XArchNetwork);

//! Network insufficient resources
XARCH_SUBCLASS (XArchNetworkResource, XArchNetwork);

//! No support for requested network resource/service
XARCH_SUBCLASS (XArchNetworkSupport, XArchNetwork);

//! Network I/O error
XARCH_SUBCLASS (XArchNetworkIO, XArchNetwork);

//! Network address is unavailable or not local
XARCH_SUBCLASS (XArchNetworkNoAddress, XArchNetwork);

//! Network address in use
XARCH_SUBCLASS (XArchNetworkAddressInUse, XArchNetwork);

//! No route to address
XARCH_SUBCLASS (XArchNetworkNoRoute, XArchNetwork);

//! Socket not connected
XARCH_SUBCLASS (XArchNetworkNotConnected, XArchNetwork);

//! Remote read end of socket has closed
XARCH_SUBCLASS (XArchNetworkShutdown, XArchNetwork);

//! Remote end of socket has disconnected
XARCH_SUBCLASS (XArchNetworkDisconnected, XArchNetwork);

//! Remote end of socket refused connection
XARCH_SUBCLASS (XArchNetworkConnectionRefused, XArchNetwork);

//! Remote end of socket is not responding
XARCH_SUBCLASS (XArchNetworkTimedOut, XArchNetwork);

//! Generic network name lookup erros
XARCH_SUBCLASS (XArchNetworkName, XArchNetwork);

//! The named host is unknown
XARCH_SUBCLASS (XArchNetworkNameUnknown, XArchNetworkName);

//! The named host is known but has no address
XARCH_SUBCLASS (XArchNetworkNameNoAddress, XArchNetworkName);

//! Non-recoverable name server error
XARCH_SUBCLASS (XArchNetworkNameFailure, XArchNetworkName);

//! Temporary name server error
XARCH_SUBCLASS (XArchNetworkNameUnavailable, XArchNetworkName);

//! The named host is known but no supported address
XARCH_SUBCLASS (XArchNetworkNameUnsupported, XArchNetworkName);

//! Generic daemon exception
/*!
Exceptions derived from this class are used by the daemon
library to indicate various errors.
*/
XARCH_SUBCLASS (XArchDaemon, XArch);

//! Could not daemonize
XARCH_SUBCLASS (XArchDaemonFailed, XArchDaemon);

//! Could not install daemon
XARCH_SUBCLASS (XArchDaemonInstallFailed, XArchDaemon);

//! Could not uninstall daemon
XARCH_SUBCLASS (XArchDaemonUninstallFailed, XArchDaemon);

//! Attempted to uninstall a daemon that was not installed
XARCH_SUBCLASS (XArchDaemonUninstallNotInstalled, XArchDaemonUninstallFailed);
