/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/XWindowsClipboardImageConverter.h"

#include "platform/ClipboardImageConverter.h"

XWindowsClipboardImageConverter::XWindowsClipboardImageConverter(
    Display *display, const char *atomName, const char *imageFormatHint
)
    : m_atom(XInternAtom(display, atomName, False)),
      m_imageFormatHint(imageFormatHint)
{
}

IClipboard::Format XWindowsClipboardImageConverter::getFormat() const
{
  return IClipboard::Format::Bitmap;
}

Atom XWindowsClipboardImageConverter::getAtom() const
{
  return m_atom;
}

int XWindowsClipboardImageConverter::getDataSize() const
{
  return 8;
}

std::string XWindowsClipboardImageConverter::fromIClipboard(const std::string &data) const
{
  return deskflow::platform::clipboard::encodeBitmapToImage(data, m_imageFormatHint);
}

std::string XWindowsClipboardImageConverter::toIClipboard(const std::string &data) const
{
  return deskflow::platform::clipboard::decodeImageToBitmap(data, m_imageFormatHint);
}
