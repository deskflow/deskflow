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
  ~XWindowsClipboardBMPConverter() override = default;

  // IXWindowsClipboardConverter overrides
  IClipboard::EFormat getFormat() const override;
  Atom getAtom() const override;
  int getDataSize() const override;
  std::string fromIClipboard(const std::string &) const override;
  std::string toIClipboard(const std::string &) const override;

private:
  Atom m_atom;
};
