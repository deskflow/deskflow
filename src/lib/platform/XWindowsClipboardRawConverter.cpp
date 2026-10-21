/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Synergy App Ltd
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/XWindowsClipboardRawConverter.h"

XWindowsClipboardRawConverter::XWindowsClipboardRawConverter(
    Display *display, const char *name, IClipboard::Format format
)
    : m_atom(XInternAtom(display, name, False)),
      m_format(format)
{
}

IClipboard::Format XWindowsClipboardRawConverter::getFormat() const
{
  return m_format;
}

Atom XWindowsClipboardRawConverter::getAtom() const
{
  return m_atom;
}

int XWindowsClipboardRawConverter::getDataSize() const
{
  return 8;
}

std::string XWindowsClipboardRawConverter::fromIClipboard(const std::string &data) const
{
  return data;
}

std::string XWindowsClipboardRawConverter::toIClipboard(const std::string &data) const
{
  return data;
}
