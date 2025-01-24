/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2004 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/OSXClipboardTextConverter.h"

#include "base/Unicode.h"

//
// OSXClipboardTextConverter
//

OSXClipboardTextConverter::OSXClipboardTextConverter()
{
  // do nothing
}

OSXClipboardTextConverter::~OSXClipboardTextConverter()
{
  // do nothing
}

CFStringRef OSXClipboardTextConverter::getOSXFormat() const
{
  return CFSTR("public.plain-text");
}

std::string OSXClipboardTextConverter::convertString(
    const std::string &data, CFStringEncoding fromEncoding, CFStringEncoding toEncoding
)
{
  CFStringRef stringRef = CFStringCreateWithCString(kCFAllocatorDefault, data.c_str(), fromEncoding);

  if (stringRef == NULL) {
    return std::string();
  }

  CFIndex buffSize;
  CFRange entireString = CFRangeMake(0, CFStringGetLength(stringRef));

  CFStringGetBytes(stringRef, entireString, toEncoding, 0, false, NULL, 0, &buffSize);

  char *buffer = new char[buffSize];

  if (buffer == NULL) {
    CFRelease(stringRef);
    return std::string();
  }

  CFStringGetBytes(stringRef, entireString, toEncoding, 0, false, (uint8_t *)buffer, buffSize, NULL);

  std::string result(buffer, buffSize);

  delete[] buffer;
  CFRelease(stringRef);

  return result;
}

std::string OSXClipboardTextConverter::doFromIClipboard(const std::string &data) const
{
  return convertString(data, kCFStringEncodingUTF8, CFStringGetSystemEncoding());
}

std::string OSXClipboardTextConverter::doToIClipboard(const std::string &data) const
{
  return convertString(data, CFStringGetSystemEncoding(), kCFStringEncodingUTF8);
}
