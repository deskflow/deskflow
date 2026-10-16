/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Synergy App Ltd
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "platform/MSWindowsClipboard.h"

class MSWindowsClipboardImageConverter : public IMSWindowsClipboardConverter
{
public:
  MSWindowsClipboardImageConverter(const wchar_t *formatName, const char *imageFormat);
  ~MSWindowsClipboardImageConverter() override = default;

  IClipboard::Format getFormat() const override;
  UINT getWin32Format() const override;
  HANDLE fromIClipboard(const std::string &) const override;
  std::string toIClipboard(HANDLE) const override;

private:
  UINT m_format;
  const char *m_imageFormat;
};
