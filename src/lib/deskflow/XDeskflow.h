/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "base/XBase.h"

/**
 * @brief The XDeskflow class Generic deskflow exception class
 */
class XDeskflow : public XBase
{
  using XBase::XBase;
};

/**
 * @brief XBadClient - Thrown when the client fails to follow the protocol.
 */
class XBadClient : public XDeskflow
{
  using XDeskflow::XDeskflow;

protected:
  std::string getWhat() const throw() override;
};

/**
 * @brief XInvalidProtocol - Thrown when the server protocol is unreconized.
 */
class XInvalidProtocol : public XDeskflow
{
  using XDeskflow::XDeskflow;

protected:
  std::string getWhat() const throw() override;
};

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
  int getMajor() const noexcept;
  //! Get client's minor version number
  int getMinor() const noexcept;

  //@}

protected:
  std::string getWhat() const throw() override;

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
  explicit XDuplicateClient(const std::string &name);
  ~XDuplicateClient() throw() override = default;

  //! @name accessors
  //@{

  //! Get client's name
  virtual const std::string &getName() const noexcept;

  //@}

protected:
  std::string getWhat() const throw() override;

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
  explicit XUnknownClient(const std::string &name);
  ~XUnknownClient() throw() override = default;

  //! @name accessors
  //@{

  //! Get the client's name
  virtual const std::string &getName() const noexcept;

  //@}

protected:
  std::string getWhat() const throw() override;

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
  explicit XExitApp(int code);
  ~XExitApp() throw() override = default;

  //! Get the exit code
  int getCode() const noexcept;

protected:
  std::string getWhat() const throw() override;

private:
  int m_code;
};
