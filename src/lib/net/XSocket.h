/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "base/XBase.h"
#include "common/Common.h"
#include "io/XIO.h"

/**
 * @brief XSocket generic socket exception
 */
class XSocket : public XBase
{
  using XBase::XBase;
};

//! Socket bad address exception
/*!
Thrown when attempting to create an invalid network address.
*/
class XSocketAddress : public XSocket
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

  XSocketAddress(SocketError, const std::string &hostname, int port) noexcept;
  ~XSocketAddress() throw() override = default;

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
  // XBase overrides
  std::string getWhat() const throw() override;

private:
  SocketError m_error;
  std::string m_hostname;
  int m_port;
};

/**
 * @brief XSocketIOClose - Thrown if a stream cannot be closed.
 */
class XSocketIOClose : public XIOClose
{
public:
  XSocketIOClose() : XIOClose(), m_state(kDone)
  {
    // do nothing
  }
  explicit XSocketIOClose(const std::string &msg) : XIOClose(msg), m_state(kFirst)
  {
    // do nothing
  }
  ~XSocketIOClose() throw() override = default;

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
      return XIOClose::what();
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
 * @brief XSocketWithWhat - generic XSocket Exception with a generic `what` method impl
 */
class XSocketWithWhat : public XSocket
{
public:
  XSocketWithWhat() : XSocket(), m_state(kDone)
  {
    // do nothing
  }
  explicit XSocketWithWhat(const std::string &msg) : XSocket(msg), m_state(kFirst)
  {
    // do nothing
  }
  ~XSocketWithWhat() throw() override = default;

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
      return XSocket::what();
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
 * @brief XSocketBind - Thrown when a socket cannot be bound to an address.
 */
class XSocketBind : public XSocketWithWhat
{
  using XSocketWithWhat::XSocketWithWhat;

protected:
  std::string getWhat() const throw() override;
};

/**
 * @brief XSocketAddressInUse
 * Thrown when a socket cannot be bound to an address because the address is already in use.
 */
class XSocketAddressInUse : public XSocketWithWhat
{
  using XSocketWithWhat::XSocketWithWhat;

protected:
  std::string getWhat() const throw() override;
};

/**
 * @brief XSocketConnect - Thrown when a socket cannot connect to a remote endpoint.
 */
class XSocketConnect : public XSocketWithWhat
{
  using XSocketWithWhat::XSocketWithWhat;

protected:
  std::string getWhat() const throw() override;
};

/**
 * @brief XSocketConnect - Thrown when a socket cannot be created (by the operating system).
 */
class XSocketCreate : public XSocketWithWhat
{
  using XSocketWithWhat::XSocketWithWhat;

protected:
  std::string getWhat() const throw() override;
};
