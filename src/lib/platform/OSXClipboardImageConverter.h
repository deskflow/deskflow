/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "platform/OSXClipboard.h"

//! Convert Deskflow bitmaps to/from common macOS image flavors.
class OSXClipboardImageConverter : public IOSXClipboardConverter
{
public:
  OSXClipboardImageConverter(CFStringRef osxFormat, const char *imageFormatHint);
  ~OSXClipboardImageConverter() override = default;

  IClipboard::Format getFormat() const override;
  CFStringRef getOSXFormat() const override;
  std::string fromIClipboard(const std::string &data) const override;
  std::string toIClipboard(const std::string &data) const override;

private:
  CFStringRef m_osxFormat;
  const char *m_imageFormatHint;
};
