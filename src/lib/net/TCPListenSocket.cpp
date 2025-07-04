/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "net/TCPListenSocket.h"

#include "arch/Arch.h"
#include "arch/XArch.h"
#include "base/IEventQueue.h"
#include "base/Log.h"
#include "io/XIO.h"
#include "net/NetworkAddress.h"
#include "net/SocketMultiplexer.h"
#include "net/TCPSocket.h"
#include "net/TSocketMultiplexerMethodJob.h"
#include "net/XSocket.h"

//
// TCPListenSocket
//

TCPListenSocket::TCPListenSocket(
    IEventQueue *events, SocketMultiplexer *socketMultiplexer, IArchNetwork::AddressFamily family
)
    : m_events(events),
      m_socketMultiplexer(socketMultiplexer)
{
  try {
    m_socket = ARCH->newSocket(family, IArchNetwork::SocketType::Stream);
  } catch (XArchNetwork &e) {
    throw XSocketCreate(e.what());
  }
}

TCPListenSocket::~TCPListenSocket()
{
  try {
    if (m_socket != nullptr) {
      m_socketMultiplexer->removeSocket(this);
      ARCH->closeSocket(m_socket);
    }
  } catch (...) {
    // ignore
    LOG((CLOG_WARN "error while closing TCP socket"));
  }
}

void TCPListenSocket::bind(const NetworkAddress &addr)
{
  try {
    std::scoped_lock lock{m_mutex};
    ARCH->setReuseAddrOnSocket(m_socket, true);
    ARCH->bindSocket(m_socket, addr.getAddress());
    ARCH->listenOnSocket(m_socket);
    m_socketMultiplexer->addSocket(
        this, new TSocketMultiplexerMethodJob<TCPListenSocket>(
                  this, &TCPListenSocket::serviceListening, m_socket, true, false
              )
    );
  } catch (XArchNetworkAddressInUse &e) {
    throw XSocketAddressInUse(e.what());
  } catch (XArchNetwork &e) {
    throw XSocketBind(e.what());
  }
}

void TCPListenSocket::close()
{
  std::scoped_lock lock{m_mutex};
  if (m_socket == nullptr) {
    throw XIOClosed();
  }
  try {
    m_socketMultiplexer->removeSocket(this);
    ARCH->closeSocket(m_socket);
    m_socket = nullptr;
  } catch (XArchNetwork &e) {
    throw XSocketIOClose(e.what());
  }
}

void *TCPListenSocket::getEventTarget() const
{
  return const_cast<void *>(static_cast<const void *>(this));
}

std::unique_ptr<IDataSocket> TCPListenSocket::accept()
{
  std::unique_ptr<IDataSocket> socket;
  try {
    socket = std::make_unique<TCPSocket>(m_events, m_socketMultiplexer, ARCH->acceptSocket(m_socket, nullptr));
    setListeningJob();
    return socket;
  } catch (XArchNetwork &) {
    if (socket) {
      setListeningJob();
    }
    return nullptr;
  } catch (std::exception &ex) {
    if (socket) {
      setListeningJob();
    }
    throw ex;
  }
}

void TCPListenSocket::setListeningJob()
{
  m_socketMultiplexer->addSocket(
      this,
      new TSocketMultiplexerMethodJob<TCPListenSocket>(this, &TCPListenSocket::serviceListening, m_socket, true, false)
  );
}

ISocketMultiplexerJob *TCPListenSocket::serviceListening(ISocketMultiplexerJob *job, bool read, bool, bool error)
{
  if (error) {
    close();
    return nullptr;
  }
  if (read) {
    m_events->addEvent(Event(EventTypes::ListenSocketConnecting, this));
    // stop polling on this socket until the client accepts
    return nullptr;
  }
  return job;
}
