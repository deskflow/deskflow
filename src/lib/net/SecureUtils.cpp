/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2021 Barrier Contributors
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "SecureUtils.h"

#include "base/FinalAction.h"
#include "io/Filesystem.h"

#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>

#include <algorithm>
#include <stdexcept>

namespace deskflow {

namespace {

const EVP_MD *digestForType(Fingerprint::Type type)
{
  switch (type) {
  case Fingerprint::Type::SHA1:
    return EVP_sha1();
  case Fingerprint::Type::SHA256:
    return EVP_sha256();
  default:
    break;
  }
  throw std::runtime_error("Unknown fingerprint type " + std::to_string(static_cast<int>(type)));
}

} // namespace

QString formatSSLFingerprint(const QByteArray &fingerprint, bool enableSeparators)
{
  if (enableSeparators)
    return fingerprint.toHex(':').toUpper();
  else
    return fingerprint.toHex().toUpper();
}

Fingerprint sslCertFingerprint(const X509 *cert, Fingerprint::Type type)
{
  if (!cert) {
    throw std::runtime_error("certificate is null");
  }

  unsigned char digest[EVP_MAX_MD_SIZE];
  unsigned int digestLength = 0;

  if (int result = X509_digest(cert, digestForType(type), digest, &digestLength); result <= 0) {
    throw std::runtime_error("failed to calculate fingerprint, digest result: " + std::to_string(result));
  }

  QByteArray digestArray(reinterpret_cast<const char *>(digest), digestLength);
  return {type, digestArray};
}

Fingerprint pemFileCertFingerprint(const std::string &path, Fingerprint::Type type)
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

  EVP_PKEY *privateKey = PEM_read_PrivateKey(fp, nullptr, nullptr, nullptr);

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

QString formatSSLFingerprintColumns(const QByteArray &fingerprint)
{
  auto kMaxColumns = 24;

  QString hex = fingerprint.toHex(':').toUpper();
  if (hex.isEmpty()) {
    return hex;
  }

  QString formattedString;
  while (!hex.isEmpty()) {
    formattedString.append(hex.first(kMaxColumns));
    hex.remove(0, kMaxColumns);
    if (formattedString.endsWith(':'))
      formattedString.removeLast();
    formattedString.append('\n');
  }
  formattedString.removeLast();
  return formattedString;
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

QString generateFingerprintArt(const QByteArray &rawDigest)
{
  const auto baseSize = 8;
  const auto rows = (baseSize + 1);
  const auto columns = (baseSize * 2 + 1);
  const QString characterPool = " .o+=*BOX@%&#/^SE";
  const std::size_t len = characterPool.length() - 1;

  std::uint8_t field[columns][rows];
  memset(field, 0, columns * rows * sizeof(char));
  int x = columns / 2;
  int y = rows / 2;

  /* process raw key */
  for (int byte : rawDigest) {
    /* each byte conveys four 2-bit move commands */
    for (uint32_t b = 0; b < 4; b++) {
      /* evaluate 2 bit, rest is shifted later */
      x += (byte & 0x1) ? 1 : -1;
      y += (byte & 0x2) ? 1 : -1;

      /* assure we are still in bounds */
      x = std::clamp(x, 0, columns - 1);
      y = std::clamp(y, 0, rows - 1);

      /* augment the field */
      if (field[x][y] < len - 2)
        field[x][y]++;
      byte = byte >> 2;
    }
  }

  /* mark starting point and end point*/
  field[columns / 2][rows / 2] = len - 1;
  field[x][y] = len;

  QString result;
  result.reserve((columns + 3) * (rows + 2));
  result.append("╔═════════════════╗\n");

  /* output content */
  for (y = 0; y < rows; y++) {
    result.append("║");
    for (x = 0; x < columns; x++)
      result.append(characterPool.at(std::min<int>(field[x][y], len)));
    result.append("║\n");
  }

  result.append("╚═════════════════╝");
  return result;
}

} // namespace deskflow
