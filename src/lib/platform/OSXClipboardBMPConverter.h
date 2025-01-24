/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2014 - 2016 Symless Ltd
 * SPDX-FileCopyrightText: (C) 2014 Ryan Chapman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "platform/OSXClipboard.h"

//! Convert to/from some text encoding
class OSXClipboardBMPConverter : public IOSXClipboardConverter
{
public:
  OSXClipboardBMPConverter();
  virtual ~OSXClipboardBMPConverter();

  // IMSWindowsClipboardConverter overrides
  virtual IClipboard::EFormat getFormat() const;

  virtual CFStringRef getOSXFormat() const;

  // OSXClipboardAnyBMPConverter overrides
  virtual std::string fromIClipboard(const std::string &) const;
  virtual std::string toIClipboard(const std::string &) const;

  // generic encoding converter
  static std::string convertString(const std::string &data, CFStringEncoding fromEncoding, CFStringEncoding toEncoding);
};
