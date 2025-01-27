/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2015 - 2022 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once
#include "InverseClientSocket.h"
#include "SslApi.h"

//! Secure socket
/*!
A secure socket using SSL.
*/
class SecureClientSocket : public InverseClientSocket
{
public:
  SecureClientSocket(IEventQueue *events, SocketMultiplexer *socketMultiplexer, IArchNetwork::EAddressFamily family);
  SecureClientSocket(SecureClientSocket const &) = delete;
  SecureClientSocket(SecureClientSocket &&) = delete;

  SecureClientSocket &operator=(SecureClientSocket const &) = delete;
  SecureClientSocket &operator=(SecureClientSocket &&) = delete;

  // IDataSocket overrides
  void connect(const NetworkAddress &) override;

  ISocketMultiplexerJob *newJob();
  bool isFatal() const override
  {
    return m_fatal;
  }
  void setFatal(int code);
  int getRetry(int errorCode, int retry) const;
  bool isSecureReady() const;
  void secureConnect();
  void secureAccept();
  int secureRead(void *buffer, int size, int &read);
  int secureWrite(const void *buffer, int size, int &wrote);
  EJobResult doRead() override;
  EJobResult doWrite() override;
  bool loadCertificates(const std::string &CertFile);

private:
  // SSL
  void initContext(bool server);
  int secureAccept(int s);
  int secureConnect(int s);
  void checkResult(int n, int &retry);
  void disconnect();

  ISocketMultiplexerJob *serviceConnect(ISocketMultiplexerJob *, bool, bool, bool);

  ISocketMultiplexerJob *serviceAccept(ISocketMultiplexerJob *, bool, bool, bool);

  void handleTCPConnected(const Event &, void *);

  deskflow::ssl::SslApi m_ssl{false};
  bool m_secureReady = false;
  bool m_fatal = false;
};
