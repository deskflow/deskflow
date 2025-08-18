/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "base/BaseException.h"
#include "io/IOException.h"

/**
 * @brief SocketException generic socket exception
 */
class SocketException : public BaseException
{
  using BaseException::BaseException;
};

//! Socket bad address exception
/*!
Thrown when attempting to create an invalid network address.
*/
class SocketAddressException : public SocketException
{
public:
  //! Failure codes
  enum class SocketError
  {
    Unknown,     //!< Unknown error
    NotFound,    //!< The hostname is unknown
    NoAddress,   //!< The hostname is valid but has no IP address
    Unsupported, //!< The hostname is valid but has no supported address
    BadPort      //!< The port is invalid
  };

  SocketAddressException(SocketError, const std::string &hostname, int port) noexcept;
  ~SocketAddressException() throw() override = default;

  //! @name accessors
  //@{

  //! Get the error code
  SocketError getError() const noexcept;
  //! Get the hostname
  std::string getHostname() const noexcept;
  //! Get the port
  int getPort() const noexcept;

  //@}

protected:
  // BaseException overrides
  std::string getWhat() const throw() override;

private:
  SocketError m_error;
  std::string m_hostname;
  int m_port;
};

/**
 * @brief SocketIOCloseException - Thrown if a stream cannot be closed.
 */
class SocketIOCloseException : public IOCloseException
{
public:
  SocketIOCloseException() : IOCloseException(), m_state(kDone)
  {
    // do nothing
  }
  explicit SocketIOCloseException(const std::string &msg) : IOCloseException(msg), m_state(kFirst)
  {
    // do nothing
  }
  ~SocketIOCloseException() throw() override = default;

  const char *what() const throw() override
  {
    if (m_state == kFirst) {
      m_state = kFormat;
      m_formatted = getWhat();
      m_state = kDone;
    }
    if (m_state == kDone) {
      return m_formatted.c_str();
    } else {
      return IOCloseException::what();
    }
  }

protected:
  std::string getWhat() const throw() override;

private:
  enum EState
  {
    kFirst,
    kFormat,
    kDone
  };
  mutable EState m_state;
  mutable std::string m_formatted;
};

/**
 * @brief SocketWithWhatException - generic SocketException Exception with a generic `what` method impl
 */
class SocketWithWhatException : public SocketException
{
public:
  SocketWithWhatException() : SocketException(), m_state(kDone)
  {
    // do nothing
  }
  explicit SocketWithWhatException(const std::string &msg) : SocketException(msg), m_state(kFirst)
  {
    // do nothing
  }
  ~SocketWithWhatException() throw() override = default;

  const char *what() const throw() override
  {
    if (m_state == kFirst) {
      m_state = kFormat;
      m_formatted = getWhat();
      m_state = kDone;
    }
    if (m_state == kDone) {
      return m_formatted.c_str();
    } else {
      return SocketException::what();
    }
  }

private:
  enum EState
  {
    kFirst,
    kFormat,
    kDone
  };
  mutable EState m_state;
  mutable std::string m_formatted;
};

/**
 * @brief SocketBindException - Thrown when a socket cannot be bound to an address.
 */
class SocketBindException : public SocketWithWhatException
{
  using SocketWithWhatException::SocketWithWhatException;

protected:
  std::string getWhat() const throw() override;
};

/**
 * @brief SocketAddressInUseException
 * Thrown when a socket cannot be bound to an address because the address is already in use.
 */
class SocketAddressInUseException : public SocketWithWhatException
{
  using SocketWithWhatException::SocketWithWhatException;

protected:
  std::string getWhat() const throw() override;
};

/**
 * @brief SocketConnectException - Thrown when a socket cannot connect to a remote endpoint.
 */
class SocketConnectException : public SocketWithWhatException
{
  using SocketWithWhatException::SocketWithWhatException;

protected:
  std::string getWhat() const throw() override;
};

/**
 * @brief SocketCreateException - Thrown when a socket cannot be created (by the operating system).
 */
class SocketCreateException : public SocketWithWhatException
{
  using SocketWithWhatException::SocketWithWhatException;

protected:
  std::string getWhat() const throw() override;
};
