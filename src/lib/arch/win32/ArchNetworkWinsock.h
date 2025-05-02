/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <ws2tcpip.h>
// declare no functions in winsock2
#ifndef INCL_WINSOCK_API_PROTOTYPES
#define INCL_WINSOCK_API_PROTOTYPES 0
#endif
#define INCL_WINSOCK_API_TYPEDEFS 0

#include "arch/IArchMultithread.h"
#include "arch/IArchNetwork.h"

#include <WinSock2.h>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <list>

#pragma comment(lib, "ws2_32.lib")

#define ARCH_NETWORK ArchNetworkWinsock

class ArchSocketImpl
{
public:
  SOCKET m_socket;
  int m_refCount;
  WSAEVENT m_event;
  bool m_pollWrite;
};

class ArchNetAddressImpl
{
public:
  static ArchNetAddressImpl *alloc(size_t);

public:
  int m_len;
  struct sockaddr_storage m_addr;
};
#define ADDR_HDR_SIZE offsetof(ArchNetAddressImpl, m_addr)
#define TYPED_ADDR(type_, addr_) (reinterpret_cast<type_ *>(&addr_->m_addr))

//! Win32 implementation of IArchNetwork
class ArchNetworkWinsock : public IArchNetwork
{
public:
  ArchNetworkWinsock();
  ~ArchNetworkWinsock() override;

  void init() override;

  // IArchNetwork overrides
  ArchSocket newSocket(EAddressFamily, ESocketType) override;
  ArchSocket copySocket(ArchSocket s) override;
  void closeSocket(ArchSocket s) override;
  void closeSocketForRead(ArchSocket s) override;
  void closeSocketForWrite(ArchSocket s) override;
  void bindSocket(ArchSocket s, ArchNetAddress addr) override;
  void listenOnSocket(ArchSocket s) override;
  ArchSocket acceptSocket(ArchSocket s, ArchNetAddress *addr) override;
  bool connectSocket(ArchSocket s, ArchNetAddress name) override;
  int pollSocket(PollEntry[], int num, double timeout) override;
  void unblockPollSocket(ArchThread thread) override;
  size_t readSocket(ArchSocket s, void *buf, size_t len) override;
  size_t writeSocket(ArchSocket s, const void *buf, size_t len) override;
  void throwErrorOnSocket(ArchSocket) override;
  bool setNoDelayOnSocket(ArchSocket, bool noDelay) override;
  bool setReuseAddrOnSocket(ArchSocket, bool reuse) override;
  std::string getHostName() override;
  ArchNetAddress newAnyAddr(EAddressFamily) override;
  ArchNetAddress copyAddr(ArchNetAddress) override;
  std::vector<ArchNetAddress> nameToAddr(const std::string &) override;
  void closeAddr(ArchNetAddress) override;
  std::string addrToName(ArchNetAddress) override;
  std::string addrToString(ArchNetAddress) override;
  EAddressFamily getAddrFamily(ArchNetAddress) override;
  void setAddrPort(ArchNetAddress, int port) override;
  int getAddrPort(ArchNetAddress) override;
  bool isAnyAddr(ArchNetAddress) override;
  bool isEqualAddr(ArchNetAddress, ArchNetAddress) override;

private:
  void initModule(HMODULE);

  void setBlockingOnSocket(SOCKET, bool blocking);

  void throwError(int);
  void throwNameError(int);

private:
  using EventList = std::list<WSAEVENT>;

  ArchMutex m_mutex;
  EventList m_unblockEvents;
};
