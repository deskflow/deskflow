/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "arch/win32/XArchWindows.h"

#include <QString>

QString windowsErrorToQString(DWORD error)
{
  LPWSTR buffer = nullptr; // Using FORMAT_MESSAGE_ALLOCATE_BUFFER
  DWORD size = FormatMessage(
      FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, error,
      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPWSTR>(&buffer), 0, nullptr
  );

  if (size == 0) {
    return QString("Unknown Windows error: %1").arg(error);
  }

  QString message = QString::fromWCharArray(buffer, int(size));

  // Gotcha: Was allocated by FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER)
  LocalFree(buffer);

  return QString("[%1] %2").arg(error).arg(message.trimmed());
}

QString winsockErrorToQString(int error)
{
  // built-in windows function for looking up error message strings
  // may not look up network error messages correctly.  we'll have
  // to do it ourself.
  static const struct
  {
    int m_code;
    const char *m_msg;
  }

  s_netErrorCodes[] = {

      // 10004
      {WSAEINTR, "The (blocking) call was canceled via WSACancelBlockingCall"},

      // 10009
      {WSAEBADF, "Bad file handle"},

      // 10013
      {WSAEACCES, "The requested address is a broadcast address, but the appropriate flag was not set"},

      // 10014
      {WSAEFAULT, "WSAEFAULT"},

      // 10022
      {WSAEINVAL, "WSAEINVAL"},

      // 10024
      {WSAEMFILE, "No more file descriptors available"},

      // 10035
      {WSAEWOULDBLOCK,
       "Socket is marked as non-blocking and no connections are present or the receive operation would block"},

      // 10036
      {WSAEINPROGRESS, "A blocking Windows Sockets operation is in progress"},

      // 10037
      {WSAEALREADY, "The asynchronous routine being canceled has already completed"},

      // 10038
      {WSAENOTSOCK, "At least on descriptor is not a socket"},

      // 10039
      {WSAEDESTADDRREQ, "A destination address is required"},

      // 10040
      {WSAEMSGSIZE, "The datagram was too large to fit into the specified buffer and was truncated"},

      // 10041
      {WSAEPROTOTYPE, "The specified protocol is the wrong type for this socket"},

      // 10042
      {WSAENOPROTOOPT, "The option is unknown or unsupported"},

      // 10043
      {WSAEPROTONOSUPPORT, "The specified protocol is not supported"},

      // 10044
      {WSAESOCKTNOSUPPORT, "The specified socket type is not supported by this address family"},

      // 10045
      {WSAEOPNOTSUPP, "The referenced socket is not a type that supports that operation"},

      // 10046
      {WSAEPFNOSUPPORT, "BSD: Protocol family not supported"},

      // 10047
      {WSAEAFNOSUPPORT, "The specified address family is not supported"},

      // 10048
      {WSAEADDRINUSE, "The specified address is already in use"},

      // 10049
      {WSAEADDRNOTAVAIL, "The specified address is not available from the local machine"},

      // 10050
      {WSAENETDOWN, "The Windows Sockets implementation has detected that the network subsystem has failed"},

      // 10051
      {WSAENETUNREACH, "The network can't be reached from this host at this time"},

      // 10052
      {WSAENETRESET, "The connection must be reset because the Windows Sockets implementation dropped it"},

      // 10053
      {WSAECONNABORTED, "The virtual circuit was aborted due to timeout or other failure"},

      // 10054
      {WSAECONNRESET, "The virtual circuit was reset by the remote side"},

      // 10055
      {WSAENOBUFS, "No buffer space is available or a buffer deadlock has occurred. The socket cannot be created"},

      // 10056
      {WSAEISCONN, "The socket is already connected"},

      // 10057
      {WSAENOTCONN, "The socket is not connected"},

      // 10058
      {WSAESHUTDOWN, "The socket has been shutdown"},

      // 10059
      {WSAETOOMANYREFS, "BSD: Too many references"},

      // 10060
      {WSAETIMEDOUT, "Attempt to connect timed out without establishing a connection"},

      // 10061
      {WSAECONNREFUSED, "Connection was refused"},

      // 10062
      {WSAELOOP, "Undocumented WinSock error code used in BSD"},

      // 10063
      {WSAENAMETOOLONG, "Undocumented WinSock error code used in BSD"},

      // 10064
      {WSAEHOSTDOWN, "Undocumented WinSock error code used in BSD"},

      // 10065
      {WSAEHOSTUNREACH, "No route to host"},

      // 10066
      {WSAENOTEMPTY, "Undocumented WinSock error code"},

      // 10067
      {WSAEPROCLIM, "Undocumented WinSock error code"},

      // 10068
      {WSAEUSERS, "Undocumented WinSock error code"},

      // 10069
      {WSAEDQUOT, "Undocumented WinSock error code"},

      // 10070
      {WSAESTALE, "Undocumented WinSock error code"},

      // 10071
      {WSAEREMOTE, "Undocumented WinSock error code"},

      // 10091
      {WSASYSNOTREADY, "Underlying network subsytem is not ready for network communication"},

      // 10092
      {WSAVERNOTSUPPORTED, "The version of WinSock API support requested is not provided in this implementation"},

      // 10093
      {WSANOTINITIALISED, "WinSock subsystem not properly initialized"},

      // 10101
      {WSAEDISCON, "Virtual circuit has gracefully terminated connection"},

      // 11001
      {WSAHOST_NOT_FOUND, "The specified host is unknown"},

      // 11002
      {WSATRY_AGAIN, "A temporary error occurred on an authoritative name server"},

      // 11003
      {WSANO_RECOVERY, "A non-recoverable name server error occurred"},

      // 11004
      {WSANO_DATA, "The requested name is valid but does not have an IP address"},

      // end
      {0, nullptr}
  };

  for (unsigned int i = 0; s_netErrorCodes[i].m_code != 0; ++i) {
    if (s_netErrorCodes[i].m_code == error) {
      return QString("[%1] %2").arg(error).arg(s_netErrorCodes[i].m_msg);
    }
  }

  return QString("Unknown Winsock error: %1").arg(error);
}

std::string windowsErrorToString(DWORD error)
{
  return windowsErrorToQString(error).toStdString();
}

std::string winsockErrorToString(int error)
{
  return winsockErrorToQString(error).toStdString();
}
