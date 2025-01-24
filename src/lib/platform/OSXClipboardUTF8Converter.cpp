/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2004 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "OSXClipboardUTF8Converter.h"

CFStringRef OSXClipboardUTF8Converter::getOSXFormat() const
{
  return CFSTR("public.utf8-plain-text");
}

std::string OSXClipboardUTF8Converter::doFromIClipboard(const std::string &data) const
{
  return data;
}

std::string OSXClipboardUTF8Converter::doToIClipboard(const std::string &data) const
{
  return data;
}
