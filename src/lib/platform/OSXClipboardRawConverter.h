/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Synergy App Ltd
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "platform/OSXClipboard.h"

class OSXClipboardRawConverter : public IOSXClipboardConverter
{
public:
  OSXClipboardRawConverter(const char *uti, IClipboard::Format format);
  OSXClipboardRawConverter(const OSXClipboardRawConverter &) = delete;
  OSXClipboardRawConverter &operator=(const OSXClipboardRawConverter &) = delete;
  ~OSXClipboardRawConverter() override;

  IClipboard::Format getFormat() const override;
  CFStringRef getOSXFormat() const override;

  std::string fromIClipboard(const std::string &) const override;
  std::string toIClipboard(const std::string &) const override;

private:
  CFStringRef m_uti;
  IClipboard::Format m_format;
};
