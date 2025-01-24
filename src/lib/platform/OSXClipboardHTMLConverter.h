/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2014 - 2016 Symless Ltd
 * SPDX-FileCopyrightText: (C) 2014 Ryan Chapman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "OSXClipboardAnyTextConverter.h"

//! Convert to/from HTML encoding
class OSXClipboardHTMLConverter : public OSXClipboardAnyTextConverter
{
public:
  OSXClipboardHTMLConverter();
  virtual ~OSXClipboardHTMLConverter();

  // IMSWindowsClipboardConverter overrides
  virtual IClipboard::EFormat getFormat() const;

  virtual CFStringRef getOSXFormat() const;

protected:
  // OSXClipboardAnyTextConverter overrides
  virtual std::string doFromIClipboard(const std::string &) const;
  virtual std::string doToIClipboard(const std::string &) const;

  // generic encoding converter
  static std::string convertString(const std::string &data, CFStringEncoding fromEncoding, CFStringEncoding toEncoding);
};
