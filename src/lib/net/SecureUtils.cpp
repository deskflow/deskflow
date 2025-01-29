/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Devlopers
 * SPDX-FileCopyrightText: (C) 2021 Barrier Contributors
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "SecureUtils.h"
#include "base/String.h"
#include "base/finally.h"
#include "io/filesystem.h"

#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <stdexcept>

namespace deskflow {

namespace {

const EVP_MD *digestForType(FingerprintType type)
{
  switch (type) {
  case FingerprintType::SHA1:
    return EVP_sha1();
  case FingerprintType::SHA256:
    return EVP_sha256();
  }
  throw std::runtime_error("Unknown fingerprint type " + std::to_string(static_cast<int>(type)));
}

} // namespace

std::string formatSSLFingerprint(const std::vector<uint8_t> &fingerprint, bool enableSeparators)
{
  std::string rtn = deskflow::string::toHex(fingerprint, 2);

  deskflow::string::uppercase(rtn);

  if (enableSeparators) {
    size_t separators = rtn.size() / 2;
    for (size_t i = 1; i < separators; i++)
      rtn.insert(i * 3 - 1, ":");
  }
  return rtn;
}

std::vector<uint8_t> SSLCertFingerprint(X509 *cert, FingerprintType type)
{
  if (!cert) {
    throw std::runtime_error("certificate is null");
  }

  unsigned char digest[EVP_MAX_MD_SIZE];
  unsigned int digest_length = 0;
  int result = X509_digest(cert, digestForType(type), digest, &digest_length);

  if (result <= 0) {
    throw std::runtime_error("failed to calculate fingerprint, digest result: " + std::to_string(result));
  }

  std::vector<std::uint8_t> digest_vec;
  digest_vec.assign(reinterpret_cast<std::uint8_t *>(digest), reinterpret_cast<std::uint8_t *>(digest) + digest_length);
  return digest_vec;
}

std::vector<std::uint8_t> pemFileCertFingerprint(const std::string &path, FingerprintType type)
{
  auto fp = fopenUtf8Path(path, "r");
  if (!fp) {
    throw std::runtime_error("Could not open certificate path");
  }
  auto file_close = finally([fp]() { std::fclose(fp); });

  X509 *cert = PEM_read_X509(fp, nullptr, nullptr, nullptr);
  if (!cert) {
    throw std::runtime_error("Certificate could not be parsed");
  }
  auto cert_free = finally([cert]() { X509_free(cert); });

  return SSLCertFingerprint(cert, type);
}
} // namespace deskflow
