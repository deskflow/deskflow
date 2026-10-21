/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Synergy App Ltd
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "platform/MSWindowsClipboard.h"

class MSWindowsClipboardRawConverter : public IMSWindowsClipboardConverter
{
public:
  MSWindowsClipboardRawConverter(const wchar_t *formatName, IClipboard::Format format);
  MSWindowsClipboardRawConverter(UINT win32Format, IClipboard::Format format);
  ~MSWindowsClipboardRawConverter() override = default;

  IClipboard::Format getFormat() const override;
  UINT getWin32Format() const override;
  HANDLE fromIClipboard(const std::string &) const override;
  std::string toIClipboard(HANDLE) const override;

private:
  UINT m_win32Format;
  IClipboard::Format m_format;
};
