/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2004 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "platform/MSWindowsClipboard.h"

//! Convert to/from some text encoding
class MSWindowsClipboardBitmapConverter : public IMSWindowsClipboardConverter
{
public:
  MSWindowsClipboardBitmapConverter();
  virtual ~MSWindowsClipboardBitmapConverter();

  // IMSWindowsClipboardConverter overrides
  virtual IClipboard::EFormat getFormat() const;
  virtual UINT getWin32Format() const;
  virtual HANDLE fromIClipboard(const std::string &) const;
  virtual std::string toIClipboard(HANDLE) const;
};
