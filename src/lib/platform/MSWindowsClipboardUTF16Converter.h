/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "platform/MSWindowsClipboardAnyTextConverter.h"

//! Convert to/from UTF-16 encoding
class MSWindowsClipboardUTF16Converter : public MSWindowsClipboardAnyTextConverter
{
public:
  MSWindowsClipboardUTF16Converter() = default;
  ~MSWindowsClipboardUTF16Converter() override = default;

  // IMSWindowsClipboardConverter overrides
  UINT getWin32Format() const override;

protected:
  // MSWindowsClipboardAnyTextConverter overrides
  std::string doFromIClipboard(const std::string &) const override;
  std::string doToIClipboard(const std::string &) const override;
};
