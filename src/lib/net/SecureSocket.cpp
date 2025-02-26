/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2015 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "SecureSocket.h"
#include "SecureUtils.h"

#include "arch/XArch.h"
#include "base/Log.h"
#include "base/Path.h"
#include "base/String.h"
#include "base/TMethodEventJob.h"
#include "common/constants.h"
#include "mt/Lock.h"
#include "net/FingerprintDatabase.h"
#include "net/TCPSocket.h"
#include "net/TSocketMultiplexerMethodJob.h"
#include <net/SslLogger.h>

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

static const std::size_t MAX_INPUT_BUFFER_SIZE = 1024 * 1024;

static const float s_retryDelay = 0.01f;

enum
{
  kMsgSize = 128
};

struct Ssl
{
  SSL_CTX *m_context;
  SSL *m_ssl;
};

static int verifyIgnoreCertCallback(X509_STORE_CTX *, void *)
{
  return 1;
}

SecureSocket::SecureSocket(
    IEventQueue *events, SocketMultiplexer *socketMultiplexer, IArchNetwork::EAddressFamily family,
    SecurityLevel securityLevel
)
    : TCPSocket(events, socketMultiplexer, family),
      m_ssl(nullptr),
      m_secureReady(false),
      m_fatal(false),
      m_securityLevel{securityLevel}
{
}

SecureSocket::SecureSocket(
    IEventQueue *events, SocketMultiplexer *socketMultiplexer, ArchSocket socket, SecurityLevel securityLevel
)
    : TCPSocket(events, socketMultiplexer, socket),
      m_ssl(nullptr),
      m_secureReady(false),
      m_fatal(false),
      m_securityLevel{securityLevel}
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
  static uint8_t buffer[4096];
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

      if (m_inputBuffer.getSize() > MAX_INPUT_BUFFER_SIZE) {
        break;
      }

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
  std::lock_guard<std::mutex> ssl_lock{ssl_mutex_};

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
  std::lock_guard<std::mutex> ssl_lock{ssl_mutex_};

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
  std::lock_guard<std::mutex> ssl_lock{ssl_mutex_};

  m_ssl = new Ssl();
  m_ssl->m_context = NULL;
  m_ssl->m_ssl = NULL;

  initContext(server);
}

bool SecureSocket::loadCertificates(std::string &filename)
{
  std::lock_guard<std::mutex> ssl_lock{ssl_mutex_};

  if (filename.empty()) {
    SslLogger::logError("tls certificate is not specified");
    return false;
  } else {
    std::ifstream file(deskflow::filesystem::path(filename));
    bool exist = file.good();
    file.close();

    if (!exist) {
      std::string errorMsg("tls certificate doesn't exist: ");
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

  if (m_securityLevel == SecurityLevel::PeerAuth) {
    // We want to ask for peer certificate, but not verify it. If we don't ask for peer
    // certificate, e.g. client won't send it.
    SSL_CTX_set_verify(m_ssl->m_context, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, nullptr);
    SSL_CTX_set_cert_verify_callback(m_ssl->m_context, verifyIgnoreCertCallback, nullptr);
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
  std::lock_guard<std::mutex> ssl_lock{ssl_mutex_};

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
  std::lock_guard<std::mutex> ssl_lock{ssl_mutex_};

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
    if (m_securityLevel == SecurityLevel::PeerAuth) {
      std::string dbDir = deskflow::string::sprintf(
          "%s/%s/%s", ARCH->getProfileDirectory().c_str(), kSslDir, kFingerprintTrustedClientsFilename
      );
      if (!verifyCertFingerprint(dbDir)) {
        retry = 0;
        disconnect();
        return -1; // Fail
      }
    }
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

  std::string certDir =
      deskflow::string::sprintf("%s/%s/%s", ARCH->getProfileDirectory().c_str(), kSslDir, kCertificateFilename);

  if (!loadCertificates(certDir)) {
    LOG((CLOG_ERR "could not load client certificates"));
    disconnect();
    return -1;
  }

  std::lock_guard<std::mutex> ssl_lock{ssl_mutex_};

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
  std::string dbDir = deskflow::string::sprintf(
      "%s/%s/%s", ARCH->getProfileDirectory().c_str(), kSslDir, kFingerprintTrustedServersFilename
  );
  if (verifyCertFingerprint(dbDir)) {
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

bool SecureSocket::verifyCertFingerprint(const deskflow::fs::path &fingerprintDbPath)
{
  deskflow::FingerprintData sha1;
  deskflow::FingerprintData sha256;
  try {
    auto cert = SSL_get_peer_certificate(m_ssl->m_ssl);
    sha1 = deskflow::sslCertFingerprint(cert, deskflow::FingerprintType::SHA1);
    sha256 = deskflow::sslCertFingerprint(cert, deskflow::FingerprintType::SHA256);
  } catch (const std::exception &e) {
    LOG((CLOG_ERR "%s", e.what()));
    return false;
  }

  // Gui Must Parse these two lines, DO NOT CHANGE
  LOG(
      (CLOG_NOTE "peer fingerprint: (SHA1) %s (SHA256) %s", deskflow::formatSSLFingerprint(sha1.data).c_str(),
       deskflow::formatSSLFingerprint(sha256.data).c_str())
  );

  // check if this fingerprint exist
  std::ifstream file;

  deskflow::openUtf8Path(file, fingerprintDbPath);
  deskflow::FingerprintDatabase db;
  db.read(fingerprintDbPath);
  const bool emptyDB = db.fingerprints().empty();

  if (file.good() && emptyDB) {
    LOG((CLOG_ERR "failed to open trusted fingerprints file: %s", fingerprintDbPath.c_str()));
    return false;
  }

  if (!emptyDB) {
    LOG((CLOG_NOTE "read %d fingerprints from %s", db.fingerprints().size(), fingerprintDbPath.c_str()));
  }

  if (!db.isTrusted(sha256)) {
    LOG((CLOG_WARN "fingerprint does not match trusted fingerprint"));
    return false;
  }

  LOG((CLOG_NOTE "fingerprint matches trusted fingerprint"));
  return true;
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
