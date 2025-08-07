/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "base/BaseException.h"

/**
 * @brief The DeskflowException class Generic deskflow exception class
 */
class DeskflowException : public BaseException
{
  using BaseException::BaseException;
};

/**
 * @brief BadClientException - Thrown when the client fails to follow the protocol.
 */
class BadClientException : public DeskflowException
{
  using DeskflowException::DeskflowException;

protected:
  std::string getWhat() const throw() override;
};

/**
 * @brief InvalidProtocolException - Thrown when the server protocol is unreconized.
 */
class InvalidProtocolException : public DeskflowException
{
  using DeskflowException::DeskflowException;

protected:
  std::string getWhat() const throw() override;
};

//! Incompatible client exception
/*!
Thrown when a client attempting to connect has an incompatible version.
*/
class IncompatibleClientException : public DeskflowException
{
public:
  IncompatibleClientException(int major, int minor);

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
class DuplicateClientException : public DeskflowException
{
public:
  explicit DuplicateClientException(const std::string &name);
  ~DuplicateClientException() throw() override = default;

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
class UnknownClientException : public DeskflowException
{
public:
  explicit UnknownClientException(const std::string &name);
  ~UnknownClientException() throw() override = default;

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
class ExitAppException : public DeskflowException
{
public:
  explicit ExitAppException(int code);
  ~ExitAppException() throw() override = default;

  //! Get the exit code
  int getCode() const noexcept;

protected:
  std::string getWhat() const throw() override;

private:
  int m_code;
};
