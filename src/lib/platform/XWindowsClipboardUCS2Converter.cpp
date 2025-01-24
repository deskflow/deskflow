/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/XWindowsClipboardUCS2Converter.h"

#include "base/Unicode.h"

//
// XWindowsClipboardUCS2Converter
//

XWindowsClipboardUCS2Converter::XWindowsClipboardUCS2Converter(Display *display, const char *name)
    : m_atom(XInternAtom(display, name, False))
{
  // do nothing
}

XWindowsClipboardUCS2Converter::~XWindowsClipboardUCS2Converter()
{
  // do nothing
}

IClipboard::EFormat XWindowsClipboardUCS2Converter::getFormat() const
{
  return IClipboard::kText;
}

Atom XWindowsClipboardUCS2Converter::getAtom() const
{
  return m_atom;
}

int XWindowsClipboardUCS2Converter::getDataSize() const
{
  return 16;
}

std::string XWindowsClipboardUCS2Converter::fromIClipboard(const std::string &data) const
{
  return Unicode::UTF8ToUCS2(data);
}

std::string XWindowsClipboardUCS2Converter::toIClipboard(const std::string &data) const
{
  return Unicode::UCS2ToUTF8(data);
}
