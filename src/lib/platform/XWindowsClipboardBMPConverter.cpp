/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2004 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/XWindowsClipboardBMPConverter.h"

#include "deskflow/ClipboardBitmap.h"

//
// XWindowsClipboardBMPConverter
//

XWindowsClipboardBMPConverter::XWindowsClipboardBMPConverter(Display *display)
    : m_atom(XInternAtom(display, "image/bmp", False))
{
  // do nothing
}

IClipboard::Format XWindowsClipboardBMPConverter::getFormat() const
{
  return IClipboard::Format::Bitmap;
}

Atom XWindowsClipboardBMPConverter::getAtom() const
{
  return m_atom;
}

int XWindowsClipboardBMPConverter::getDataSize() const
{
  return 8;
}

std::string XWindowsClipboardBMPConverter::fromIClipboard(const std::string &bmp) const
{
  return deskflow::bmp::dibToBmp(bmp);
}

std::string XWindowsClipboardBMPConverter::toIClipboard(const std::string &bmp) const
{
  return deskflow::bmp::bmpToDib(bmp);
}
