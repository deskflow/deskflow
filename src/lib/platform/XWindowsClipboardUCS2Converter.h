/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "platform/XWindowsClipboard.h"

//! Convert to/from UCS-2 encoding
class XWindowsClipboardUCS2Converter : public IXWindowsClipboardConverter
{
public:
  /*!
  \c name is converted to an atom and that is reported by getAtom().
  */
  XWindowsClipboardUCS2Converter(Display *display, const char *name);
  ~XWindowsClipboardUCS2Converter() override = default;

  // IXWindowsClipboardConverter overrides
  IClipboard::EFormat getFormat() const override;
  Atom getAtom() const override;
  int getDataSize() const override;
  std::string fromIClipboard(const std::string &) const override;
  std::string toIClipboard(const std::string &) const override;

private:
  Atom m_atom;
};
