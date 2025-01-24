/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2012 Nick Bolton
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "arch/Arch.h"
#include "arch/IArchMultithread.h"
#include "base/ILogOutputter.h"
#include "common/ipc.h"

#include <deque>

class IpcServer;
class Event;
class IpcClientProxy;

//! Write log to GUI over IPC
/*!
This outputter writes output to the GUI via IPC.
*/
class IpcLogOutputter : public ILogOutputter
{
public:
  /*!
  If \p useThread is \c true, the buffer will be sent using a thread.
  If \p useThread is \c false, then the buffer needs to be sent manually
  using the \c sendBuffer() function.
  */
  IpcLogOutputter(IpcServer &ipcServer, IpcClientType clientType, bool useThread);
  IpcLogOutputter(IpcLogOutputter const &) = delete;
  virtual ~IpcLogOutputter();

  // ILogOutputter overrides
  virtual void open(const char *title);
  virtual void close();
  virtual void show(bool showIfEmpty);
  virtual bool write(ELevel level, const char *message);

  //! @name manipulators
  //@{

  //! Notify that the buffer should be sent.
  void notifyBuffer();

  //! Set the buffer size
  /*!
  Set the maximum size of the buffer to protect memory
  from runaway logging.
  */
  void bufferMaxSize(uint16_t bufferMaxSize);

  //! Set the rate limit
  /*!
  Set the maximum number of \p writeRate for every \p timeRate in seconds.
  */
  void bufferRateLimit(uint16_t writeLimit, double timeLimit);

  //! Send the buffer
  /*!
  Sends a chunk of the buffer to the IPC server, normally called
  when threaded mode is on.
  */
  void sendBuffer();

  //@}

  //! @name accessors
  //@{

  //! Get the buffer size
  /*!
  Returns the maximum size of the buffer.
  */
  uint16_t bufferMaxSize() const;

  //@}

private:
  void init();
  void bufferThread(void *);
  std::string getChunk(size_t count);
  void appendBuffer(const std::string &text);
  bool isRunning();

private:
  using Buffer = std::deque<std::string>;

  IpcServer &m_ipcServer;
  Buffer m_buffer;
  ArchMutex m_bufferMutex;
  bool m_sending;
  Thread *m_bufferThread;
  bool m_running;
  ArchCond m_notifyCond;
  ArchMutex m_notifyMutex;
  bool m_bufferWaiting;
  IArchMultithread::ThreadID m_bufferThreadId;
  uint16_t m_bufferMaxSize;
  uint16_t m_bufferRateWriteLimit;
  double m_bufferRateTimeLimit;
  uint16_t m_bufferWriteCount;
  double m_bufferRateStart;
  IpcClientType m_clientType;
  ArchMutex m_runningMutex;
};
