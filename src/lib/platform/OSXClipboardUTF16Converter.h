/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2004 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "platform/OSXClipboardAnyTextConverter.h"

//! Convert to/from UTF-16 encoding
class OSXClipboardUTF16Converter : public OSXClipboardAnyTextConverter
{
public:
  OSXClipboardUTF16Converter();
  virtual ~OSXClipboardUTF16Converter();

  // IOSXClipboardAnyTextConverter overrides
  virtual CFStringRef getOSXFormat() const;

protected:
  // OSXClipboardAnyTextConverter overrides
  virtual std::string doFromIClipboard(const std::string &) const;
  virtual std::string doToIClipboard(const std::string &) const;
};
