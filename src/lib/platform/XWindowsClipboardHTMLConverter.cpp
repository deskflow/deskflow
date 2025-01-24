/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2004 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/XWindowsClipboardHTMLConverter.h"

#include "base/Unicode.h"

//
// XWindowsClipboardHTMLConverter
//

XWindowsClipboardHTMLConverter::XWindowsClipboardHTMLConverter(Display *display, const char *name)
    : m_atom(XInternAtom(display, name, False))
{
  // do nothing
}

XWindowsClipboardHTMLConverter::~XWindowsClipboardHTMLConverter()
{
  // do nothing
}

IClipboard::EFormat XWindowsClipboardHTMLConverter::getFormat() const
{
  return IClipboard::kHTML;
}

Atom XWindowsClipboardHTMLConverter::getAtom() const
{
  return m_atom;
}

int XWindowsClipboardHTMLConverter::getDataSize() const
{
  return 8;
}

std::string XWindowsClipboardHTMLConverter::fromIClipboard(const std::string &data) const
{
  return data;
}

std::string XWindowsClipboardHTMLConverter::toIClipboard(const std::string &data) const
{
  if (Unicode::isUTF8(data)) {
    return data;
  } else {
    return Unicode::UTF16ToUTF8(data);
  }
}
