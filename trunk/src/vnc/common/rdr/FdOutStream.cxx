/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 * USA.
 */

#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <winsock2.h>
#define write(s,b,l) send(s,(const char*)b,l,0)
#define EWOULDBLOCK WSAEWOULDBLOCK
#undef errno
#define errno WSAGetLastError()
#undef EINTR
#define EINTR WSAEINTR
#else
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include <sys/time.h>
#endif

#include <rdr/FdOutStream.h>
#include <rdr/Exception.h>


using namespace rdr;

enum { DEFAULT_BUF_SIZE = 16384,
       MIN_BULK_SIZE = 1024 };

FdOutStream::FdOutStream(int fd_, int timeoutms_, int bufSize_)
  : fd(fd_), timeoutms(timeoutms_),
    bufSize(bufSize_ ? bufSize_ : DEFAULT_BUF_SIZE), offset(0)
{
  ptr = start = new U8[bufSize];
  end = start + bufSize;
}

FdOutStream::~FdOutStream()
{
  try {
    flush();
  } catch (Exception&) {
  }
  delete [] start;
}

void FdOutStream::setTimeout(int timeoutms_) {
  timeoutms = timeoutms_;
}

void FdOutStream::writeBytes(const void* data, int length)
{
  if (length < MIN_BULK_SIZE) {
    OutStream::writeBytes(data, length);
    return;
  }

  const U8* dataPtr = (const U8*)data;

  flush();

  while (length > 0) {
    int n = writeWithTimeout(dataPtr, length);
    length -= n;
    dataPtr += n;
    offset += n;
  }
}

int FdOutStream::length()
{
  return offset + ptr - start;
}

void FdOutStream::flush()
{
  U8* sentUpTo = start;
  while (sentUpTo < ptr) {
    int n = writeWithTimeout((const void*) sentUpTo, ptr - sentUpTo);
    sentUpTo += n;
    offset += n;
  }

  ptr = start;
}


int FdOutStream::overrun(int itemSize, int nItems)
{
  if (itemSize > bufSize)
    throw Exception("FdOutStream overrun: max itemSize exceeded");

  flush();

  if (itemSize * nItems > end - ptr)
    nItems = (end - ptr) / itemSize;

  return nItems;
}

//
// writeWithTimeout() writes up to the given length in bytes from the given
// buffer to the file descriptor.  If there is a timeout set and that timeout
// expires, it throws a TimedOut exception.  Otherwise it returns the number of
// bytes written.  It never attempts to write() unless select() indicates that
// the fd is writable - this means it can be used on an fd which has been set
// non-blocking.  It also has to cope with the annoying possibility of both
// select() and write() returning EINTR.
//

int FdOutStream::writeWithTimeout(const void* data, int length)
{
  int n;

  do {

    do {
      fd_set fds;
      struct timeval tv;
      struct timeval* tvp = &tv;

      if (timeoutms != -1) {
        tv.tv_sec = timeoutms / 1000;
        tv.tv_usec = (timeoutms % 1000) * 1000;
      } else {
        tvp = 0;
      }

      FD_ZERO(&fds);
      FD_SET(fd, &fds);
#ifdef _WIN32_WCE
      // NB: This fixes a broken Winsock2 select() behaviour.  select()
      // never returns for non-blocking sockets, unless they're already
      // ready to be written to...
      u_long zero = 0; ioctlsocket(fd, FIONBIO, &zero);
#endif
      n = select(fd+1, 0, &fds, 0, tvp);
#ifdef _WIN32_WCE
      u_long one = 0; ioctlsocket(fd, FIONBIO, &one);
#endif
    } while (n < 0 && errno == EINTR);

    if (n < 0) throw SystemException("select",errno);

    if (n == 0) throw TimedOut();

    do {
      n = ::write(fd, data, length);
    } while (n < 0 && (errno == EINTR));
      
    // NB: This outer loop simply fixes a broken Winsock2 EWOULDBLOCK
    // condition, found only under Win98 (first edition), with slow
    // network connections.  Should in fact never ever happen...
  } while (n < 0 && (errno == EWOULDBLOCK));

  if (n < 0) throw SystemException("write",errno);

  return n;
}
