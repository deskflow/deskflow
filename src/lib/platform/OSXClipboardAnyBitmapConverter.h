/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2014 - 2016 Symless Ltd
 * SPDX-FileCopyrightText: (C) 2014 Ryan Chapman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "platform/OSXClipboard.h"

//! Convert to/from some text encoding
class OSXClipboardAnyBitmapConverter : public IOSXClipboardConverter
{
public:
  OSXClipboardAnyBitmapConverter();
  virtual ~OSXClipboardAnyBitmapConverter();

  // IOSXClipboardConverter overrides
  virtual IClipboard::EFormat getFormat() const;
  virtual CFStringRef getOSXFormat() const = 0;
  virtual std::string fromIClipboard(const std::string &) const;
  virtual std::string toIClipboard(const std::string &) const;

protected:
  //! Convert from IClipboard format
  /*!
   Do UTF-8 conversion and linefeed conversion.
  */
  virtual std::string doFromIClipboard(const std::string &) const = 0;

  //! Convert to IClipboard format
  /*!
   Do UTF-8 conversion and Linefeed conversion.
  */
  virtual std::string doToIClipboard(const std::string &) const = 0;
};
