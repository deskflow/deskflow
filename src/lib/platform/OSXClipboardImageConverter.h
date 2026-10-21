/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Synergy App Ltd
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "platform/OSXClipboard.h"
#include "platform/OSXClipboardBMPConverter.h"

class OSXClipboardImageConverter : public IOSXClipboardConverter
{
public:
  explicit OSXClipboardImageConverter(const char *uti, const char *aliasUti = nullptr);
  OSXClipboardImageConverter(const OSXClipboardImageConverter &) = delete;
  OSXClipboardImageConverter &operator=(const OSXClipboardImageConverter &) = delete;
  ~OSXClipboardImageConverter() override;

  IClipboard::Format getFormat() const override;
  CFStringRef getOSXFormat() const override;
  CFStringRef getAliasOSXFormat() const override;

  std::string fromIClipboard(const std::string &) const override;
  std::string toIClipboard(const std::string &) const override;

private:
  static std::string convertImage(const std::string &image, CFStringRef toType);

  const char *m_utiName;
  CFStringRef m_uti;
  CFStringRef m_aliasUti;
  OSXClipboardBMPConverter m_bmpConverter;
};
