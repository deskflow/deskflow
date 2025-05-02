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
  OSXClipboardTextConverter() = default;
  ~OSXClipboardTextConverter() override = default;

  // IOSXClipboardAnyTextConverter overrides
  CFStringRef getOSXFormat() const override;

protected:
  // OSXClipboardAnyTextConverter overrides
  std::string doFromIClipboard(const std::string &) const override;
  std::string doToIClipboard(const std::string &) const override;

  // generic encoding converter
  static std::string convertString(const std::string &data, CFStringEncoding fromEncoding, CFStringEncoding toEncoding);
};
