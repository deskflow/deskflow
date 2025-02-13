/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2015 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "io/filesystem.h"
#include "net/SecurityLevel.h"
#include "net/TCPSocket.h"
#include "net/XSocket.h"
#include <mutex>

class IEventQueue;
class SocketMultiplexer;
class ISocketMultiplexerJob;

struct Ssl;

//! Secure socket
/*!
A secure socket using SSL.
*/
class SecureSocket : public TCPSocket
{
public:
  SecureSocket(
      IEventQueue *events, SocketMultiplexer *socketMultiplexer, IArchNetwork::EAddressFamily family,
      SecurityLevel securityLevel = SecurityLevel::Encrypted
  );
  SecureSocket(
      IEventQueue *events, SocketMultiplexer *socketMultiplexer, ArchSocket socket,
      SecurityLevel securityLevel = SecurityLevel::Encrypted
  );
  SecureSocket(SecureSocket const &) = delete;
  SecureSocket(SecureSocket &&) = delete;
  ~SecureSocket();

  SecureSocket &operator=(SecureSocket const &) = delete;
  SecureSocket &operator=(SecureSocket &&) = delete;

  // ISocket overrides
  void close() override;

  // IDataSocket overrides
  virtual void connect(const NetworkAddress &) override;

  ISocketMultiplexerJob *newJob() override;
  bool isFatal() const override
  {
    return m_fatal;
  }
  void isFatal(bool b)
  {
    m_fatal = b;
  }
  bool isSecureReady();
  void secureConnect();
  void secureAccept();
  int secureRead(void *buffer, int size, int &read);
  int secureWrite(const void *buffer, int size, int &wrote);
  EJobResult doRead() override;
  EJobResult doWrite() override;
  void initSsl(bool server);
  bool loadCertificates(std::string &CertFile);

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
  bool verifyCertFingerprint(const deskflow::fs::path &fingerprintDbPath);

  ISocketMultiplexerJob *serviceConnect(ISocketMultiplexerJob *, bool, bool, bool);

  ISocketMultiplexerJob *serviceAccept(ISocketMultiplexerJob *, bool, bool, bool);

  void handleTCPConnected(const Event &event, void *);

private:
  // all accesses to m_ssl must be protected by this mutex. The only function that is called
  // from outside SocketMultiplexer thread is close(), so we mostly care about things accessed
  // by it.
  std::mutex ssl_mutex_;

  Ssl *m_ssl;
  bool m_secureReady;
  bool m_fatal;
  SecurityLevel m_securityLevel = SecurityLevel::Encrypted;
};
