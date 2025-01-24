/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/XWindowsClipboardUTF8Converter.h"

#include <algorithm>

//
// XWindowsClipboardUTF8Converter
//

XWindowsClipboardUTF8Converter::XWindowsClipboardUTF8Converter(Display *display, const char *name, bool normalize)
    : m_atom(XInternAtom(display, name, False)),
      m_normalize(normalize)
{
  // do nothing
}

XWindowsClipboardUTF8Converter::~XWindowsClipboardUTF8Converter()
{
  // do nothing
}

IClipboard::EFormat XWindowsClipboardUTF8Converter::getFormat() const
{
  return IClipboard::kText;
}

Atom XWindowsClipboardUTF8Converter::getAtom() const
{
  return m_atom;
}

int XWindowsClipboardUTF8Converter::getDataSize() const
{
  return 8;
}

static bool isCR(char ch)
{
  return (ch == '\r');
}

std::string XWindowsClipboardUTF8Converter::fromIClipboard(const std::string &data) const
{
  return data;
}

std::string XWindowsClipboardUTF8Converter::toIClipboard(const std::string &data) const
{
  // https://bugzilla.mozilla.org/show_bug.cgi?id=1547595
  // GTK normalizes the clipboard's line endings to CRLF (\r\n) internally.
  // When sending the raw data to other systems, like Windows, where \n is
  // converted to \r\n we end up with \r\r\n, resulting in double lines when
  // pasting.
  //
  // This seems to happen only when the clipboard format is
  // text/plain;charset=utf8 and not when it's UTF8_STRING.
  // When normalize clipboard is set, any \r present in the string is removed

  if (m_normalize) {
    std::string copy = data;

    copy.erase(std::remove_if(copy.begin(), copy.end(), isCR), copy.end());
    return copy;
  }

  return data;
}
