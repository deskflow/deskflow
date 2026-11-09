/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2015 - 2016 Synergy App Ltd
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "common/PeerAuthentication.h"
#include "net/SecurityLevel.h"
#include "net/TCPSocket.h"

#include <memory>
#include <mutex>

class Event;
class IEventQueue;
class SocketMultiplexer;
class ISocketMultiplexerJob;
class QString;

struct Ssl;
struct ssl_ctx_st;

//! Secure socket
/*!
A secure socket using SSL.
*/
class SecureSocket : public TCPSocket
{
public:
  SecureSocket(
      IEventQueue *events, SocketMultiplexer *socketMultiplexer, IArchNetwork::AddressFamily family,
      SecurityLevel securityLevel = SecurityLevel::Encrypted
  );
  SecureSocket(
      IEventQueue *events, SocketMultiplexer *socketMultiplexer, ArchSocket socket,
      SecurityLevel securityLevel = SecurityLevel::Encrypted
  );
  SecureSocket(SecureSocket const &) = delete;
  SecureSocket(SecureSocket &&) = delete;
  ~SecureSocket() override;

  SecureSocket &operator=(SecureSocket const &) = delete;
  SecureSocket &operator=(SecureSocket &&) = delete;

  // ISocket overrides
  void close() override;

  // IDataSocket overrides
  void connect(const NetworkAddress &) override;

  ISocketMultiplexerJob *newJob() override;
  bool isFatal() const override
  {
    return m_fatal;
  }
  void isFatal(bool b)
  {
    m_fatal = b;
  }
  bool isSecureReady() const;
  QString peerIdentityReference() const;
  void secureConnect();
  void secureAccept();
  int secureRead(void *buffer, int size, int &read);
  int secureWrite(const void *buffer, int size, int &wrote);
  JobResult doRead() override;
  JobResult doWrite() override;
  void initSsl(bool server);
  bool loadCertificate(const QString &filename);
  static bool loadPreparedIdentity(ssl_ctx_st *context, const QByteArray &material);
  // Offline-testable decision also used by the live handshake path. It does
  // not construct a socket, read credentials or add a trusted fingerprint.
  static deskflow::auth::Report authenticatePeerDigest(
      deskflow::auth::Report attempt, const QString &digest, bool tls, bool peerChecks, bool databaseTrusted
  );

private:
  // SSL
  void initContext(bool server);
  void createSSL();
  void freeSSL();
  int secureAccept(int s);
  int secureConnect(int s);
  bool showCertificate() const;
  void checkResult(int n, int &retry);
  void disconnect();
  bool verifyCertFingerprint(const QString &FingerprintDatabasePath);
  void publishAuthentication(deskflow::auth::Outcome outcome, const QString &digest = {});

  ISocketMultiplexerJob *serviceConnect(ISocketMultiplexerJob *const socket, bool, bool, bool);

  ISocketMultiplexerJob *serviceAccept(ISocketMultiplexerJob *const socket, bool, bool, bool);

  void handleTCPConnected(const Event &event);

private:
  // all accesses to m_ssl must be protected by this mutex. The only function that is called
  // from outside SocketMultiplexer thread is close(), so we mostly care about things accessed
  // by it.
  std::mutex ssl_mutex_;
  mutable std::mutex m_identityMutex;
  QString m_verifiedPeerIdentity;

  std::unique_ptr<Ssl> m_ssl;
  bool m_secureReady = false;
  bool m_fatal = false;
  SecurityLevel m_securityLevel = SecurityLevel::Encrypted;
  deskflow::auth::Report m_authAttempt;
  std::atomic_bool m_authCancelled{false};
  bool m_authTerminal = false;

  bool m_writeRetry = false;
  int m_writeRetrySize = 0;
  int m_writeStaticBufferSize = 0;
  void *m_writeStaticBuffer = nullptr;
};
