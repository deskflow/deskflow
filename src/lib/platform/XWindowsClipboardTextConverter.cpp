/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/XWindowsClipboardTextConverter.h"

#include <QString>

//
// XWindowsClipboardTextConverter
//

XWindowsClipboardTextConverter::XWindowsClipboardTextConverter(Display *display, const char *name)
    : m_atom(XInternAtom(display, name, False))
{
  // do nothing
}

IClipboard::Format XWindowsClipboardTextConverter::getFormat() const
{
  return IClipboard::Format::Text;
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
  return QString::fromStdString(data).toLatin1().toStdString();
}

std::string XWindowsClipboardTextConverter::toIClipboard(const std::string &data) const
{
  return QString::fromLatin1(data).toUtf8().toStdString();
}
