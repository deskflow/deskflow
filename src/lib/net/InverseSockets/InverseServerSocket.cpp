/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2022 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "InverseServerSocket.h"

#include "arch/Arch.h"
#include "arch/XArch.h"
#include "base/IEventQueue.h"
#include "base/Log.h"
#include "io/XIO.h"
#include "mt/Lock.h"
#include "mt/Mutex.h"
#include "net/NetworkAddress.h"
#include "net/SocketMultiplexer.h"
#include "net/TCPSocket.h"
#include "net/TSocketMultiplexerMethodJob.h"
#include "net/XSocket.h"

//
// InverseServerSocket
//

InverseServerSocket::InverseServerSocket(
    IEventQueue *events, SocketMultiplexer *socketMultiplexer, IArchNetwork::EAddressFamily family
)
    : m_socket(family),
      m_events(events),
      m_socketMultiplexer(socketMultiplexer)
{
}

InverseServerSocket::~InverseServerSocket()
{
  m_socketMultiplexer->removeSocket(this);
}

void InverseServerSocket::bind(const NetworkAddress &addr)
{
  Lock lock(&m_mutex);
  m_address = addr;
  m_socket.connectSocket(m_address);
  setListeningJob(true);
}

void InverseServerSocket::close()
{
  Lock lock(&m_mutex);
  m_socketMultiplexer->removeSocket(this);
  m_socket.closeSocket();
}

void *InverseServerSocket::getEventTarget() const
{
  return const_cast<void *>(static_cast<const void *>(this));
}

IDataSocket *InverseServerSocket::accept()
{
  IDataSocket *socket = nullptr;
  try {
    socket = new TCPSocket(m_events, m_socketMultiplexer, m_socket.getRawSocket());
    if (socket != nullptr) {
      setListeningJob();
    }
    return socket;
  } catch (const XArchNetwork &) {
    if (socket != nullptr) {
      delete socket;
      setListeningJob();
    }
    return nullptr;
  } catch (const std::exception &) {
    if (socket != nullptr) {
      delete socket;
      setListeningJob();
    }
    throw;
  }
}

void InverseServerSocket::setListeningJob(bool read)
{
  m_socketMultiplexer->addSocket(
      this, new TSocketMultiplexerMethodJob<InverseServerSocket>(
                this, &InverseServerSocket::serviceListening, m_socket.getRawSocket(), true, read
            )
  );
}

ISocketMultiplexerJob *InverseServerSocket::serviceListening(ISocketMultiplexerJob *job, bool, bool write, bool error)
{
  if (error) {
    m_socket.connectSocket(m_address);
    return job;
  }
  if (write) {
    m_events->addEvent(Event(m_events->forIListenSocket().connecting(), this));
    // stop polling on this socket until the client accepts
    return nullptr;
  }
  return job;
}
