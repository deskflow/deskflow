/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2004 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "platform/OSXClipboardAnyTextConverter.h"

class OSXClipboardUTF8Converter : public OSXClipboardAnyTextConverter
{
public:
  OSXClipboardUTF8Converter() = default;
  ~OSXClipboardUTF8Converter() override = default;

  CFStringRef getOSXFormat() const override;

private:
  std::string doFromIClipboard(const std::string &) const override;
  std::string doToIClipboard(const std::string &) const override;
};
