/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <stdexcept>
#include <string>

//! Exception base class
/*!
This is the base class of most exception types.
*/
class XBase : public std::runtime_error
{
public:
  //! Use getWhat() as the result of what()
  XBase();
  //! Use \c msg as the result of what()
  explicit XBase(const std::string &msg);
  ~XBase() throw() override = default;

  //! Reason for exception
  const char *what() const throw() override;

protected:
  //! Get a human readable string describing the exception
  virtual std::string getWhat() const noexcept
  {
    return "";
  }

  //! Format a string
  /*!
  Looks up a message format using \c id, using \c defaultFormat if
  no format can be found, then replaces positional parameters in
  the format string and returns the result.
  */
  virtual std::string format(const char *id, const char *defaultFormat, ...) const noexcept;

private:
  mutable std::string m_what;
};
