/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/MSWindowsClipboardUTF16Converter.h"

#include <QString>

//
// MSWindowsClipboardUTF16Converter
//

UINT MSWindowsClipboardUTF16Converter::getWin32Format() const
{
  return CF_UNICODETEXT;
}

std::string MSWindowsClipboardUTF16Converter::doFromIClipboard(const std::string &data) const
{
  const auto text = QString::fromUtf8(data.data(), static_cast<qsizetype>(data.size()));
  return std::string(reinterpret_cast<const char *>(text.utf16()), (text.size() + 1) * sizeof(char16_t));
}

std::string MSWindowsClipboardUTF16Converter::doToIClipboard(const std::string &data) const
{
  auto text = QStringView(reinterpret_cast<const char16_t *>(data.data()), data.size() / sizeof(char16_t));
  if (const auto nul = text.indexOf(QChar(u'\0')); nul >= 0) {
    text.truncate(nul);
  }
  if (text.startsWith(QChar(QChar::ByteOrderMark))) {
    text = text.sliced(1);
  }
  return text.toUtf8().toStdString();
}
