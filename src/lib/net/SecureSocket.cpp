/*
 * Deskflow -- mouse and keyboard sharing utility
 * Copyright (C) 2015-2016 Symless Ltd.
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "SecureSocket.h"

#include "arch/XArch.h"
#include "base/Log.h"
#include "base/Path.h"
#include "base/TMethodEventJob.h"
#include "mt/Lock.h"
#include "net/TCPSocket.h"
#include "net/TSocketMultiplexerMethodJob.h"
#include <net/InverseSockets/SslLogger.h>

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iterator>
#include <memory>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <sstream>

//
// SecureSocket
//

#define MAX_ERROR_SIZE 65535

static const float s_retryDelay = 0.01f;

enum
{
  kMsgSize = 128
};

// TODO: Reduce duplication of these strings between here and TlsFingerprint.cpp
static const char kFingerprintDirName[] = "tls";
static const char kFingerprintTrustedServersFilename[] = "trusted-servers";

struct Ssl
{
  SSL_CTX *m_context;
  SSL *m_ssl;
};

SecureSocket::SecureSocket(
    IEventQueue *events, SocketMultiplexer *socketMultiplexer, IArchNetwork::EAddressFamily family
)
    : TCPSocket(events, socketMultiplexer, family),
      m_ssl(nullptr),
      m_secureReady(false),
      m_fatal(false)
{
}

SecureSocket::SecureSocket(IEventQueue *events, SocketMultiplexer *socketMultiplexer, ArchSocket socket)
    : TCPSocket(events, socketMultiplexer, socket),
      m_ssl(nullptr),
      m_secureReady(false),
      m_fatal(false)
{
}

SecureSocket::~SecureSocket()
{
  freeSSL();
}

void SecureSocket::close()
{
  freeSSL();
  TCPSocket::close();
}

void SecureSocket::connect(const NetworkAddress &addr)
{
  m_events->adoptHandler(
      m_events->forIDataSocket().connected(), getEventTarget(),
      new TMethodEventJob<SecureSocket>(this, &SecureSocket::handleTCPConnected)
  );

  TCPSocket::connect(addr);
}

ISocketMultiplexerJob *SecureSocket::newJob()
{
  // after TCP connection is established, SecureSocket will pick up
  // connected event and do secureConnect
  if (m_connected && !m_secureReady) {
    return NULL;
  }

  return TCPSocket::newJob();
}

void SecureSocket::secureConnect()
{
  setJob(new TSocketMultiplexerMethodJob<SecureSocket>(
      this, &SecureSocket::serviceConnect, getSocket(), isReadable(), isWritable()
  ));
}

void SecureSocket::secureAccept()
{
  setJob(new TSocketMultiplexerMethodJob<SecureSocket>(
      this, &SecureSocket::serviceAccept, getSocket(), isReadable(), isWritable()
  ));
}

TCPSocket::EJobResult SecureSocket::doRead()
{
  static UInt8 buffer[4096];
  memset(buffer, 0, sizeof(buffer));
  int bytesRead = 0;
  int status = 0;

  if (isSecureReady()) {
    status = secureRead(buffer, sizeof(buffer), bytesRead);
    if (status < 0) {
      return kBreak;
    } else if (status == 0) {
      return kNew;
    }
  } else {
    return kRetry;
  }

  if (bytesRead > 0) {
    bool wasEmpty = (m_inputBuffer.getSize() == 0);

    // slurp up as much as possible
    do {
      m_inputBuffer.write(buffer, bytesRead);

      status = secureRead(buffer, sizeof(buffer), bytesRead);
      if (status < 0) {
        return kBreak;
      }
    } while (bytesRead > 0 || status > 0);

    // send input ready if input buffer was empty
    if (wasEmpty) {
      sendEvent(m_events->forIStream().inputReady());
    }
  } else {
    // remote write end of stream hungup.  our input side
    // has therefore shutdown but don't flush our buffer
    // since there's still data to be read.
    sendEvent(m_events->forIStream().inputShutdown());
    if (!m_writable && m_inputBuffer.getSize() == 0) {
      sendEvent(m_events->forISocket().disconnected());
      m_connected = false;
    }
    m_readable = false;
    return kNew;
  }

  return kRetry;
}

TCPSocket::EJobResult SecureSocket::doWrite()
{
  static bool s_retry = false;
  static int s_retrySize = 0;
  static int s_staticBufferSize = 0;
  static void *s_staticBuffer = NULL;

  // write data
  int bufferSize = 0;
  int bytesWrote = 0;
  int status = 0;

  if (s_retry) {
    bufferSize = s_retrySize;
  } else {
    bufferSize = m_outputBuffer.getSize();
    if (bufferSize != 0) {
      if (bufferSize > s_staticBufferSize) {
        s_staticBuffer = realloc(s_staticBuffer, bufferSize);
        s_staticBufferSize = bufferSize;
      }
      memcpy(s_staticBuffer, m_outputBuffer.peek(bufferSize), bufferSize);
    }
  }

  if (bufferSize == 0) {
    return kRetry;
  }

  if (isSecureReady()) {
    status = secureWrite(s_staticBuffer, bufferSize, bytesWrote);
    if (status > 0) {
      s_retry = false;
      bufferSize = 0;
    } else if (status < 0) {
      return kBreak;
    } else if (status == 0) {
      s_retry = true;
      s_retrySize = bufferSize;
      return kNew;
    }
  } else {
    return kRetry;
  }

  if (bytesWrote > 0) {
    discardWrittenData(bytesWrote);
    return kNew;
  }

  return kRetry;
}

int SecureSocket::secureRead(void *buffer, int size, int &read)
{
  if (m_ssl->m_ssl != NULL) {
    LOG((CLOG_DEBUG2 "reading secure socket"));
    read = SSL_read(m_ssl->m_ssl, buffer, size);

    static int retry;

    // Check result will cleanup the connection in the case of a fatal
    checkResult(read, retry);

    if (retry) {
      return 0;
    }

    if (isFatal()) {
      return -1;
    }
  }
  // According to SSL spec, the number of bytes read must not be negative and
  // not have an error code from SSL_get_error(). If this happens, it is
  // itself an error. Let the parent handle the case
  return read;
}

int SecureSocket::secureWrite(const void *buffer, int size, int &wrote)
{
  if (m_ssl->m_ssl != NULL) {
    LOG((CLOG_DEBUG2 "writing secure socket: %p", this));

    wrote = SSL_write(m_ssl->m_ssl, buffer, size);

    static int retry;

    // Check result will cleanup the connection in the case of a fatal
    checkResult(wrote, retry);

    if (retry) {
      return 0;
    }

    if (isFatal()) {
      return -1;
    }
  }
  // According to SSL spec, r must not be negative and not have an error code
  // from SSL_get_error(). If this happens, it is itself an error. Let the
  // parent handle the case
  return wrote;
}

bool SecureSocket::isSecureReady()
{
  return m_secureReady;
}

void SecureSocket::initSsl(bool server)
{
  m_ssl = new Ssl();
  m_ssl->m_context = NULL;
  m_ssl->m_ssl = NULL;

  initContext(server);
}

bool SecureSocket::loadCertificates(String &filename)
{
  if (filename.empty()) {
    SslLogger::logError("tls certificate is not specified");
    return false;
  } else {
    std::ifstream file(deskflow::filesystem::path(filename));
    bool exist = file.good();
    file.close();

    if (!exist) {
      String errorMsg("tls certificate doesn't exist: ");
      errorMsg.append(filename);
      SslLogger::logError(errorMsg.c_str());
      return false;
    }
  }

  int r = 0;
  r = SSL_CTX_use_certificate_file(m_ssl->m_context, filename.c_str(), SSL_FILETYPE_PEM);
  if (r <= 0) {
    SslLogger::logError("could not use tls certificate");
    return false;
  }

  r = SSL_CTX_use_PrivateKey_file(m_ssl->m_context, filename.c_str(), SSL_FILETYPE_PEM);
  if (r <= 0) {
    SslLogger::logError("could not use tls private key");
    return false;
  }

  r = SSL_CTX_check_private_key(m_ssl->m_context);
  if (!r) {
    SslLogger::logError("could not verify tls private key");
    return false;
  }

  return true;
}

void SecureSocket::initContext(bool server)
{
  SSL_library_init();

  const SSL_METHOD *method;

  // load & register all cryptos, etc.
  OpenSSL_add_all_algorithms();

  // load all error messages
  SSL_load_error_strings();
  SslLogger::logSecureLibInfo();

  if (server) {
    method = SSLv23_server_method();
  } else {
    method = SSLv23_client_method();
  }

  // create new context from method
  SSL_METHOD *m = const_cast<SSL_METHOD *>(method);
  m_ssl->m_context = SSL_CTX_new(m);

  // Prevent the usage of of all version prior to TLSv1.2 as they are known to
  // be vulnerable
  SSL_CTX_set_options(m_ssl->m_context, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1);

  if (m_ssl->m_context == NULL) {
    SslLogger::logError();
  }
}

void SecureSocket::createSSL()
{
  // I assume just one instance is needed
  // get new SSL state with context
  if (m_ssl->m_ssl == NULL) {
    assert(m_ssl->m_context != NULL);
    m_ssl->m_ssl = SSL_new(m_ssl->m_context);
  }
}

void SecureSocket::freeSSL()
{
  isFatal(true);
  // take socket from multiplexer ASAP otherwise the race condition
  // could cause events to get called on a dead object. TCPSocket
  // will do this, too, but the double-call is harmless
  setJob(NULL);
  if (m_ssl) {
    if (m_ssl->m_ssl != NULL) {
      SSL_shutdown(m_ssl->m_ssl);

      SSL_free(m_ssl->m_ssl);
      m_ssl->m_ssl = NULL;
    }
    if (m_ssl->m_context != NULL) {
      SSL_CTX_free(m_ssl->m_context);
      m_ssl->m_context = NULL;
    }
    delete m_ssl;
    m_ssl = nullptr;
  }
}

int SecureSocket::secureAccept(int socket)
{
  createSSL();

  // set connection socket to SSL state
  SSL_set_fd(m_ssl->m_ssl, socket);

  LOG((CLOG_DEBUG2 "accepting secure socket"));
  int r = SSL_accept(m_ssl->m_ssl);

  static int retry;

  checkResult(r, retry);

  if (isFatal()) {
    // tell user and sleep so the socket isn't hammered.
    LOG((CLOG_ERR "failed to accept secure socket"));
    LOG((CLOG_WARN "client connection may not be secure"));
    m_secureReady = false;
    ARCH->sleep(1);
    retry = 0;
    return -1; // Failed, error out
  }

  // If not fatal and no retry, state is good
  if (retry == 0) {
    m_secureReady = true;
    LOG((CLOG_INFO "accepted secure socket"));
    SslLogger::logSecureCipherInfo(m_ssl->m_ssl);
    SslLogger::logSecureConnectInfo(m_ssl->m_ssl);
    return 1;
  }

  // If not fatal and retry is set, not ready, and return retry
  if (retry > 0) {
    LOG((CLOG_DEBUG2 "retry accepting secure socket"));
    m_secureReady = false;
    ARCH->sleep(s_retryDelay);
    return 0;
  }

  // no good state exists here
  LOG((CLOG_ERR "unexpected state attempting to accept connection"));
  return -1;
}

int SecureSocket::secureConnect(int socket)
{
  createSSL();

  // attach the socket descriptor
  SSL_set_fd(m_ssl->m_ssl, socket);

  LOG((CLOG_DEBUG2 "connecting secure socket"));

  // TODO: S1-1766, enable hostname verification.
  // the cert will need to be installed in the trusted store on the client.
  // we'll probably need to find a way of securely transferring the cert.
  int r = SSL_connect(m_ssl->m_ssl);

  static int retry;

  checkResult(r, retry);

  if (isFatal()) {
    LOG((CLOG_ERR "failed to connect secure socket"));
    retry = 0;
    return -1;
  }

  // If we should retry, not ready and return 0
  if (retry > 0) {
    LOG((CLOG_DEBUG2 "retry connect secure socket"));
    m_secureReady = false;
    ARCH->sleep(s_retryDelay);
    return 0;
  }

  retry = 0;
  // No error, set ready, process and return ok
  m_secureReady = true;
  if (verifyCertFingerprint()) {
    LOG((CLOG_INFO "connected to secure socket"));
    if (!showCertificate()) {
      disconnect();
      return -1; // Cert fail, error
    }
  } else {
    LOG((CLOG_ERR "failed to verify server certificate fingerprint"));
    disconnect();
    return -1; // Fingerprint failed, error
  }
  LOG((CLOG_DEBUG2 "connected secure socket"));
  SslLogger::logSecureCipherInfo(m_ssl->m_ssl);
  SslLogger::logSecureConnectInfo(m_ssl->m_ssl);
  return 1;
}

bool SecureSocket::showCertificate() const
{
  X509 *cert;
  char *line;

  // get the server's certificate
  cert = SSL_get_peer_certificate(m_ssl->m_ssl);
  if (cert != NULL) {
    line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
    LOG((CLOG_INFO "server tls certificate info: %s", line));
    OPENSSL_free(line);
    X509_free(cert);
  } else {
    SslLogger::logError("server has no tls certificate");
    return false;
  }

  return true;
}

void SecureSocket::checkResult(int status, int &retry)
{
  // ssl errors are a little quirky. the "want" errors are normal and
  // should result in a retry.

  int errorCode = SSL_get_error(m_ssl->m_ssl, status);

  switch (errorCode) {
  case SSL_ERROR_NONE:
    retry = 0;
    // operation completed
    break;

  case SSL_ERROR_ZERO_RETURN:
    // connection closed
    isFatal(true);
    LOG((CLOG_DEBUG "tls connection closed"));
    break;

  case SSL_ERROR_WANT_READ:
    retry++;
    LOG((CLOG_DEBUG2 "want to read, error=%d, attempt=%d", errorCode, retry));
    break;

  case SSL_ERROR_WANT_WRITE:
    // Need to make sure the socket is known to be writable so the impending
    // select action actually triggers on a write. This isn't necessary for
    // m_readable because the socket logic is always readable
    m_writable = true;
    retry++;
    LOG((CLOG_DEBUG2 "want to write, error=%d, attempt=%d", errorCode, retry));
    break;

  case SSL_ERROR_WANT_CONNECT:
    retry++;
    LOG((CLOG_DEBUG2 "want to connect, error=%d, attempt=%d", errorCode, retry));
    break;

  case SSL_ERROR_WANT_ACCEPT:
    retry++;
    LOG((CLOG_DEBUG2 "want to accept, error=%d, attempt=%d", errorCode, retry));
    break;

  case SSL_ERROR_SYSCALL:
    LOG((CLOG_ERR "tls error occurred (system call failure)"));
    if (ERR_peek_error() == 0) {
      if (status == 0) {
        LOG((CLOG_ERR "eof violates tls protocol"));
      } else if (status == -1) {
        // underlying socket I/O reproted an error
        try {
          ARCH->throwErrorOnSocket(getSocket());
        } catch (XArchNetwork &e) {
          LOG((CLOG_ERR "%s", e.what()));
        }
      }
    }

    isFatal(true);
    break;

  case SSL_ERROR_SSL:
    LOG((CLOG_ERR "tls error occurred (generic failure)"));
    isFatal(true);
    break;

  default:
    LOG((CLOG_ERR "tls error occurred (unknown failure)"));
    isFatal(true);
    break;
  }

  if (isFatal()) {
    retry = 0;
    SslLogger::logError();
    disconnect();
  }
}

void SecureSocket::disconnect()
{
  sendEvent(getEvents()->forISocket().stopRetry());
  sendEvent(getEvents()->forISocket().disconnected());
  sendEvent(getEvents()->forIStream().inputShutdown());
}

void SecureSocket::formatFingerprint(String &fingerprint, bool hex, bool separator)
{
  if (hex) {
    // to hexidecimal
    deskflow::string::toHex(fingerprint, 2);
  }

  // all uppercase
  deskflow::string::uppercase(fingerprint);

  if (separator) {
    // add colon to separate each 2 charactors
    size_t separators = fingerprint.size() / 2;
    for (size_t i = 1; i < separators; i++) {
      fingerprint.insert(i * 3 - 1, ":");
    }
  }
}

bool SecureSocket::verifyCertFingerprint()
{
  // calculate received certificate fingerprint
  using AutoX509 = std::unique_ptr<X509, decltype(&X509_free)>;
  AutoX509 cert(SSL_get_peer_certificate(m_ssl->m_ssl), &X509_free);

  unsigned char tempFingerprint[EVP_MAX_MD_SIZE];
  unsigned int tempFingerprintLen;
  int digestResult = X509_digest(cert.get(), EVP_sha256(), tempFingerprint, &tempFingerprintLen);

  if (digestResult <= 0) {
    LOG((CLOG_ERR "failed to calculate fingerprint, digest result: %d", digestResult));
    return false;
  }

  // format fingerprint into hexdecimal format with colon separator
  String fingerprint(static_cast<char *>(static_cast<void *>(tempFingerprint)), tempFingerprintLen);
  formatFingerprint(fingerprint);
  LOG((CLOG_NOTE "server fingerprint: %s", fingerprint.c_str()));

  String trustedServersFilename;
  trustedServersFilename = deskflow::string::sprintf(
      "%s/%s/%s", ARCH->getProfileDirectory().c_str(), kFingerprintDirName, kFingerprintTrustedServersFilename
  );

  // check if this fingerprint exist
  String fileLine;
  std::ifstream file;
  file.open(deskflow::filesystem::path(trustedServersFilename));

  bool isValid = false;
  if (file.is_open()) {
    while (!file.eof()) {
      getline(file, fileLine);
      if (!fileLine.empty() && !fileLine.compare(fingerprint)) {
        isValid = true;
        break;
      }
    }
  } else {
    LOG((CLOG_ERR "fail to open trusted fingerprints file: %s", trustedServersFilename.c_str()));
  }

  file.close();
  return isValid;
}

ISocketMultiplexerJob *SecureSocket::serviceConnect(ISocketMultiplexerJob *job, bool, bool write, bool error)
{
  Lock lock(&getMutex());

  int status = 0;
#ifdef SYSAPI_WIN32
  status = secureConnect(static_cast<int>(getSocket()->m_socket));
#elif SYSAPI_UNIX
  status = secureConnect(getSocket()->m_fd);
#endif

  // If status < 0, error happened
  if (status < 0) {
    return NULL;
  }

  // If status > 0, success
  if (status > 0) {
    sendEvent(m_events->forIDataSocket().secureConnected());
    return newJob();
  }

  // Retry case
  return new TSocketMultiplexerMethodJob<SecureSocket>(
      this, &SecureSocket::serviceConnect, getSocket(), isReadable(), isWritable()
  );
}

ISocketMultiplexerJob *SecureSocket::serviceAccept(ISocketMultiplexerJob *job, bool, bool write, bool error)
{
  Lock lock(&getMutex());

  int status = 0;
#ifdef SYSAPI_WIN32
  status = secureAccept(static_cast<int>(getSocket()->m_socket));
#elif SYSAPI_UNIX
  status = secureAccept(getSocket()->m_fd);
#endif
  // If status < 0, error happened
  if (status < 0) {
    return NULL;
  }

  // If status > 0, success
  if (status > 0) {
    sendEvent(m_events->forClientListener().accepted());
    return newJob();
  }

  // Retry case
  return new TSocketMultiplexerMethodJob<SecureSocket>(
      this, &SecureSocket::serviceAccept, getSocket(), isReadable(), isWritable()
  );
}

void SecureSocket::handleTCPConnected(const Event &, void *)
{
  if (getSocket() == nullptr) {
    LOG((CLOG_DEBUG "disregarding stale connect event"));
    return;
  }
  secureConnect();
}
