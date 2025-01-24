/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "platform/MSWindowsClipboard.h"

//! Convert to/from some text encoding
class MSWindowsClipboardAnyTextConverter : public IMSWindowsClipboardConverter
{
public:
  MSWindowsClipboardAnyTextConverter();
  virtual ~MSWindowsClipboardAnyTextConverter();

  // IMSWindowsClipboardConverter overrides
  virtual IClipboard::EFormat getFormat() const;
  virtual UINT getWin32Format() const = 0;
  virtual HANDLE fromIClipboard(const std::string &) const;
  virtual std::string toIClipboard(HANDLE) const;

protected:
  //! Convert from IClipboard format
  /*!
  Do UTF-8 conversion only.  Memory handle allocation and
  linefeed conversion is done by this class.  doFromIClipboard()
  must include the nul terminator in the returned string (not
  including the std::string's nul terminator).
  */
  virtual std::string doFromIClipboard(const std::string &) const = 0;

  //! Convert to IClipboard format
  /*!
  Do UTF-8 conversion only.  Memory handle allocation and
  linefeed conversion is done by this class.
  */
  virtual std::string doToIClipboard(const std::string &) const = 0;

private:
  std::string convertLinefeedToWin32(const std::string &) const;
  std::string convertLinefeedToUnix(const std::string &) const;
};
