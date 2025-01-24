/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2004 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "platform/XWindowsClipboard.h"

//! Convert to/from HTML encoding
class XWindowsClipboardHTMLConverter : public IXWindowsClipboardConverter
{
public:
  /*!
  \c name is converted to an atom and that is reported by getAtom().
  */
  XWindowsClipboardHTMLConverter(Display *display, const char *name);
  virtual ~XWindowsClipboardHTMLConverter();

  // IXWindowsClipboardConverter overrides
  virtual IClipboard::EFormat getFormat() const;
  virtual Atom getAtom() const;
  virtual int getDataSize() const;
  virtual std::string fromIClipboard(const std::string &) const;
  virtual std::string toIClipboard(const std::string &) const;

private:
  Atom m_atom;
};
