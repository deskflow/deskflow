/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "arch/IArchMultithread.h"
#include "arch/IArchNetwork.h"

#include <memory>
#include <mutex>
#include <poll.h>
#include <sys/socket.h>

#define ARCH_NETWORK ArchNetworkBSD
#define TYPED_ADDR(type_, addr_) (reinterpret_cast<type_ *>(&addr_->m_addr))

// old systems may use char* for [gs]etsockopt()'s optval argument.
// this should be void on modern systems but char is forwards
// compatible so we always use it.
using optval_t = char;

class ArchSocketImpl
{
public:
  int m_fd;
  int m_refCount;
};

class ArchNetAddressImpl
{
public:
  ArchNetAddressImpl() : m_len(sizeof(m_addr))
  {
    // do nothing
  }

public:
  struct sockaddr_storage m_addr;
  socklen_t m_len;
};

//! Berkeley (BSD) sockets implementation of IArchNetwork
class ArchNetworkBSD : public IArchNetwork
{

public:
  struct Deps
  {
    virtual ~Deps() = default;
    virtual void sleep(double);
    virtual int poll(struct pollfd *, nfds_t, int);
    virtual std::shared_ptr<struct pollfd[]> makePollFD(nfds_t);
    virtual ssize_t read(int, void *, size_t);
    virtual void testCancelThread();
  };

  explicit ArchNetworkBSD(std::shared_ptr<Deps> deps = std::make_shared<Deps>()) : m_pDeps(deps)
  {
  }
  ArchNetworkBSD(ArchNetworkBSD const &) = delete;
  ArchNetworkBSD(ArchNetworkBSD &&) = delete;
  ~ArchNetworkBSD() override = default;

  ArchNetworkBSD &operator=(ArchNetworkBSD const &) = delete;
  ArchNetworkBSD &operator=(ArchNetworkBSD &&) = delete;

  void init() override;

  // IArchNetwork overrides
  ArchSocket newSocket(AddressFamily, SocketType) override;
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
  ArchNetAddress newAnyAddr(AddressFamily) override;
  ArchNetAddress copyAddr(ArchNetAddress) override;
  std::vector<ArchNetAddress> nameToAddr(const std::string &) override;
  void closeAddr(ArchNetAddress) override;
  std::string addrToName(ArchNetAddress) override;
  std::string addrToString(ArchNetAddress) override;
  AddressFamily getAddrFamily(ArchNetAddress) override;
  void setAddrPort(ArchNetAddress, int port) override;
  int getAddrPort(ArchNetAddress) override;
  bool isAnyAddr(ArchNetAddress) override;
  bool isEqualAddr(ArchNetAddress, ArchNetAddress) override;

private:
  const int *getUnblockPipe();
  const int *getUnblockPipeForThread(ArchThread);
  void setBlockingOnSocket(int fd, bool blocking) const;
  [[noreturn]] void throwError(int) const override;
  [[noreturn]] void throwNameError(int) const override;

  std::shared_ptr<Deps> m_pDeps;
  std::mutex m_mutex;
};
