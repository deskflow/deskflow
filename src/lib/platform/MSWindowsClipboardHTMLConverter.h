/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2004 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "platform/MSWindowsClipboardAnyTextConverter.h"

//! Convert to/from HTML encoding
class MSWindowsClipboardHTMLConverter : public MSWindowsClipboardAnyTextConverter
{
public:
  MSWindowsClipboardHTMLConverter();
  ~MSWindowsClipboardHTMLConverter() override = default;

  // IMSWindowsClipboardConverter overrides
  IClipboard::EFormat getFormat() const override;
  UINT getWin32Format() const override;

protected:
  // MSWindowsClipboardAnyTextConverter overrides
  std::string doFromIClipboard(const std::string &) const override;
  std::string doToIClipboard(const std::string &) const override;

private:
  std::string findArg(const std::string &data, const std::string &name) const;

private:
  UINT m_format;
};
