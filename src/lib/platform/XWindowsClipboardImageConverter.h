/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "platform/XWindowsClipboard.h"

//! Convert Deskflow bitmaps to/from common X11 image atoms.
class XWindowsClipboardImageConverter : public IXWindowsClipboardConverter
{
public:
  XWindowsClipboardImageConverter(Display *display, const char *atomName, const char *imageFormatHint);
  ~XWindowsClipboardImageConverter() override = default;

  IClipboard::Format getFormat() const override;
  Atom getAtom() const override;
  int getDataSize() const override;
  std::string fromIClipboard(const std::string &data) const override;
  std::string toIClipboard(const std::string &data) const override;

private:
  Atom m_atom;
  const char *m_imageFormatHint;
};
