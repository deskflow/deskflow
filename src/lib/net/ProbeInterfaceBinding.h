// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#pragma once
#include "arch/Arch.h"
#include <cstdint>
#ifdef Q_OS_MACOS
#include <netinet/in.h>
#endif
namespace deskflow::net {
// Never accept index zero (the OS fallback interface). Read back the setting;
// an unsupported option or a different result is a terminal refusal.
template <class Set, class Get> bool requireProbeInterface(uint32_t index, Set set, Get get)
{
  if (!index || index > 0xffffffu)
    return false;
  uint32_t actual = 0;
  return set(index) && get(actual) && actual == index;
}
inline bool bindProbeInterface(ArchSocket socket, uint32_t index)
{
  if (!socket)
    return false;
#ifdef Q_OS_MACOS
  return requireProbeInterface(
      index, [socket](uint32_t n) { return setsockopt(socket->m_fd, IPPROTO_IP, IP_BOUND_IF, &n, sizeof(n)) == 0; },
      [socket](uint32_t &n) {
        socklen_t size = sizeof(n);
        return getsockopt(socket->m_fd, IPPROTO_IP, IP_BOUND_IF, &n, &size) == 0 && size == sizeof(n);
      }
  );
#elif defined(Q_OS_WIN)
  return requireProbeInterface(
      index,
      [socket](uint32_t n) {
        const DWORD value = htonl(n);
        return setsockopt(
                   socket->m_socket, IPPROTO_IP, IP_UNICAST_IF, reinterpret_cast<const char *>(&value), sizeof(value)
               ) == 0;
      },
      [socket](uint32_t &n) {
        DWORD value = 0;
        int size = sizeof(value);
        const bool ok =
            getsockopt(socket->m_socket, IPPROTO_IP, IP_UNICAST_IF, reinterpret_cast<char *>(&value), &size) == 0;
        n = ntohl(value);
        return ok && size == sizeof(value);
      }
  );
#else
  return false;
#endif
}
} // namespace deskflow::net
