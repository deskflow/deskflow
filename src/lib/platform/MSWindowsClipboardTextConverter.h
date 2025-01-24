/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "platform/MSWindowsClipboardAnyTextConverter.h"

//! Convert to/from locale text encoding
class MSWindowsClipboardTextConverter : public MSWindowsClipboardAnyTextConverter
{
public:
  MSWindowsClipboardTextConverter();
  virtual ~MSWindowsClipboardTextConverter();

  // IMSWindowsClipboardConverter overrides
  virtual UINT getWin32Format() const;

protected:
  // MSWindowsClipboardAnyTextConverter overrides
  virtual std::string doFromIClipboard(const std::string &) const;
  virtual std::string doToIClipboard(const std::string &) const;
};
