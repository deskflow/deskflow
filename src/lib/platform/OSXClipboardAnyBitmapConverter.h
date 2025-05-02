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
  OSXClipboardAnyBitmapConverter() = default;
  ~OSXClipboardAnyBitmapConverter() override = default;

  // IOSXClipboardConverter overrides
  IClipboard::EFormat getFormat() const override;
  CFStringRef getOSXFormat() const override = 0;
  std::string fromIClipboard(const std::string &) const override;
  std::string toIClipboard(const std::string &) const override;

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
