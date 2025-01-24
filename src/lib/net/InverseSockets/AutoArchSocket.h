/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2022 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once
#include "net/NetworkAddress.h"

class AutoArchSocket
{
public:
  explicit AutoArchSocket(IArchNetwork::EAddressFamily family);
  ~AutoArchSocket();

  AutoArchSocket(const AutoArchSocket &) = delete;
  AutoArchSocket &operator=(const AutoArchSocket &) = delete;

  void setNoDelayOnSocket(bool value = true);
  void setReuseAddrOnSocket(bool value = true);

  void listenOnSocket();
  ArchSocket acceptSocket();
  void bindSocket(const NetworkAddress &addr);
  bool connectSocket(const NetworkAddress &addr);
  void bindAndListen(const NetworkAddress &addr);

  void closeSocket();
  void closeSocketForRead();
  void closeSocketForWrite();

  size_t readSocket(uint8_t *buffer, size_t size);
  size_t writeSocket(const uint8_t *buffer, size_t size);
  void throwErrorOnSocket();

  bool isValid() const;
  ArchSocket getRawSocket() const;
  void operator=(ArchSocket socket);

private:
  ArchSocket m_socket = nullptr;
};
