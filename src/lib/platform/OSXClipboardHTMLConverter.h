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
  OSXClipboardHTMLConverter() = default;
  ~OSXClipboardHTMLConverter() override = default;

  // IMSWindowsClipboardConverter overrides
  IClipboard::EFormat getFormat() const override;
  CFStringRef getOSXFormat() const override;

protected:
  // OSXClipboardAnyTextConverter overrides
  std::string doFromIClipboard(const std::string &) const override;
  std::string doToIClipboard(const std::string &) const override;

  // generic encoding converter
  static std::string convertString(const std::string &data, CFStringEncoding fromEncoding, CFStringEncoding toEncoding);
};
