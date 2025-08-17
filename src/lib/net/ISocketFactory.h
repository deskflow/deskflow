/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "arch/IArchNetwork.h"
#include "common/Common.h"
#include "net/SecurityLevel.h"

class IDataSocket;
class IListenSocket;

//! Socket factory
/*!
This interface defines the methods common to all factories used to
create sockets.
*/
class ISocketFactory
{
public:
  virtual ~ISocketFactory() = default;
  //! @name accessors
  //@{

  //! Create data socket
  virtual IDataSocket *create(
      IArchNetwork::AddressFamily family = IArchNetwork::AddressFamily::INet,
      SecurityLevel securityLevel = SecurityLevel::PlainText
  ) const = 0;

  //! Create listen socket
  virtual IListenSocket *createListen(
      IArchNetwork::AddressFamily family = IArchNetwork::AddressFamily::INet,
      SecurityLevel securityLevel = SecurityLevel::PlainText
  ) const = 0;

  //@}
};
