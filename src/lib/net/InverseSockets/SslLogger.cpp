/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2015 - 2022 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "SslLogger.h"
#include <iterator>
#include <sstream>

#include <base/Log.h>
#include <openssl/err.h>
#include <openssl/ssl.h>

namespace {

void showCipherStackDesc(STACK_OF(SSL_CIPHER) * stack)
{
  char msg[128] = {0};
  for (int i = 0; i < sk_SSL_CIPHER_num(stack); ++i) {
    auto cipher = sk_SSL_CIPHER_value(stack, i);
    SSL_CIPHER_description(cipher, msg, sizeof(msg));

    // SSL puts a newline in the description
    auto pos = strnlen(msg, sizeof(msg)) - 1;
    if (msg[pos] == '\n') {
      msg[pos] = '\0';
    }

    LOG((CLOG_DEBUG1 "%s", msg));
  }
}

void logLocalSecureCipherInfo(const SSL *ssl)
{
  auto sStack = SSL_get_ciphers(ssl);

  if (sStack) {
    LOG((CLOG_DEBUG1 "available local ciphers:"));
    showCipherStackDesc(sStack);
  } else {
    LOG((CLOG_DEBUG1 "local cipher list not available"));
  }
}

void logRemoteSecureCipherInfo(const SSL *ssl)
{
#if OPENSSL_VERSION_NUMBER < 0x10100000L || defined(LIBRESSL_VERSION_NUMBER)
  // ssl->session->ciphers is not forward compatable,
  // In future release of OpenSSL, it's not visible,
  // however, LibreSSL still uses this.
  auto cStack = ssl->session->ciphers;
#else
  // Use SSL_get_client_ciphers() for newer versions of OpenSSL.
  auto cStack = SSL_get_client_ciphers(ssl);
#endif
  if (cStack) {
    LOG((CLOG_DEBUG1 "available remote ciphers:"));
    showCipherStackDesc(cStack);
  } else {
    LOG((CLOG_DEBUG1 "remote cipher list not available"));
  }
}

} // namespace

void SslLogger::logSecureLibInfo()
{
  if (CLOG->getFilter() >= kDEBUG) {
    LOG((CLOG_DEBUG "openssl version: %s", SSLeay_version(SSLEAY_VERSION)));
    LOG((CLOG_DEBUG1 "openssl flags: %s", SSLeay_version(SSLEAY_CFLAGS)));
    LOG((CLOG_DEBUG1 "openssl built on: %s", SSLeay_version(SSLEAY_BUILT_ON)));
    LOG((CLOG_DEBUG1 "openssl platform: %s", SSLeay_version(SSLEAY_PLATFORM)));
    LOG((CLOG_DEBUG1 "openssl dir: %s", SSLeay_version(SSLEAY_DIR)));
  }
}

void SslLogger::logSecureCipherInfo(const SSL *ssl)
{
  if (ssl && CLOG->getFilter() >= kDEBUG1) {
    logLocalSecureCipherInfo(ssl);
    logRemoteSecureCipherInfo(ssl);
  }
}

void SslLogger::logSecureConnectInfo(const SSL *ssl)
{
  if (ssl) {
    auto cipher = SSL_get_current_cipher(ssl);

    if (cipher) {
      char msg[128] = {0};
      SSL_CIPHER_description(cipher, msg, sizeof(msg));
      LOG((CLOG_DEBUG "openssl cipher: %s", msg));

      // For some reason SSL_get_version is return mismatching information to
      // SSL_CIPHER_description
      //  so grab the version out the description instead, This seems like a
      //  hacky way of doing it. But when the cipher says "TLSv1.2" but the
      //  get_version returns "TLSv1/SSLv3" we it doesn't look right For some
      //  reason macOS hates regex's so stringstream is used
      std::istringstream iss(msg);

      // Take the stream input and splits it into a vetor directly
      const std::vector<std::string> parts{
          std::istream_iterator<std::string>{iss}, std::istream_iterator<std::string>{}
      };
      if (parts.size() > 2) {
        // log the section containing the protocol version
        LOG((CLOG_INFO "network encryption protocol: %s", parts[1].c_str()));
      } else {
        // log the error in spliting then display the whole description rather
        // then nothing
        LOG((CLOG_ERR "could not split cipher for protocol"));
        LOG((CLOG_INFO "network encryption protocol: %s", msg));
      }
    } else {
      LOG((CLOG_ERR "could not get secure socket cipher"));
    }
  }
}

void SslLogger::logError(const std::string &reason)
{
  if (!reason.empty()) {
    LOG((CLOG_ERR "secure socket error: %s", reason.c_str()));
  }

  auto id = ERR_get_error();
  if (id) {
    char error[65535] = {0};
    ERR_error_string_n(id, error, sizeof(error));
    LOG((CLOG_ERR "openssl error: %s", error));
  }
}

void SslLogger::logErrorByCode(int code, int retry)
{
  switch (code) {
  case SSL_ERROR_NONE:
    break;

  case SSL_ERROR_ZERO_RETURN:
    LOG((CLOG_DEBUG "tls connection closed"));
    break;

  case SSL_ERROR_WANT_READ:
    LOG((CLOG_DEBUG2 "want to read, error=%d, attempt=%d", code, retry));
    break;

  case SSL_ERROR_WANT_WRITE:
    LOG((CLOG_DEBUG2 "want to write, error=%d, attempt=%d", code, retry));
    break;

  case SSL_ERROR_WANT_CONNECT:
    LOG((CLOG_DEBUG2 "want to connect, error=%d, attempt=%d", code, retry));
    break;

  case SSL_ERROR_WANT_ACCEPT:
    LOG((CLOG_DEBUG2 "want to accept, error=%d, attempt=%d", code, retry));
    break;

  case SSL_ERROR_SYSCALL:
    LOG((CLOG_ERR "tls error occurred (system call failure)"));
    break;

  case SSL_ERROR_SSL:
    LOG((CLOG_ERR "tls error occurred (generic failure)"));
    break;

  default:
    LOG((CLOG_ERR "tls error occurred (unknown failure)"));
    break;
  }
}
