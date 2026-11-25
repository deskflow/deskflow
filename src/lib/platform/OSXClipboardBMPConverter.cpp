/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2014 - 2016, 2023 - 2026 Symless Ltd
 * SPDX-FileCopyrightText: (C) 2014 Ryan Chapman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/OSXClipboardBMPConverter.h"

#include "deskflow/ClipboardBitmap.h"

IClipboard::Format OSXClipboardBMPConverter::getFormat() const
{
  return IClipboard::Format::Bitmap;
}

CFStringRef OSXClipboardBMPConverter::getOSXFormat() const
{
  return CFSTR("com.microsoft.bmp");
}

std::string OSXClipboardBMPConverter::fromIClipboard(const std::string &bmp) const
{
  return deskflow::bmp::dibToBmp(bmp);
}

std::string OSXClipboardBMPConverter::toIClipboard(const std::string &bmp) const
{
  return deskflow::bmp::bmpToDib(bmp);
}
