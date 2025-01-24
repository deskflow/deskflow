/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "arch/unix/ArchNetworkBSD.h"

#include "arch/Arch.h"
#include "arch/unix/ArchMultithreadPosix.h"
#include "arch/unix/XArchUnix.h"

#include <arpa/inet.h>
#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#if !defined(TCP_NODELAY)
#include <netinet/tcp.h>
#endif

#if !HAVE_INET_ATON
#include <stdio.h>
#endif

static const int s_family[] = {
    PF_UNSPEC,
    PF_INET,
    PF_INET6,
};

static const int s_type[] = {SOCK_DGRAM, SOCK_STREAM};

#if !HAVE_INET_ATON
// parse dotted quad addresses.  we don't bother with the weird BSD'ism
// of handling octal and hex and partial forms.
static in_addr_t inet_aton(const char *cp, struct in_addr *inp)
{
  unsigned int a, b, c, d;
  if (sscanf(cp, "%u.%u.%u.%u", &a, &b, &c, &d) != 4) {
    return 0;
  }
  if (a >= 256 || b >= 256 || c >= 256 || d >= 256) {
    return 0;
  }
  unsigned char *incp = (unsigned char *)inp;
  incp[0] = (unsigned char)(a & 0xffu);
  incp[1] = (unsigned char)(b & 0xffu);
  incp[2] = (unsigned char)(c & 0xffu);
  incp[3] = (unsigned char)(d & 0xffu);
  return inp->s_addr;
}
#endif

//
// ArchNetworkBSD::Deps
//

void ArchNetworkBSD::Deps::sleep(double seconds)
{
  //
  ARCH->sleep(seconds);
}

int ArchNetworkBSD::Deps::poll(struct pollfd *fds, nfds_t nfds, int timeout)
{
  return ::poll(fds, nfds, timeout);
}

std::shared_ptr<struct pollfd[]> ArchNetworkBSD::Deps::makePollFD(nfds_t n)
{
  // C++20 supports std::make_shared<struct pollfd[]>(n) but this is not
  // implemented on the compiler that comes with Ubuntu 22 and a few other
  // distros, so use the manual new and delete until we drop those distros.
  return std::shared_ptr<struct pollfd[]>(new struct pollfd[n], std::default_delete<struct pollfd[]>());
}

ssize_t ArchNetworkBSD::Deps::read(int fd, void *buf, size_t len)
{
  return ::read(fd, buf, len);
}

void ArchNetworkBSD::Deps::testCancelThread()
{
  ARCH->testCancelThread();
}

//
// ArchNetworkBSD
//

ArchNetworkBSD::~ArchNetworkBSD()
{
  if (m_mutex)
    ARCH->closeMutex(m_mutex);
}

void ArchNetworkBSD::init()
{
  // create mutex to make some calls thread safe
  m_mutex = ARCH->newMutex();
}

ArchSocket ArchNetworkBSD::newSocket(EAddressFamily family, ESocketType type)
{
  // create socket
  int fd = socket(s_family[family], s_type[type], 0);
  if (fd == -1) {
    throwError(errno);
  }
  try {
    setBlockingOnSocket(fd, false);
  } catch (...) {
    close(fd);
    throw;
  }

  // allocate socket object
  auto *newSocket = new ArchSocketImpl;
  newSocket->m_fd = fd;
  newSocket->m_refCount = 1;
  return newSocket;
}

ArchSocket ArchNetworkBSD::copySocket(ArchSocket s)
{
  assert(s != NULL);

  // ref the socket and return it
  ARCH->lockMutex(m_mutex);
  ++s->m_refCount;
  ARCH->unlockMutex(m_mutex);
  return s;
}

void ArchNetworkBSD::closeSocket(ArchSocket s)
{
  assert(s != NULL);

  // unref the socket and note if it should be released
  ARCH->lockMutex(m_mutex);
  const bool doClose = (--s->m_refCount == 0);
  ARCH->unlockMutex(m_mutex);

  // close the socket if necessary
  if (doClose) {
    if (close(s->m_fd) == -1) {
      // close failed.  restore the last ref and throw.
      int err = errno;
      ARCH->lockMutex(m_mutex);
      ++s->m_refCount;
      ARCH->unlockMutex(m_mutex);
      throwError(err);
    }
    delete s;
  }
}

void ArchNetworkBSD::closeSocketForRead(ArchSocket s)
{
  assert(s != NULL);

  if (shutdown(s->m_fd, 0) == -1) {
    if (errno != ENOTCONN) {
      throwError(errno);
    }
  }
}

void ArchNetworkBSD::closeSocketForWrite(ArchSocket s)
{
  assert(s != NULL);

  if (shutdown(s->m_fd, 1) == -1) {
    if (errno != ENOTCONN) {
      throwError(errno);
    }
  }
}

void ArchNetworkBSD::bindSocket(ArchSocket s, ArchNetAddress addr)
{
  assert(s != NULL);
  assert(addr != NULL);

  if (bind(s->m_fd, TYPED_ADDR(struct sockaddr, addr), addr->m_len) == -1) {
    throwError(errno);
  }
}

void ArchNetworkBSD::listenOnSocket(ArchSocket s)
{
  assert(s != NULL);

  // hardcoding backlog
  if (listen(s->m_fd, 3) == -1) {
    throwError(errno);
  }
}

ArchSocket ArchNetworkBSD::acceptSocket(ArchSocket s, ArchNetAddress *addr)
{
  assert(s != NULL);

  // if user passed NULL in addr then use scratch space
  ArchNetAddress dummy;
  if (addr == nullptr) {
    addr = &dummy;
  }

  // create new socket and address
  auto *newSocket = new ArchSocketImpl;
  *addr = new ArchNetAddressImpl;

  // accept on socket
  auto len = ((*addr)->m_len);
  int fd = accept(s->m_fd, TYPED_ADDR(struct sockaddr, (*addr)), &len);
  (*addr)->m_len = len;
  if (fd == -1) {
    int err = errno;
    delete newSocket;
    delete *addr;
    *addr = nullptr;
    if (err == EAGAIN) {
      return nullptr;
    }
    throwError(err);
  }

  try {
    setBlockingOnSocket(fd, false);
  } catch (...) {
    close(fd);
    delete newSocket;
    delete *addr;
    *addr = nullptr;
    throw;
  }

  // initialize socket
  newSocket->m_fd = fd;
  newSocket->m_refCount = 1;

  // discard address if not requested
  if (addr == &dummy) {
    ARCH->closeAddr(dummy);
  }

  return newSocket;
}

bool ArchNetworkBSD::connectSocket(ArchSocket s, ArchNetAddress addr)
{
  assert(s != NULL);
  assert(addr != NULL);

  if (connect(s->m_fd, TYPED_ADDR(struct sockaddr, addr), addr->m_len) == -1) {
    if (errno == EISCONN) {
      return true;
    }
    if (errno == EINPROGRESS) {
      return false;
    }
    throwError(errno);
  }
  return true;
}

int ArchNetworkBSD::pollSocket(PollEntry pe[], int num, double timeout)
{
  assert((pe != nullptr && num > 0) || num == 0);

  // return if nothing to do
  if (num == 0) {
    if (timeout > 0.0) {
      m_pDeps->sleep(timeout);
    }
    return 0;
  }

  // allocate space for translated query
  auto pfdPtr = m_pDeps->makePollFD(1 + num);
  auto *pfd = pfdPtr.get();

  // translate query
  for (int i = 0; i < num; ++i) {
    pfd[i].fd = (pe[i].m_socket == nullptr) ? -1 : pe[i].m_socket->m_fd;
    pfd[i].events = 0;
    if ((pe[i].m_events & kPOLLIN) != 0) {
      pfd[i].events |= POLLIN;
    }
    if ((pe[i].m_events & kPOLLOUT) != 0) {
      pfd[i].events |= POLLOUT;
    }
  }
  int n = num;

  // add the unblock pipe
  const int *unblockPipe = getUnblockPipe();
  if (unblockPipe != nullptr) {
    pfd[n].fd = unblockPipe[0]; // test
    pfd[n].events = POLLIN;
    ++n;
  }

  // prepare timeout
  int t = (timeout < 0.0) ? -1 : static_cast<int>(1000.0 * timeout);

  // do the poll
  n = m_pDeps->poll(pfd, n, t);

  // reset the unblock pipe
  if (n > 0 && unblockPipe != nullptr && (pfd[num].revents & POLLIN) != 0) {
    // the unblock event was signalled.  flush the pipe.
    char dummy[100];
    do {
      m_pDeps->read(unblockPipe[0], dummy, sizeof(dummy));
    } while (errno != EAGAIN);

    // don't count this unblock pipe in return value
    --n;
  }

  // handle results
  if (n == -1) {
    if (errno == EINTR) {
      // interrupted system call
      m_pDeps->testCancelThread();
      return 0;
    }
    throwError(errno);
    return -1; // unreachable
  }

  // translate back
  for (int i = 0; i < num; ++i) {
    pe[i].m_revents = 0;
    if ((pfd[i].revents & POLLIN) != 0) {
      pe[i].m_revents |= kPOLLIN;
    }
    if ((pfd[i].revents & POLLOUT) != 0) {
      pe[i].m_revents |= kPOLLOUT;
    }
    if ((pfd[i].revents & POLLERR) != 0) {
      pe[i].m_revents |= kPOLLERR;
    }
    if ((pfd[i].revents & POLLNVAL) != 0) {
      pe[i].m_revents |= kPOLLNVAL;
    }
  }

  return n;
}

void ArchNetworkBSD::unblockPollSocket(ArchThread thread)
{
  const int *unblockPipe = getUnblockPipeForThread(thread);
  if (unblockPipe != nullptr) {
    char dummy = 0;
    int ignore;

    ignore = write(unblockPipe[1], &dummy, 1);
  }
}

size_t ArchNetworkBSD::readSocket(ArchSocket s, void *buf, size_t len)
{
  assert(s != NULL);

  ssize_t n = read(s->m_fd, buf, len);
  if (n == -1) {
    if (errno == EINTR || errno == EAGAIN) {
      return 0;
    }
    throwError(errno);
  }
  return n;
}

size_t ArchNetworkBSD::writeSocket(ArchSocket s, const void *buf, size_t len)
{
  assert(s != NULL);

  ssize_t n = write(s->m_fd, buf, len);
  if (n == -1) {
    if (errno == EINTR || errno == EAGAIN) {
      return 0;
    }
    throwError(errno);
  }
  return n;
}

void ArchNetworkBSD::throwErrorOnSocket(ArchSocket s)
{
  assert(s != NULL);

  // get the error from the socket layer
  int err = 0;
  auto size = static_cast<socklen_t>(sizeof(err));
  if (getsockopt(s->m_fd, SOL_SOCKET, SO_ERROR, reinterpret_cast<optval_t *>(&err), &size) == -1) {
    err = errno;
  }

  // throw if there's an error
  if (err != 0) {
    throwError(err);
  }
}

void ArchNetworkBSD::setBlockingOnSocket(int fd, bool blocking)
{
  assert(fd != -1);

  int mode = fcntl(fd, F_GETFL, 0);
  if (mode == -1) {
    throwError(errno);
  }
  if (blocking) {
    mode &= ~O_NONBLOCK;
  } else {
    mode |= O_NONBLOCK;
  }
  if (fcntl(fd, F_SETFL, mode) == -1) {
    throwError(errno);
  }
}

bool ArchNetworkBSD::setNoDelayOnSocket(ArchSocket s, bool noDelay)
{
  assert(s != NULL);

  // get old state
  int oflag;
  auto size = static_cast<socklen_t>(sizeof(oflag));
  if (getsockopt(s->m_fd, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<optval_t *>(&oflag), &size) == -1) {
    throwError(errno);
  }

  int flag = noDelay ? 1 : 0;
  size = static_cast<socklen_t>(sizeof(flag));
  if (setsockopt(s->m_fd, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<optval_t *>(&flag), size) == -1) {
    throwError(errno);
  }

  return (oflag != 0);
}

bool ArchNetworkBSD::setReuseAddrOnSocket(ArchSocket s, bool reuse)
{
  assert(s != NULL);

  // get old state
  int oflag;
  auto size = static_cast<socklen_t>(sizeof(oflag));
  if (getsockopt(s->m_fd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<optval_t *>(&oflag), &size) == -1) {
    throwError(errno);
  }

  int flag = reuse ? 1 : 0;
  size = static_cast<socklen_t>(sizeof(flag));
  if (setsockopt(s->m_fd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<optval_t *>(&flag), size) == -1) {
    throwError(errno);
  }

  return (oflag != 0);
}

std::string ArchNetworkBSD::getHostName()
{
  char name[256];
  if (gethostname(name, sizeof(name)) == -1) {
    name[0] = '\0';
  } else {
    name[sizeof(name) - 1] = '\0';
  }
  return name;
}

ArchNetAddress ArchNetworkBSD::newAnyAddr(EAddressFamily family)
{
  // allocate address
  auto *addr = new ArchNetAddressImpl;

  // fill it in
  switch (family) {
  case kINET: {
    auto *ipAddr = TYPED_ADDR(struct sockaddr_in, addr);
    ipAddr->sin_family = AF_INET;
    ipAddr->sin_port = 0;
    ipAddr->sin_addr.s_addr = INADDR_ANY;
    addr->m_len = static_cast<socklen_t>(sizeof(struct sockaddr_in));
    break;
  }

  case kINET6: {
    struct sockaddr_in6 *ipAddr = TYPED_ADDR(struct sockaddr_in6, addr);
    ipAddr->sin6_family = AF_INET6;
    ipAddr->sin6_port = 0;
    memcpy(&ipAddr->sin6_addr, &in6addr_any, sizeof(in6addr_any));
    addr->m_len = (socklen_t)sizeof(struct sockaddr_in6);
    break;
  }
  default:
    delete addr;
    assert(0 && "invalid family");
  }

  return addr;
}

ArchNetAddress ArchNetworkBSD::copyAddr(ArchNetAddress addr)
{
  assert(addr != NULL);

  // allocate and copy address
  return new ArchNetAddressImpl(*addr);
}

std::vector<ArchNetAddress> ArchNetworkBSD::nameToAddr(const std::string &name)
{
  struct addrinfo hints;
  struct in6_addr serveraddr;

  memset(&hints, 0, sizeof(hints));
  hints.ai_flags = AI_NUMERICSERV;
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  if (inet_pton(AF_INET, name.c_str(), &serveraddr) == 1) {
    hints.ai_family = AF_INET;
    hints.ai_flags |= AI_NUMERICHOST;
  } else if (inet_pton(AF_INET6, name.c_str(), &serveraddr) == 1) {
    hints.ai_family = AF_INET6;
    hints.ai_flags |= AI_NUMERICHOST;
  }

  // done with static buffer
  ARCH->lockMutex(m_mutex);
  struct addrinfo *pResult = nullptr;
  int ret = getaddrinfo(name.c_str(), nullptr, &hints, &pResult);
  if (ret != 0) {
    ARCH->unlockMutex(m_mutex);
    throwNameError(ret);
  }

  // allocate address
  std::vector<ArchNetAddressImpl *> addresses;
  for (auto address = pResult; address != nullptr; address = address->ai_next) {
    addresses.push_back(new ArchNetAddressImpl);
    if (address->ai_family == AF_INET) {
      addresses.back()->m_len = (socklen_t)sizeof(struct sockaddr_in);
    } else {
      addresses.back()->m_len = (socklen_t)sizeof(struct sockaddr_in6);
    }

    memcpy(&addresses.back()->m_addr, address->ai_addr, addresses.back()->m_len);
  }

  freeaddrinfo(pResult);
  ARCH->unlockMutex(m_mutex);

  return addresses;
}

void ArchNetworkBSD::closeAddr(ArchNetAddress addr)
{
  assert(addr != NULL);

  delete addr;
}

std::string ArchNetworkBSD::addrToName(ArchNetAddress addr)
{
  assert(addr != NULL);

  // mutexed name lookup (ugh)
  ARCH->lockMutex(m_mutex);
  char host[1024];
  char service[20];
  int ret =
      getnameinfo(TYPED_ADDR(struct sockaddr, addr), addr->m_len, host, sizeof(host), service, sizeof(service), 0);
  if (ret != 0) {
    ARCH->unlockMutex(m_mutex);
    throwNameError(ret);
  }

  // save (primary) name
  std::string name = host;

  // done with static buffer
  ARCH->unlockMutex(m_mutex);

  return name;
}

std::string ArchNetworkBSD::addrToString(ArchNetAddress addr)
{
  assert(addr != NULL);

  switch (getAddrFamily(addr)) {
  case kINET: {
    auto *ipAddr = TYPED_ADDR(struct sockaddr_in, addr);
    ARCH->lockMutex(m_mutex);
    std::string s = inet_ntoa(ipAddr->sin_addr);
    ARCH->unlockMutex(m_mutex);
    return s;
  }

  case kINET6: {
    char strAddr[INET6_ADDRSTRLEN];
    struct sockaddr_in6 *ipAddr = TYPED_ADDR(struct sockaddr_in6, addr);
    ARCH->lockMutex(m_mutex);
    inet_ntop(AF_INET6, &ipAddr->sin6_addr, strAddr, INET6_ADDRSTRLEN);
    ARCH->unlockMutex(m_mutex);
    return strAddr;
  }

  default:
    assert(0 && "unknown address family");
    return "";
  }
}

IArchNetwork::EAddressFamily ArchNetworkBSD::getAddrFamily(ArchNetAddress addr)
{
  assert(addr != NULL);

  switch (addr->m_addr.ss_family) {
  case AF_INET:
    return kINET;
  case AF_INET6:
    return kINET6;

  default:
    return kUNKNOWN;
  }
}

void ArchNetworkBSD::setAddrPort(ArchNetAddress addr, int port)
{
  assert(addr != NULL);

  switch (getAddrFamily(addr)) {
  case kINET: {
    auto *ipAddr = TYPED_ADDR(struct sockaddr_in, addr);
    ipAddr->sin_port = htons(port);
    break;
  }

  case kINET6: {
    struct sockaddr_in6 *ipAddr = TYPED_ADDR(struct sockaddr_in6, addr);
    ipAddr->sin6_port = htons(port);
    break;
  }

  default:
    assert(0 && "unknown address family");
    break;
  }
}

int ArchNetworkBSD::getAddrPort(ArchNetAddress addr)
{
  assert(addr != NULL);

  switch (getAddrFamily(addr)) {
  case kINET: {
    auto *ipAddr = TYPED_ADDR(struct sockaddr_in, addr);
    return ntohs(ipAddr->sin_port);
  }

  case kINET6: {
    struct sockaddr_in6 *ipAddr = TYPED_ADDR(struct sockaddr_in6, addr);
    return ntohs(ipAddr->sin6_port);
  }

  default:
    assert(0 && "unknown address family");
    return 0;
  }
}

bool ArchNetworkBSD::isAnyAddr(ArchNetAddress addr)
{
  assert(addr != NULL);

  switch (getAddrFamily(addr)) {
  case kINET: {
    auto *ipAddr = TYPED_ADDR(struct sockaddr_in, addr);
    return (ipAddr->sin_addr.s_addr == INADDR_ANY && addr->m_len == static_cast<socklen_t>(sizeof(struct sockaddr_in)));
  }

  case kINET6: {
    struct sockaddr_in6 *ipAddr = TYPED_ADDR(struct sockaddr_in6, addr);
    return (
        addr->m_len == (socklen_t)sizeof(struct sockaddr_in6) &&
        memcmp(
            static_cast<const void *>(&ipAddr->sin6_addr), static_cast<const void *>(&in6addr_any), sizeof(in6_addr)
        ) == 0
    );
  }

  default:
    assert(0 && "unknown address family");
    return true;
  }
}

bool ArchNetworkBSD::isEqualAddr(ArchNetAddress a, ArchNetAddress b)
{
  return (a->m_len == b->m_len && memcmp(&a->m_addr, &b->m_addr, a->m_len) == 0);
}

const int *ArchNetworkBSD::getUnblockPipe()
{
  ArchMultithreadPosix *mt = ArchMultithreadPosix::getInstance();
  ArchThread thread = mt->newCurrentThread();
  const int *p = getUnblockPipeForThread(thread);
  ARCH->closeThread(thread);
  return p;
}

const int *ArchNetworkBSD::getUnblockPipeForThread(ArchThread thread)
{
  ArchMultithreadPosix *mt = ArchMultithreadPosix::getInstance();
  auto *unblockPipe = static_cast<int *>(mt->getNetworkDataForThread(thread));
  if (unblockPipe == nullptr) {
    unblockPipe = new int[2];
    if (pipe(unblockPipe) != -1) {
      try {
        setBlockingOnSocket(unblockPipe[0], false);
        mt->setNetworkDataForCurrentThread(unblockPipe);
      } catch (...) {
        delete[] unblockPipe;
        unblockPipe = nullptr;
      }
    } else {
      delete[] unblockPipe;
      unblockPipe = nullptr;
    }
  }
  return unblockPipe;
}

void ArchNetworkBSD::throwError(int err)
{
  switch (err) {
  case EINTR:
    ARCH->testCancelThread();
    throw XArchNetworkInterrupted(new XArchEvalUnix(err));

  case EACCES:
  case EPERM:
    throw XArchNetworkAccess(new XArchEvalUnix(err));

  case ENFILE:
  case EMFILE:
  case ENODEV:
  case ENOBUFS:
  case ENOMEM:
  case ENETDOWN:
#if defined(ENOSR)
  case ENOSR:
#endif
    throw XArchNetworkResource(new XArchEvalUnix(err));

  case EPROTOTYPE:
  case EPROTONOSUPPORT:
  case EAFNOSUPPORT:
  case EPFNOSUPPORT:
  case ESOCKTNOSUPPORT:
  case EINVAL:
  case ENOPROTOOPT:
  case EOPNOTSUPP:
  case ESHUTDOWN:
#if defined(ENOPKG)
  case ENOPKG:
#endif
    throw XArchNetworkSupport(new XArchEvalUnix(err));

  case EIO:
    throw XArchNetworkIO(new XArchEvalUnix(err));

  case EADDRNOTAVAIL:
    throw XArchNetworkNoAddress(new XArchEvalUnix(err));

  case EADDRINUSE:
    throw XArchNetworkAddressInUse(new XArchEvalUnix(err));

  case EHOSTUNREACH:
  case ENETUNREACH:
    throw XArchNetworkNoRoute(new XArchEvalUnix(err));

  case ENOTCONN:
    throw XArchNetworkNotConnected(new XArchEvalUnix(err));

  case EPIPE:
    throw XArchNetworkShutdown(new XArchEvalUnix(err));

  case ECONNABORTED:
  case ECONNRESET:
    throw XArchNetworkDisconnected(new XArchEvalUnix(err));

  case ECONNREFUSED:
    throw XArchNetworkConnectionRefused(new XArchEvalUnix(err));

  case EHOSTDOWN:
  case ETIMEDOUT:
    throw XArchNetworkTimedOut(new XArchEvalUnix(err));

  default:
    throw XArchNetwork(new XArchEvalUnix(err));
  }
}

void ArchNetworkBSD::throwNameError(int err)
{
  static const char *s_msg[] = {
      "The specified host is unknown", "The requested name is valid but does not have an IP address",
      "A non-recoverable name server error occurred", "A temporary error occurred on an authoritative name server",
      "An unknown name server error occurred"
  };

  switch (err) {
  case HOST_NOT_FOUND:
    throw XArchNetworkNameUnknown(s_msg[0]);

  case NO_DATA:
    throw XArchNetworkNameNoAddress(s_msg[1]);

  case NO_RECOVERY:
    throw XArchNetworkNameFailure(s_msg[2]);

  case TRY_AGAIN:
    throw XArchNetworkNameUnavailable(s_msg[3]);

  default:
    throw XArchNetworkName(s_msg[4]);
  }
}
