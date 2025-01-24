/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "base/XBase.h"
#include "common/common.h"
#include "io/XIO.h"

//! Generic socket exception
XBASE_SUBCLASS(XSocket, XBase);

//! Socket bad address exception
/*!
Thrown when attempting to create an invalid network address.
*/
class XSocketAddress : public XSocket
{
public:
  //! Failure codes
  enum EError
  {
    kUnknown,     //!< Unknown error
    kNotFound,    //!< The hostname is unknown
    kNoAddress,   //!< The hostname is valid but has no IP address
    kUnsupported, //!< The hostname is valid but has no supported address
    kBadPort      //!< The port is invalid
  };

  XSocketAddress(EError, const std::string &hostname, int port) _NOEXCEPT;
  virtual ~XSocketAddress() _NOEXCEPT
  {
  }

  //! @name accessors
  //@{

  //! Get the error code
  EError getError() const throw();
  //! Get the hostname
  std::string getHostname() const throw();
  //! Get the port
  int getPort() const throw();

  //@}

protected:
  // XBase overrides
  virtual std::string getWhat() const throw();

private:
  EError m_error;
  std::string m_hostname;
  int m_port;
};

//! I/O closing exception
/*!
Thrown if a stream cannot be closed.
*/
XBASE_SUBCLASS_FORMAT(XSocketIOClose, XIOClose);

//! Socket cannot bind address exception
/*!
Thrown when a socket cannot be bound to an address.
*/
XBASE_SUBCLASS_FORMAT(XSocketBind, XSocket);

//! Socket address in use exception
/*!
Thrown when a socket cannot be bound to an address because the address
is already in use.
*/
XBASE_SUBCLASS(XSocketAddressInUse, XSocketBind);

//! Cannot connect socket exception
/*!
Thrown when a socket cannot connect to a remote endpoint.
*/
XBASE_SUBCLASS_FORMAT(XSocketConnect, XSocket);

//! Cannot create socket exception
/*!
Thrown when a socket cannot be created (by the operating system).
*/
XBASE_SUBCLASS_FORMAT(XSocketCreate, XSocket);
