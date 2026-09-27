/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Synergy App Ltd
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/OSXClipboardRawConverter.h"

OSXClipboardRawConverter::OSXClipboardRawConverter(const char *uti, IClipboard::Format format)
    : m_uti(CFStringCreateWithCString(kCFAllocatorDefault, uti, kCFStringEncodingUTF8)),
      m_format(format)
{
}

OSXClipboardRawConverter::~OSXClipboardRawConverter()
{
  CFRelease(m_uti);
}

IClipboard::Format OSXClipboardRawConverter::getFormat() const
{
  return m_format;
}

CFStringRef OSXClipboardRawConverter::getOSXFormat() const
{
  return m_uti;
}

std::string OSXClipboardRawConverter::fromIClipboard(const std::string &data) const
{
  return data;
}

std::string OSXClipboardRawConverter::toIClipboard(const std::string &data) const
{
  return data;
}
