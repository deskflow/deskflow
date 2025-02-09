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

#include <algorithm>
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

FingerprintData sslCertFingerprint(X509 *cert, FingerprintType type)
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
  return {fingerprintTypeToString(type), digestVec};
}

FingerprintData pemFileCertFingerprint(const std::string &path, FingerprintType type)
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

  return sslCertFingerprint(cert, type);
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

std::string formatSSLFingerprintColumns(const std::vector<uint8_t> &fingerprint)
{
  auto kmaxColumns = 8;

  std::string hex = deskflow::string::toHex(fingerprint, 2);
  deskflow::string::uppercase(hex);
  if (hex.empty() || hex.size() % 2 != 0) {
    return hex;
  }

  std::string separated;
  for (std::size_t i = 0; i < hex.size(); i += kmaxColumns * 2) {
    for (std::size_t j = i; j < i + 16 && j < hex.size() - 1; j += 2) {
      separated.push_back(hex[j]);
      separated.push_back(hex[j + 1]);
      separated.push_back(':');
    }
    separated.push_back('\n');
  }
  separated.pop_back(); // we don't need last newline character
  return separated;
}

/*
    Draw an ASCII-Art representing the fingerprint so human brain can
    profit from its built-in pattern recognition ability.
    This technique is called "random art" and can be found in some
    scientific publications like this original paper:
    "Hash Visualization: a New Technique to improve Real-World Security",
    Perrig A. and Song D., 1999, International Workshop on Cryptographic
    Techniques and E-Commerce (CrypTEC '99)
    sparrow.ece.cmu.edu/~adrian/projects/validation/validation.pdf
    The subject came up in a talk by Dan Kaminsky, too.
    If you see the picture is different, the key is different.
    If the picture looks the same, you still know nothing.
    The algorithm used here is a worm crawling over a discrete plane,
    leaving a trace (augmenting the field) everywhere it goes.
    Movement is taken from rawDigest 2bit-wise.  Bumping into walls
    makes the respective movement vector be ignored for this turn.
    Graphs are not unambiguous, because circles in graphs can be
walked in either direction.
 */

/*
    Field sizes for the random art.  Have to be odd, so the starting point
    can be in the exact middle of the picture, and `baseSize` should be >=8 .
    Else pictures would be too dense, and drawing the frame would
    fail, too, because the key type would not fit in anymore.
*/

std::string generateFingerprintArt(const std::vector<std::uint8_t> &rawDigest)
{
  const auto baseSize = 8;
  const auto rows = (baseSize + 1);
  const auto columns = (baseSize * 2 + 1);
  const std::string characterPool = " .o+=*BOX@%&#/^SE";
  const std::size_t len = characterPool.length() - 1;

  std::uint8_t field[columns][rows];
  memset(field, 0, columns * rows * sizeof(char));
  int x = columns / 2;
  int y = rows / 2;

  /* process raw key */
  for (size_t i = 0; i < rawDigest.size(); i++) {
    /* each byte conveys four 2-bit move commands */
    int input = rawDigest[i];
    for (uint32_t b = 0; b < 4; b++) {
      /* evaluate 2 bit, rest is shifted later */
      x += (input & 0x1) ? 1 : -1;
      y += (input & 0x2) ? 1 : -1;

      /* assure we are still in bounds */
      x = std::clamp(x, 0, columns - 1);
      y = std::clamp(y, 0, rows - 1);

      /* augment the field */
      if (field[x][y] < len - 2)
        field[x][y]++;
      input = input >> 2;
    }
  }

  /* mark starting point and end point*/
  field[columns / 2][rows / 2] = len - 1;
  field[x][y] = len;

  std::string result;
  result.reserve((columns + 3) * (rows + 2));
  result.append("╔═════════════════╗\n");

  /* output content */
  for (y = 0; y < rows; y++) {
    result.append("║");
    for (x = 0; x < columns; x++)
      result.append(characterPool.substr(std::min<int>(field[x][y], len), 1));
    result.append("║\n");
  }

  result.append("╚═════════════════╝");
  return result;
}

} // namespace deskflow
