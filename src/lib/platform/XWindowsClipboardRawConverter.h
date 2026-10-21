/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Synergy App Ltd
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "platform/XWindowsClipboard.h"

class XWindowsClipboardRawConverter : public IXWindowsClipboardConverter
{
public:
  XWindowsClipboardRawConverter(Display *display, const char *name, IClipboard::Format format);
  ~XWindowsClipboardRawConverter() override = default;

  IClipboard::Format getFormat() const override;
  Atom getAtom() const override;
  int getDataSize() const override;
  std::string fromIClipboard(const std::string &) const override;
  std::string toIClipboard(const std::string &) const override;

private:
  Atom m_atom;
  IClipboard::Format m_format;
};
