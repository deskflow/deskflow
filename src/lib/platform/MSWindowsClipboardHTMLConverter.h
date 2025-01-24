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
  virtual ~MSWindowsClipboardHTMLConverter();

  // IMSWindowsClipboardConverter overrides
  virtual IClipboard::EFormat getFormat() const;
  virtual UINT getWin32Format() const;

protected:
  // MSWindowsClipboardAnyTextConverter overrides
  virtual std::string doFromIClipboard(const std::string &) const;
  virtual std::string doToIClipboard(const std::string &) const;

private:
  std::string findArg(const std::string &data, const std::string &name) const;

private:
  UINT m_format;
};
