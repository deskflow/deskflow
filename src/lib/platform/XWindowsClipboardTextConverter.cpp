/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/XWindowsClipboardTextConverter.h"

#include "base/Unicode.h"

//
// XWindowsClipboardTextConverter
//

XWindowsClipboardTextConverter::XWindowsClipboardTextConverter(Display *display, const char *name)
    : m_atom(XInternAtom(display, name, False))
{
  // do nothing
}

XWindowsClipboardTextConverter::~XWindowsClipboardTextConverter()
{
  // do nothing
}

IClipboard::EFormat XWindowsClipboardTextConverter::getFormat() const
{
  return IClipboard::kText;
}

Atom XWindowsClipboardTextConverter::getAtom() const
{
  return m_atom;
}

int XWindowsClipboardTextConverter::getDataSize() const
{
  return 8;
}

std::string XWindowsClipboardTextConverter::fromIClipboard(const std::string &data) const
{
  return Unicode::UTF8ToText(data);
}

std::string XWindowsClipboardTextConverter::toIClipboard(const std::string &data) const
{
  // convert to UTF-8
  bool errors;
  std::string utf8 = Unicode::textToUTF8(data, &errors);

  // if there were decoding errors then, to support old applications
  // that don't understand UTF-8 but can report the exact binary
  // UTF-8 representation, see if the data appears to be UTF-8.  if
  // so then use it as is.
  if (errors && Unicode::isUTF8(data)) {
    return data;
  }

  return utf8;
}
