/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2015 - 2022 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once
#include <openssl/ssl.h>
#include <string>

namespace deskflow {
namespace ssl {

class SslApi
{
public:
  explicit SslApi(bool isServer = false);
  SslApi(SslApi const &) = delete;
  SslApi &operator=(SslApi const &) = delete;
  ~SslApi();

  int read(char *buffer, int size);
  int write(const char *buffer, int size);
  int accept(int socket);
  int connect(int socket);

  bool loadCertificate(const std::string &filename);
  bool showCertificate() const;
  std::string getFingerprint() const;
  bool isTrustedFingerprint(const std::string &fingerprint) const;

  void logSecureInfo() const;
  int getErrorCode(int status) const;

private:
  void createSSL();
  void formatFingerprint(std::string &fingerprint) const;
  bool isCertificateExists(const std::string &filename) const;
  void createContext(bool isServer = false);

  SSL *m_ssl = nullptr;
  SSL_CTX *m_context = nullptr;
};

} // namespace ssl
} // namespace deskflow
