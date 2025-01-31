/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

// TODO: remove dead code if not used in PR #7931

#include "encoding_utilities.h"

#include <stringapiset.h>

namespace deskflow {

// TODO: use this function in PR #7931 or delete it
std::string winWcharToUtf8(const WCHAR *utfStr)
{
  int utfLength = lstrlenW(utfStr);
  int mbLength = WideCharToMultiByte(CP_UTF8, 0, utfStr, utfLength, NULL, 0, NULL, NULL);
  std::string mbStr(mbLength, 0);
  WideCharToMultiByte(CP_UTF8, 0, utfStr, utfLength, &mbStr[0], mbLength, NULL, NULL);
  return mbStr;
}

// TODO: use this function in PR #7931 or delete it
std::vector<WCHAR> utf8ToWinChar(const std::string &str)
{
  int result_len = MultiByteToWideChar(CP_UTF8, 0, str.data(), str.size(), NULL, 0);
  std::vector<WCHAR> result;
  result.resize(result_len + 1, 0);
  MultiByteToWideChar(CP_UTF8, 0, str.data(), str.size(), result.data(), result_len);
  return result;
}

} // namespace deskflow
