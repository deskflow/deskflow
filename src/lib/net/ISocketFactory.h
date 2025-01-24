/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "arch/IArchNetwork.h"
#include "common/IInterface.h"

class IDataSocket;
class IListenSocket;

//! Socket factory
/*!
This interface defines the methods common to all factories used to
create sockets.
*/
class ISocketFactory : public IInterface
{
public:
  //! @name accessors
  //@{

  //! Create data socket
  virtual IDataSocket *create(bool secure, IArchNetwork::EAddressFamily family = IArchNetwork::kINET) const = 0;

  //! Create listen socket
  virtual IListenSocket *createListen(bool secure, IArchNetwork::EAddressFamily family = IArchNetwork::kINET) const = 0;

  //@}
};
