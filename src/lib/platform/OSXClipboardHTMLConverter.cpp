/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2014 - 2016 Symless Ltd
 * SPDX-FileCopyrightText: (C) 2014 Ryan Chapman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/OSXClipboardHTMLConverter.h"

#include "base/Unicode.h"

OSXClipboardHTMLConverter::OSXClipboardHTMLConverter()
{
  // do nothing
}

OSXClipboardHTMLConverter::~OSXClipboardHTMLConverter()
{
  // do nothing
}

IClipboard::EFormat OSXClipboardHTMLConverter::getFormat() const
{
  return IClipboard::kHTML;
}

CFStringRef OSXClipboardHTMLConverter::getOSXFormat() const
{
  return CFSTR("public.html");
}

std::string OSXClipboardHTMLConverter::convertString(
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

std::string OSXClipboardHTMLConverter::doFromIClipboard(const std::string &data) const
{
  return data;
}

std::string OSXClipboardHTMLConverter::doToIClipboard(const std::string &data) const
{
  if (Unicode::isUTF8(data)) {
    return data;
  } else {
    return convertString(data, CFStringGetSystemEncoding(), kCFStringEncodingUTF8);
  }
}
