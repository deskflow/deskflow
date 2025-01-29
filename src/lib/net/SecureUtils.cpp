/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2021 Barrier Contributors
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "SecureUtils.h"
#include "base/String.h"
#include "base/finally.h"
#include "io/filesystem.h"

#include <openssl/evp.h>
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
  default:
    break;
  }
  throw std::runtime_error("Unknown fingerprint type " + std::to_string(static_cast<int>(type)));
}

} // namespace

std::string formatSSLFingerprint(const std::vector<uint8_t> &fingerprint, bool enableSeparators)
{
  std::string result = deskflow::string::toHex(fingerprint, 2);

  deskflow::string::uppercase(result);

  if (enableSeparators) {
    const auto usedSpaces = 3;
    size_t separators = result.size() / 2;
    for (size_t i = 1; i < separators; i++)
      result.insert(i * usedSpaces - 1, ":");
  }
  return result;
}

std::vector<uint8_t> SSLCertFingerprint(X509 *cert, FingerprintType type)
{
  if (!cert) {
    throw std::runtime_error("certificate is null");
  }

  unsigned char digest[EVP_MAX_MD_SIZE];
  unsigned int digestLength = 0;
  int result = X509_digest(cert, digestForType(type), digest, &digestLength);

  if (result <= 0) {
    throw std::runtime_error("failed to calculate fingerprint, digest result: " + std::to_string(result));
  }

  std::vector<std::uint8_t> digestVec;
  digestVec.assign(reinterpret_cast<std::uint8_t *>(digest), reinterpret_cast<std::uint8_t *>(digest) + digestLength);
  return digestVec;
}

std::vector<std::uint8_t> pemFileCertFingerprint(const std::string &path, FingerprintType type)
{
  auto fp = fopenUtf8Path(path, "r");
  if (!fp) {
    throw std::runtime_error("could not open certificate path");
  }
  auto fileClose = finally([fp]() { std::fclose(fp); });

  X509 *cert = PEM_read_X509(fp, nullptr, nullptr, nullptr);
  if (!cert) {
    throw std::runtime_error("certificate could not be parsed");
  }
  auto certFree = finally([cert]() { X509_free(cert); });

  return SSLCertFingerprint(cert, type);
}

void generatePemSelfSignedCert(const std::string &path, int keyLength)
{
  auto expirationDays = 365;

  auto *privateKey = EVP_PKEY_new();
  if (!privateKey) {
    throw std::runtime_error("could not allocate private key for certificate");
  }
  auto privateKeyFree = finally([privateKey]() { EVP_PKEY_free(privateKey); });

  privateKey = EVP_RSA_gen(keyLength);

  auto *cert = X509_new();
  if (!cert) {
    throw std::runtime_error("could not allocate certificate");
  }
  auto certFree = finally([cert]() { X509_free(cert); });

  ASN1_INTEGER_set(X509_get_serialNumber(cert), 1);
  X509_gmtime_adj(X509_get_notBefore(cert), 0);
  X509_gmtime_adj(X509_get_notAfter(cert), expirationDays * 24 * 3600);
  X509_set_pubkey(cert, privateKey);

  auto *name = X509_get_subject_name(cert);
  X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, reinterpret_cast<const unsigned char *>("Deskflow"), -1, -1, 0);
  X509_set_issuer_name(cert, name);

  X509_sign(cert, privateKey, EVP_sha256());

  auto fp = fopenUtf8Path(path.c_str(), "w");
  if (!fp) {
    throw std::runtime_error("could not open certificate output path");
  }
  auto fileClose = finally([fp]() { std::fclose(fp); });

  PEM_write_PrivateKey(fp, privateKey, nullptr, nullptr, 0, nullptr, nullptr);
  PEM_write_X509(fp, cert);
}

int getCertLength(const std::string &path)
{
  auto fp = fopenUtf8Path(path.c_str(), "r");
  if (!fp) {
    throw std::runtime_error("could not open certificate output path");
    return -1;
  }

  EVP_PKEY *privateKey = PEM_read_PrivateKey(fp, NULL, NULL, NULL);

  fclose(fp);

  if (!privateKey) {
    throw std::runtime_error("could not open certificate");
    return -1;
  }

  if (EVP_PKEY_base_id(privateKey) != EVP_PKEY_RSA) {
    throw std::runtime_error("not an RSA key");
    return -1;
  }
  int size = EVP_PKEY_get_bits(privateKey);

  EVP_PKEY_free(privateKey);

  return size;
}

} // namespace deskflow
