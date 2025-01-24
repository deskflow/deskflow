/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "arch/IArchNetwork.h"
#include "io/StreamBuffer.h"
#include "mt/CondVar.h"
#include "mt/Mutex.h"
#include "net/IDataSocket.h"

class Mutex;
class Thread;
class ISocketMultiplexerJob;
class IEventQueue;
class SocketMultiplexer;

//! TCP data socket
/*!
A data socket using TCP.
*/
class TCPSocket : public IDataSocket
{
public:
  TCPSocket(
      IEventQueue *events, SocketMultiplexer *socketMultiplexer,
      IArchNetwork::EAddressFamily family = IArchNetwork::kINET
  );
  TCPSocket(IEventQueue *events, SocketMultiplexer *socketMultiplexer, ArchSocket socket);
  TCPSocket(TCPSocket const &) = delete;
  TCPSocket(TCPSocket &&) = delete;
  virtual ~TCPSocket();

  TCPSocket &operator=(TCPSocket const &) = delete;
  TCPSocket &operator=(TCPSocket &&) = delete;

  // ISocket overrides
  virtual void bind(const NetworkAddress &);
  virtual void close();
  virtual void *getEventTarget() const;

  // IStream overrides
  virtual uint32_t read(void *buffer, uint32_t n);
  virtual void write(const void *buffer, uint32_t n);
  virtual void flush();
  virtual void shutdownInput();
  virtual void shutdownOutput();
  virtual bool isReady() const;
  virtual bool isFatal() const;
  virtual uint32_t getSize() const;

  // IDataSocket overrides
  virtual void connect(const NetworkAddress &);

  virtual ISocketMultiplexerJob *newJob();

protected:
  enum EJobResult
  {
    kBreak = -1, //!< Break the Job chain
    kRetry,      //!< Retry the same job
    kNew         //!< Require a new job
  };

  ArchSocket getSocket()
  {
    return m_socket;
  }
  IEventQueue *getEvents()
  {
    return m_events;
  }
  virtual EJobResult doRead();
  virtual EJobResult doWrite();

  void setJob(ISocketMultiplexerJob *);

  bool isReadable()
  {
    return m_readable;
  }
  bool isWritable()
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
  void init();

  void sendConnectionFailedEvent(const char *);
  void onConnected();
  void onInputShutdown();
  void onOutputShutdown();
  void onDisconnected();

  ISocketMultiplexerJob *serviceConnecting(ISocketMultiplexerJob *, bool, bool, bool);
  ISocketMultiplexerJob *serviceConnected(ISocketMultiplexerJob *, bool, bool, bool);

protected:
  bool m_readable;
  bool m_writable;
  bool m_connected;
  IEventQueue *m_events;
  StreamBuffer m_inputBuffer;
  StreamBuffer m_outputBuffer;

private:
  Mutex m_mutex;
  ArchSocket m_socket;
  CondVar<bool> m_flushed;
  SocketMultiplexer *m_socketMultiplexer;
};
