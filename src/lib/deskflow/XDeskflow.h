/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "base/XBase.h"

//! Generic deskflow exception
XBASE_SUBCLASS(XDeskflow, XBase);

//! Subscription error
/*!
Thrown when there is a problem with the subscription.
*/
XBASE_SUBCLASS(XSubscription, XDeskflow);

//! Client error exception
/*!
Thrown when the client fails to follow the protocol.
*/
XBASE_SUBCLASS_WHAT(XBadClient, XDeskflow);

//! Server protocol error
/*!
Thrown when the server protocol is unrecognized.
*/
XBASE_SUBCLASS_WHAT(XInvalidProtocol, XDeskflow);

//! Incompatible client exception
/*!
Thrown when a client attempting to connect has an incompatible version.
*/
class XIncompatibleClient : public XDeskflow
{
public:
  XIncompatibleClient(int major, int minor);

  //! @name accessors
  //@{

  //! Get client's major version number
  int getMajor() const throw();
  //! Get client's minor version number
  int getMinor() const throw();

  //@}

protected:
  virtual std::string getWhat() const throw();

private:
  int m_major;
  int m_minor;
};

//! Client already connected exception
/*!
Thrown when a client attempting to connect is using the same name as
a client that is already connected.
*/
class XDuplicateClient : public XDeskflow
{
public:
  XDuplicateClient(const std::string &name);
  virtual ~XDuplicateClient() _NOEXCEPT
  {
  }

  //! @name accessors
  //@{

  //! Get client's name
  virtual const std::string &getName() const throw();

  //@}

protected:
  virtual std::string getWhat() const throw();

private:
  std::string m_name;
};

//! Client not in map exception
/*!
Thrown when a client attempting to connect is using a name that is
unknown to the server.
*/
class XUnknownClient : public XDeskflow
{
public:
  XUnknownClient(const std::string &name);
  virtual ~XUnknownClient() _NOEXCEPT
  {
  }

  //! @name accessors
  //@{

  //! Get the client's name
  virtual const std::string &getName() const throw();

  //@}

protected:
  virtual std::string getWhat() const throw();

private:
  std::string m_name;
};

//! Generic exit eception
/*!
Thrown when we want to abort, with the opportunity to clean up. This is a
little bit of a hack, but it's a better way of exiting, than just calling
exit(int).
*/
class XExitApp : public XDeskflow
{
public:
  XExitApp(int code);
  virtual ~XExitApp() _NOEXCEPT
  {
  }

  //! Get the exit code
  int getCode() const throw();

protected:
  virtual std::string getWhat() const throw();

private:
  int m_code;
};
