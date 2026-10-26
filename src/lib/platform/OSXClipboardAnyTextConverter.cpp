/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2004 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/OSXClipboardAnyTextConverter.h"

#include <algorithm>

//
// OSXClipboardAnyTextConverter
//

IClipboard::Format OSXClipboardAnyTextConverter::getFormat() const
{
  return IClipboard::Format::Text;
}

std::string OSXClipboardAnyTextConverter::fromIClipboard(const std::string &data) const
{
  return doFromIClipboard(data);
}

std::string OSXClipboardAnyTextConverter::toIClipboard(const std::string &data) const
{
  // convert text then newlines
  return convertLinefeedToUnix(doToIClipboard(data));
}

static bool isCR(char ch)
{
  return (ch == '\r');
}

std::string OSXClipboardAnyTextConverter::convertLinefeedToUnix(const std::string &src)
{
  std::string copy = src;

  std::replace_if(copy.begin(), copy.end(), isCR, '\n');

  return copy;
}
