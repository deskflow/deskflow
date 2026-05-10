/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2014 - 2016, 2026 Symless Ltd
 * SPDX-FileCopyrightText: (C) 2014 Ryan Chapman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "platform/OSXClipboard.h"

#include <QtTypes>

class OSXClipboardBMPConverter : public IOSXClipboardConverter
{
public:
  OSXClipboardBMPConverter() = default;
  ~OSXClipboardBMPConverter() override = default;

  IClipboard::Format getFormat() const override;
  CFStringRef getOSXFormat() const override;

  std::string fromIClipboard(const std::string &) const override;
  std::string toIClipboard(const std::string &) const override;

  static std::string convertString(const std::string &data, CFStringEncoding fromEncoding, CFStringEncoding toEncoding);

private:
  static constexpr quint32 kBiBitfields = 3;
  static constexpr quint32 kBiAlphabitfields = 6;
  static constexpr quint32 kBmpHeaderDIBPad = 10;
  static constexpr quint32 kBmpFileHeaderSize = 14;
  static constexpr quint32 kFallbackPixelOffset = 40;

  static quint32 dibPixelOffset(const quint8 *dib, qsizetype dibSize);
};
