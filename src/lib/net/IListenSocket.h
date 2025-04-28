/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "base/EventTypes.h"
#include "net/ISocket.h"

class IDataSocket;

//! Listen socket interface
/*!
This interface defines the methods common to all network sockets that
listen for incoming connections.
*/
class IListenSocket : public ISocket
{
public:
  //! @name manipulators
  //@{

  //! Accept connection
  /*!
  Accept a connection, returning a socket representing the full-duplex
  data stream.  Returns NULL if no socket is waiting to be accepted.
  This is only valid after a call to \c bind().
  */
  virtual IDataSocket *accept() = 0;

  //@}

  // ISocket overrides
  void bind(const NetworkAddress &) override = 0;
  void close() override = 0;
  void *getEventTarget() const override = 0;
};
