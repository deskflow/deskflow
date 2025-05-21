/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2004 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "base/EventTypes.h"
#include "io/IStream.h"
#include "net/ISocket.h"

//! Data stream socket interface
/*!
This interface defines the methods common to all network sockets that
represent a full-duplex data stream.
*/
class IDataSocket : public ISocket, public deskflow::IStream
{
public:
  class ConnectionFailedInfo
  {
  public:
    explicit ConnectionFailedInfo(const char *what) : m_what(what)
    {
      // do nothing
    }
    std::string m_what;
  };

  explicit IDataSocket(const IEventQueue *events)
  {
    // do nothing
  }

  //! @name manipulators
  //@{

  //! Connect socket
  /*!
  Attempt to connect to a remote endpoint.  This returns immediately
  and sends a connected event when successful or a connection failed
  event when it fails.  The stream acts as if shutdown for input and
  output until the stream connects.
  */
  virtual void connect(const NetworkAddress &) = 0;

  //@}

  // ISocket overrides
  // close() and getEventTarget() aren't pure to work around a bug
  // in VC++6.  it claims the methods are unused locals and warns
  // that it's removing them.  it's presumably tickled by inheriting
  // methods with identical signatures from both superclasses.
  void bind(const NetworkAddress &) override = 0;
  void close() override;
  void *getEventTarget() const override;

  // IStream overrides
  uint32_t read(void *buffer, uint32_t n) override = 0;
  void write(const void *buffer, uint32_t n) override = 0;
  void flush() override = 0;
  void shutdownInput() override = 0;
  void shutdownOutput() override = 0;
  bool isReady() const override = 0;
  uint32_t getSize() const override = 0;
  virtual bool isFatal() const = 0;
};
