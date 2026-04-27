/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "platform/OSXClipboard.h"

//! Convert to/from encoded macOS image pasteboard formats.
class OSXClipboardImageConverter : public IOSXClipboardConverter
{
public:
  OSXClipboardImageConverter(CFStringRef osxFormat, CFStringRef imageType);
  ~OSXClipboardImageConverter() override = default;

  IClipboard::Format getFormat() const override;
  CFStringRef getOSXFormat() const override;

  std::string fromIClipboard(const std::string &) const override;
  std::string toIClipboard(const std::string &) const override;

private:
  CFStringRef m_osxFormat;
  CFStringRef m_imageType;
};
