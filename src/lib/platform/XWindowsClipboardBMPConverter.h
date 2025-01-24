/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2004 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "platform/XWindowsClipboard.h"

//! Convert to/from some text encoding
class XWindowsClipboardBMPConverter : public IXWindowsClipboardConverter
{
public:
  XWindowsClipboardBMPConverter(Display *display);
  virtual ~XWindowsClipboardBMPConverter();

  // IXWindowsClipboardConverter overrides
  virtual IClipboard::EFormat getFormat() const;
  virtual Atom getAtom() const;
  virtual int getDataSize() const;
  virtual std::string fromIClipboard(const std::string &) const;
  virtual std::string toIClipboard(const std::string &) const;

private:
  Atom m_atom;
};
