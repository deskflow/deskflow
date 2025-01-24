/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2022 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "AutoArchSocket.h"
#include "arch/IArchNetwork.h"
#include "io/StreamBuffer.h"
#include "mt/CondVar.h"
#include "mt/Mutex.h"
#include "net/IDataSocket.h"

class ISocketMultiplexerJob;
class IEventQueue;
class SocketMultiplexer;

class InverseClientSocket : public IDataSocket
{
public:
  InverseClientSocket(
      IEventQueue *events, SocketMultiplexer *socketMultiplexer,
      IArchNetwork::EAddressFamily family = IArchNetwork::kINET
  );
  InverseClientSocket(InverseClientSocket const &) = delete;
  InverseClientSocket(InverseClientSocket &&) = delete;
  ~InverseClientSocket() override;

  InverseClientSocket &operator=(InverseClientSocket const &) = delete;
  InverseClientSocket &operator=(InverseClientSocket &&) = delete;

  // ISocket overrides
  void bind(const NetworkAddress &) override;
  void close() override;
  void *getEventTarget() const override;

  // IStream overrides
  uint32_t read(void *buffer, uint32_t n) override;
  void write(const void *buffer, uint32_t n) override;
  void flush() override;
  void shutdownInput() override;
  void shutdownOutput() override;
  bool isReady() const override;
  bool isFatal() const override;
  uint32_t getSize() const override;

  // IDataSocket overrides
  void connect(const NetworkAddress &) override;

  virtual ISocketMultiplexerJob *newJob(ArchSocket socket);

protected:
  enum class EJobResult
  {
    kBreak = -1, //!< Break the Job chain
    kRetry,      //!< Retry the same job
    kNew         //!< Require a new job
  };

  ArchSocket getSocket()
  {
    return m_socket.getRawSocket();
  }
  IEventQueue *getEvents()
  {
    return m_events;
  }
  virtual EJobResult doRead();
  virtual EJobResult doWrite();

  void setJob(ISocketMultiplexerJob *);

  bool isReadable() const
  {
    return m_readable;
  }
  bool isWritable() const
  {
    return m_writable;
  }

  Mutex &getMutex()
  {
    return m_mutex;
  }

  void sendEvent(Event::Type);
  void discardWrittenData(int bytesWrote);

private:
  void sendConnectionFailedEvent(const char *);
  void onConnected();
  void onInputShutdown();
  void onOutputShutdown();
  void onDisconnected();

  ISocketMultiplexerJob *serviceConnecting(ISocketMultiplexerJob *, bool, bool, bool);
  ISocketMultiplexerJob *serviceConnected(ISocketMultiplexerJob *, bool, bool, bool);

protected:
  bool m_readable = false;
  bool m_writable = false;
  bool m_connected = false;
  IEventQueue *m_events;
  StreamBuffer m_inputBuffer;
  StreamBuffer m_outputBuffer;
  Mutex m_mutex;
  AutoArchSocket m_socket;
  AutoArchSocket m_listener;
  CondVar<bool> m_flushed;
  SocketMultiplexer *m_socketMultiplexer;
};
