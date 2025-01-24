/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2004 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "platform/OSXClipboardAnyTextConverter.h"

//! Convert to/from locale text encoding
class OSXClipboardTextConverter : public OSXClipboardAnyTextConverter
{
public:
  OSXClipboardTextConverter();
  virtual ~OSXClipboardTextConverter();

  // IOSXClipboardAnyTextConverter overrides
  virtual CFStringRef getOSXFormat() const;

protected:
  // OSXClipboardAnyTextConverter overrides
  virtual std::string doFromIClipboard(const std::string &) const;
  virtual std::string doToIClipboard(const std::string &) const;

  // generic encoding converter
  static std::string convertString(const std::string &data, CFStringEncoding fromEncoding, CFStringEncoding toEncoding);
};
