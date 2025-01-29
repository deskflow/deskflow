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

void generatePemSelfSignedCert(const std::string &path, int keyLength)
{
  auto expiration_days = 365;

  auto *private_key = EVP_PKEY_new();
  if (!private_key) {
    throw std::runtime_error("Could not allocate private key for certificate");
  }
  auto private_key_free = finally([private_key]() { EVP_PKEY_free(private_key); });

  private_key = EVP_RSA_gen(keyLength);

  auto *cert = X509_new();
  if (!cert) {
    throw std::runtime_error("Could not allocate certificate");
  }
  auto cert_free = finally([cert]() { X509_free(cert); });

  ASN1_INTEGER_set(X509_get_serialNumber(cert), 1);
  X509_gmtime_adj(X509_get_notBefore(cert), 0);
  X509_gmtime_adj(X509_get_notAfter(cert), expiration_days * 24 * 3600);
  X509_set_pubkey(cert, private_key);

  auto *name = X509_get_subject_name(cert);
  X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, reinterpret_cast<const unsigned char *>("Deskflow"), -1, -1, 0);
  X509_set_issuer_name(cert, name);

  X509_sign(cert, private_key, EVP_sha256());

  auto fp = fopenUtf8Path(path.c_str(), "w");
  if (!fp) {
    throw std::runtime_error("Could not open certificate output path");
  }
  auto file_close = finally([fp]() { std::fclose(fp); });

  PEM_write_PrivateKey(fp, private_key, nullptr, nullptr, 0, nullptr, nullptr);
  PEM_write_X509(fp, cert);
}

int getCertLength(const std::string &path)
{
  auto fp = fopenUtf8Path(path.c_str(), "r");
  if (!fp) {
    throw std::runtime_error("Could not open certificate output path");
    return -1;
  }

  EVP_PKEY *pkey = PEM_read_PrivateKey(fp, NULL, NULL, NULL);

  fclose(fp);

  if (!pkey) {
    throw std::runtime_error("Could not open certificate");
    return -1;
  }

  if (EVP_PKEY_base_id(pkey) != EVP_PKEY_RSA) {
    throw std::runtime_error("Not an RSA KEY");
    return -1;
  }
  int size = EVP_PKEY_get_bits(pkey);

  EVP_PKEY_free(pkey);

  return size;
}

} // namespace deskflow
