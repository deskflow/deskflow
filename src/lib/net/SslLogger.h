/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2015 - 2022 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once
#include <openssl/ssl.h>
#include <string>

class SslLogger
{
public:
  static void logSecureLibInfo();
  static void logSecureCipherInfo(const SSL *ssl);
  static void logSecureConnectInfo(const SSL *ssl);
  static void logError(const std::string &reason = "");
  static void logErrorByCode(int code, int retry);
};
