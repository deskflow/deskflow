/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2004 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/OSXClipboardUTF16Converter.h"

#include "base/Unicode.h"

//
// OSXClipboardUTF16Converter
//

OSXClipboardUTF16Converter::OSXClipboardUTF16Converter()
{
  // do nothing
}

OSXClipboardUTF16Converter::~OSXClipboardUTF16Converter()
{
  // do nothing
}

CFStringRef OSXClipboardUTF16Converter::getOSXFormat() const
{
  return CFSTR("public.utf16-plain-text");
}

std::string OSXClipboardUTF16Converter::doFromIClipboard(const std::string &data) const
{
  // convert and add nul terminator
  return Unicode::UTF8ToUTF16(data);
}

std::string OSXClipboardUTF16Converter::doToIClipboard(const std::string &data) const
{
  // convert and strip nul terminator
  return Unicode::UTF16ToUTF8(data);
}
