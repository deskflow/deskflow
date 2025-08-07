/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "common/Common.h"

#include <stdexcept>
#include <string>

//! Generic thread exception
/*!
Exceptions derived from this class are used by the multithreading
library to perform stack unwinding when a thread terminates.  These
exceptions must always be rethrown by clients when caught.
*/
class ThreadException : public std::exception
{
};

//! Thread exception to cancel
/*!
Thrown to cancel a thread.  Clients must not throw this type, but
must rethrow it if caught (by ThreadCancelException, ThreadException, or ...).
*/
class ThreadCancelException : public ThreadException
{
};

/*!
\def RETHROW_THREADEXCEPTION
Convenience macro to rethrow an ThreadException exception but ignore other
exceptions.  Put this in your catch (...) handler after necessary
cleanup but before leaving or returning from the handler.
*/
#define RETHROW_THREADEXCEPTION                                                                                        \
  try {                                                                                                                \
    throw;                                                                                                             \
  } catch (ThreadException &) {                                                                                        \
    throw;                                                                                                             \
  } catch (...) {                                                                                                      \
  }

//! Generic network exception
/*!
Exceptions derived from this class are used by the networking
library to indicate various errors.
*/
class ArchNetworkException : public std::runtime_error
{
  using std::runtime_error::runtime_error;
};

//! Operation was interrupted
class ArchNetworkInterruptedException : public ArchNetworkException
{
  using ArchNetworkException::ArchNetworkException;
};

//! Network insufficient permission
class ArchNetworkAccessException : public ArchNetworkException
{
  using ArchNetworkException::ArchNetworkException;
};

//! Network insufficient resources
class ArchNetworkResourceException : public ArchNetworkException
{
  using ArchNetworkException::ArchNetworkException;
};

//! No support for requested network resource/service
class ArchNetworkSupportException : public ArchNetworkException
{
  using ArchNetworkException::ArchNetworkException;
};

//! Network I/O error
class ArchNetworkIOException : public ArchNetworkException
{
  using ArchNetworkException::ArchNetworkException;
};

//! Network address is unavailable or not local
class ArchNetworkNoAddressException : public ArchNetworkException
{
  using ArchNetworkException::ArchNetworkException;
};

//! Network address in use
class ArchNetworkAddressInUseException : public ArchNetworkException
{
  using ArchNetworkException::ArchNetworkException;
};

//! No route to address
class ArchNetworkNoRouteException : public ArchNetworkException
{
  using ArchNetworkException::ArchNetworkException;
};

//! Socket not connected
class ArchNetworkNotConnectedException : public ArchNetworkException
{
  using ArchNetworkException::ArchNetworkException;
};

//! Remote read end of socket has closed
class ArchNetworkShutdownException : public ArchNetworkException
{
  using ArchNetworkException::ArchNetworkException;
};

//! Remote end of socket has disconnected
class ArchNetworkDisconnectedException : public ArchNetworkException
{
  using ArchNetworkException::ArchNetworkException;
};

//! Remote end of socket refused connection
class ArchNetworkConnectionRefusedException : public ArchNetworkException
{
  using ArchNetworkException::ArchNetworkException;
};

//! Remote end of socket is not responding
class ArchNetworkTimedOutException : public ArchNetworkException
{
  using ArchNetworkException::ArchNetworkException;
};

//! Generic network name lookup erros
class ArchNetworkNameException : public ArchNetworkException
{
  using ArchNetworkException::ArchNetworkException;
};

//! The named host is unknown
class ArchNetworkNameUnknownException : public ArchNetworkNameException
{
  using ArchNetworkNameException::ArchNetworkNameException;
};

//! The named host is known but has no address
class ArchNetworkNameNoAddressException : public ArchNetworkNameException
{
  using ArchNetworkNameException::ArchNetworkNameException;
};

//! Non-recoverable name server error
class ArchNetworkNameFailureException : public ArchNetworkNameException
{
  using ArchNetworkNameException::ArchNetworkNameException;
};

//! Temporary name server error
class ArchNetworkNameUnavailableException : public ArchNetworkNameException
{
  using ArchNetworkNameException::ArchNetworkNameException;
};

//! The named host is known but no supported address
class ArchNetworkNameUnsupportedException : public ArchNetworkNameException
{
  using ArchNetworkNameException::ArchNetworkNameException;
};

//! Generic daemon exception
/*!
Exceptions derived from this class are used by the daemon
library to indicate various errors.
*/
class ArchDaemonException : public std::runtime_error
{
  using std::runtime_error::runtime_error;
};

//! Could not daemonize
class ArchDaemonFailedException : public ArchDaemonException
{
  using ArchDaemonException::ArchDaemonException;
};

//! Could not install daemon
class ArchDaemonInstallException : public ArchDaemonException
{
  using ArchDaemonException::ArchDaemonException;
};

//! Could not uninstall daemon
class ArchDaemonUninstallFailedException : public ArchDaemonException
{
  using ArchDaemonException::ArchDaemonException;
};

//! Attempted to uninstall a daemon that was not installed
class ArchDaemonUninstallNotInstalledException : public ArchDaemonFailedException
{
  using ArchDaemonFailedException::ArchDaemonFailedException;
};
