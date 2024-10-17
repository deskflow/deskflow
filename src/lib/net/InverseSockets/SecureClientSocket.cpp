/*
 * Deskflow -- mouse and keyboard sharing utility
 * Copyright (C) 2015-2022 Symless Ltd.
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

#include "SecureClientSocket.h"
#include "SslLogger.h"

#include <fstream>
#include <set>
#include <sstream>

#include <base/Log.h>
#include <base/Path.h>
#include <base/TMethodEventJob.h>

#include <arch/XArch.h>
#include <mt/Lock.h>

#include <openssl/err.h>
#include <openssl/ssl.h>

#include <net/NetworkAddress.h>
#include <net/TSocketMultiplexerMethodJob.h>

//
// SecureClientSocket
//
constexpr float s_retryDelay = 0.01f;

SecureClientSocket::SecureClientSocket(
    IEventQueue *events, SocketMultiplexer *socketMultiplexer, IArchNetwork::EAddressFamily family
)
    : InverseClientSocket(events, socketMultiplexer, family)
{
}

void SecureClientSocket::connect(const NetworkAddress &addr)
{
  m_events->adoptHandler(
      m_events->forIDataSocket().connected(), getEventTarget(),
      new TMethodEventJob<SecureClientSocket>(this, &SecureClientSocket::handleTCPConnected)
  );

  InverseClientSocket::connect(addr);
}

ISocketMultiplexerJob *SecureClientSocket::newJob()
{
  // after TCP connection is established, SecureClientSocket will pick up
  // connected event and do secureConnect
  if (m_connected && !m_secureReady) {
    return nullptr;
  }

  return InverseClientSocket::newJob(getSocket());
}

void SecureClientSocket::secureConnect()
{
  setJob(new TSocketMultiplexerMethodJob<SecureClientSocket>(
      this, &SecureClientSocket::serviceConnect, getSocket(), isReadable(), isWritable()
  ));
}

void SecureClientSocket::secureAccept()
{
  setJob(new TSocketMultiplexerMethodJob<SecureClientSocket>(
      this, &SecureClientSocket::serviceAccept, getSocket(), isReadable(), isWritable()
  ));
}

InverseClientSocket::EJobResult SecureClientSocket::doRead()
{
  UInt8 buffer[4096] = {0};
  int bytesRead = 0;
  int status = 0;

  if (isSecureReady()) {
    status = secureRead(buffer, sizeof(buffer), bytesRead);

    if (status < 0) {
      return InverseClientSocket::EJobResult::kBreak;
    } else if (status == 0) {
      return InverseClientSocket::EJobResult::kNew;
    }
  } else {
    return InverseClientSocket::EJobResult::kRetry;
  }

  if (bytesRead > 0) {
    bool wasEmpty = (m_inputBuffer.getSize() == 0);

    // slurp up as much as possible
    do {
      m_inputBuffer.write(buffer, bytesRead);

      status = secureRead(buffer, sizeof(buffer), bytesRead);
      if (status < 0) {
        return InverseClientSocket::EJobResult::kBreak;
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
    return InverseClientSocket::EJobResult::kNew;
  }

  return InverseClientSocket::EJobResult::kRetry;
}

InverseClientSocket::EJobResult SecureClientSocket::doWrite()
{
  static bool s_retry = false;
  static int s_retrySize = 0;
  static int s_staticBufferSize = 0;
  static void *s_staticBuffer = nullptr;

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
    return InverseClientSocket::EJobResult::kRetry;
  }

  if (isSecureReady()) {
    status = secureWrite(s_staticBuffer, bufferSize, bytesWrote);
    if (status > 0) {
      s_retry = false;
    } else if (status < 0) {
      return InverseClientSocket::EJobResult::kBreak;
    } else if (status == 0) {
      s_retry = true;
      s_retrySize = bufferSize;
      return InverseClientSocket::EJobResult::kNew;
    }
  } else {
    return InverseClientSocket::EJobResult::kRetry;
  }

  if (bytesWrote > 0) {
    discardWrittenData(bytesWrote);
    return InverseClientSocket::EJobResult::kNew;
  }

  return InverseClientSocket::EJobResult::kRetry;
}

int SecureClientSocket::secureRead(void *buffer, int size, int &read)
{
  LOG((CLOG_DEBUG2 "reading secure socket"));
  read = m_ssl.read(static_cast<char *>(buffer), size);

  static int retry = 0;

  // Check result will cleanup the connection in the case of a fatal
  checkResult(read, retry);

  if (retry) {
    return 0;
  }

  if (isFatal()) {
    return -1;
  }
  // According to SSL spec, the number of bytes read must not be negative and
  // not have an error code from SSL_get_error(). If this happens, it is
  // itself an error. Let the parent handle the case
  return read;
}

int SecureClientSocket::secureWrite(const void *buffer, int size, int &wrote)
{
  LOG((CLOG_DEBUG2 "writing secure socket: %p", this));
  wrote = m_ssl.write(static_cast<const char *>(buffer), size);

  static int retry = 0;

  // Check result will cleanup the connection in the case of a fatal
  checkResult(wrote, retry);

  if (retry) {
    return 0;
  }

  if (isFatal()) {
    return -1;
  }

  // According to SSL spec, r must not be negative and not have an error code
  // from SSL_get_error(). If this happens, it is itself an error. Let the
  // parent handle the case
  return wrote;
}

bool SecureClientSocket::isSecureReady() const
{
  return m_secureReady;
}

bool SecureClientSocket::loadCertificates(const std::string &filename)
{
  return m_ssl.loadCertificate(filename);
}

int SecureClientSocket::secureAccept(int socket)
{
  LOG((CLOG_DEBUG2 "accepting secure socket"));
  static int retry = 0;
  checkResult(m_ssl.accept(socket), retry);

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
    m_ssl.logSecureInfo();
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

int SecureClientSocket::secureConnect(int socket)
{
  LOG((CLOG_DEBUG2 "connecting secure socket"));
  static int retry = 0;
  checkResult(m_ssl.connect(socket), retry);

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
  sendEvent(m_events->forIDataSocket().secureConnected());

  auto fingerprint = m_ssl.getFingerprint();
  LOG((CLOG_NOTE "server fingerprint: %s", fingerprint.c_str()));

  if (m_ssl.isTrustedFingerprint(fingerprint)) {
    LOG((CLOG_INFO "connected to secure socket"));
    m_ssl.logSecureInfo();
    return 1;
  } else {
    LOG((CLOG_ERR "failed to verify server certificate fingerprint"));
    disconnect();
    return -1; // Fingerprint failed, error
  }
}

void SecureClientSocket::setFatal(int code)
{
  const std::set<int> nonFatal{
      SSL_ERROR_NONE, SSL_ERROR_WANT_READ, SSL_ERROR_WANT_WRITE, SSL_ERROR_WANT_CONNECT, SSL_ERROR_WANT_ACCEPT
  };
  m_fatal = nonFatal.find(code) == nonFatal.end();
}

int SecureClientSocket::getRetry(int errorCode, int retry) const
{
  const std::set<int> retryCodes{
      SSL_ERROR_WANT_READ, SSL_ERROR_WANT_WRITE, SSL_ERROR_WANT_CONNECT, SSL_ERROR_WANT_ACCEPT
  };

  if (errorCode == SSL_ERROR_NONE || isFatal()) {
    retry = 0;
  } else if (retryCodes.find(errorCode) != retryCodes.end()) {
    ++retry;
  }

  return retry;
}

void SecureClientSocket::checkResult(int status, int &retry)
{
  // ssl errors are a little quirky. the "want" errors are normal and
  // should result in a retry.
  int errorCode = m_ssl.getErrorCode(status);
  setFatal(errorCode);
  retry = getRetry(errorCode, retry);

  switch (errorCode) {
  case SSL_ERROR_WANT_WRITE:
    // Need to make sure the socket is known to be writable so the impending
    // select action actually triggers on a write. This isn't necessary for
    // m_readable because the socket logic is always readable
    m_writable = true;
    break;

  case SSL_ERROR_SYSCALL:
    if (ERR_peek_error() == 0) {
      if (status == 0) {
        LOG((CLOG_ERR "eof violates tls protocol"));
      } else if (status == -1) {
        // underlying socket I/O reproted an error
        try {
          ARCH->throwErrorOnSocket(getSocket());
        } catch (const XArchNetwork &e) {
          LOG((CLOG_ERR "%s", e.what()));
        }
      }
    }
    break;
  default:
    break;
  }

  SslLogger::logErrorByCode(errorCode, retry);

  if (isFatal()) {
    SslLogger::logError();
    disconnect();
  }
}

void SecureClientSocket::disconnect()
{
  sendEvent(getEvents()->forISocket().stopRetry());
  sendEvent(getEvents()->forISocket().disconnected());
  sendEvent(getEvents()->forIStream().inputShutdown());
}

ISocketMultiplexerJob *SecureClientSocket::serviceConnect(ISocketMultiplexerJob *, bool, bool, bool)
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
    return nullptr;
  }

  // If status > 0, success
  if (status > 0) {
    sendEvent(m_events->forIDataSocket().secureConnected());
    return newJob();
  }

  // Retry case
  return new TSocketMultiplexerMethodJob<SecureClientSocket>(
      this, &SecureClientSocket::serviceConnect, getSocket(), isReadable(), isWritable()
  );
}

ISocketMultiplexerJob *SecureClientSocket::serviceAccept(ISocketMultiplexerJob *, bool, bool, bool)
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
    return nullptr;
  }

  // If status > 0, success
  if (status > 0) {
    sendEvent(m_events->forClientListener().accepted());
    return newJob();
  }

  // Retry case
  return new TSocketMultiplexerMethodJob<SecureClientSocket>(
      this, &SecureClientSocket::serviceAccept, getSocket(), isReadable(), isWritable()
  );
}

void SecureClientSocket::handleTCPConnected(const Event &, void *)
{
  if (getSocket()) {
    secureConnect();
  } else {
    LOG((CLOG_DEBUG "disregarding stale connect event"));
  }
}
