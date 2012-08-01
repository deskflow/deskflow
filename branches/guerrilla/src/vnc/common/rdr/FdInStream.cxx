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
#ifndef _WIN32_WCE
#include <sys/timeb.h>
#endif
#define read(s,b,l) recv(s,(char*)b,l,0)
#define close closesocket
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

// XXX should use autoconf HAVE_SYS_SELECT_H
#ifdef _AIX
#include <sys/select.h>
#endif

#include <rdr/FdInStream.h>
#include <rdr/Exception.h>

#pragma warning(disable: 4244)

using namespace rdr;

enum { DEFAULT_BUF_SIZE = 8192,
       MIN_BULK_SIZE = 1024 };

FdInStream::FdInStream(int fd_, int timeoutms_, int bufSize_,
                       bool closeWhenDone_)
  : fd(fd_), closeWhenDone(closeWhenDone_),
    timeoutms(timeoutms_), blockCallback(0),
    timing(false), timeWaitedIn100us(5), timedKbits(0),
    bufSize(bufSize_ ? bufSize_ : DEFAULT_BUF_SIZE), offset(0)
{
  ptr = end = start = new U8[bufSize];
}

FdInStream::FdInStream(int fd_, FdInStreamBlockCallback* blockCallback_,
                       int bufSize_)
  : fd(fd_), timeoutms(0), blockCallback(blockCallback_),
    timing(false), timeWaitedIn100us(5), timedKbits(0),
    bufSize(bufSize_ ? bufSize_ : DEFAULT_BUF_SIZE), offset(0)
{
  ptr = end = start = new U8[bufSize];
}

FdInStream::~FdInStream()
{
  delete [] start;
  if (closeWhenDone) close(fd);
}


void FdInStream::setTimeout(int timeoutms_) {
  timeoutms = timeoutms_;
}

void FdInStream::setBlockCallback(FdInStreamBlockCallback* blockCallback_)
{
  blockCallback = blockCallback_;
  timeoutms = 0;
}

int FdInStream::pos()
{
  return offset + ptr - start;
}

void FdInStream::readBytes(void* data, int length)
{
  if (length < MIN_BULK_SIZE) {
    InStream::readBytes(data, length);
    return;
  }

  U8* dataPtr = (U8*)data;

  int n = end - ptr;
  if (n > length) n = length;

  memcpy(dataPtr, ptr, n);
  dataPtr += n;
  length -= n;
  ptr += n;

  while (length > 0) {
    n = readWithTimeoutOrCallback(dataPtr, length);
    dataPtr += n;
    length -= n;
    offset += n;
  }
}


int FdInStream::overrun(int itemSize, int nItems, bool wait)
{
  if (itemSize > bufSize)
    throw Exception("FdInStream overrun: max itemSize exceeded");

  if (end - ptr != 0)
    memmove(start, ptr, end - ptr);

  offset += ptr - start;
  end -= ptr - start;
  ptr = start;

  while (end < start + itemSize) {
    int n = readWithTimeoutOrCallback((U8*)end, start + bufSize - end, wait);
    if (n == 0) return 0;
    end += n;
  }

  if (itemSize * nItems > end - ptr)
    nItems = (end - ptr) / itemSize;

  return nItems;
}

#ifdef _WIN32
static void gettimeofday(struct timeval* tv, void*)
{
  LARGE_INTEGER counts, countsPerSec;
  static double usecPerCount = 0.0;

  if (QueryPerformanceCounter(&counts)) {
    if (usecPerCount == 0.0) {
      QueryPerformanceFrequency(&countsPerSec);
      usecPerCount = 1000000.0 / countsPerSec.QuadPart;
    }

    LONGLONG usecs = (LONGLONG)(counts.QuadPart * usecPerCount);
    tv->tv_usec = (long)(usecs % 1000000);
    tv->tv_sec = (long)(usecs / 1000000);

  } else {
#ifndef _WIN32_WCE
    struct timeb tb;
    ftime(&tb);
    tv->tv_sec = tb.time;
    tv->tv_usec = tb.millitm * 1000;
#else
    throw SystemException("QueryPerformanceCounter", GetLastError());
#endif
  }
}
#endif

//
// readWithTimeoutOrCallback() reads up to the given length in bytes from the
// file descriptor into a buffer.  If the wait argument is false, then zero is
// returned if no bytes can be read without blocking.  Otherwise if a
// blockCallback is set, it will be called (repeatedly) instead of blocking.
// If alternatively there is a timeout set and that timeout expires, it throws
// a TimedOut exception.  Otherwise it returns the number of bytes read.  It
// never attempts to read() unless select() indicates that the fd is readable -
// this means it can be used on an fd which has been set non-blocking.  It also
// has to cope with the annoying possibility of both select() and read()
// returning EINTR.
//

int FdInStream::readWithTimeoutOrCallback(void* buf, int len, bool wait)
{
  struct timeval before, after;
  if (timing)
    gettimeofday(&before, 0);

  int n;
  while (true) {
    do {
      fd_set fds;
      struct timeval tv;
      struct timeval* tvp = &tv;

      if (!wait) {
        tv.tv_sec = tv.tv_usec = 0;
      } else if (timeoutms != -1) {
        tv.tv_sec = timeoutms / 1000;
        tv.tv_usec = (timeoutms % 1000) * 1000;
      } else {
        tvp = 0;
      }

      FD_ZERO(&fds);
      FD_SET(fd, &fds);
      n = select(fd+1, &fds, 0, 0, tvp);
    } while (n < 0 && errno == EINTR);

    if (n > 0) break;
    if (n < 0) throw SystemException("select",errno);
    if (!wait) return 0;
    if (!blockCallback) throw TimedOut();

    blockCallback->blockCallback();
  }

  do {
    n = ::read(fd, buf, len);
  } while (n < 0 && errno == EINTR);

  if (n < 0) throw SystemException("read",errno);
  if (n == 0) throw EndOfStream();

  if (timing) {
    gettimeofday(&after, 0);
//      fprintf(stderr,"%d.%06d\n",(after.tv_sec - before.tv_sec),
//              (after.tv_usec - before.tv_usec));
    int newTimeWaited = ((after.tv_sec - before.tv_sec) * 10000 +
                         (after.tv_usec - before.tv_usec) / 100);
    int newKbits = n * 8 / 1000;

//      if (newTimeWaited == 0) {
//        fprintf(stderr,"new kbps infinite t %d k %d\n",
//                newTimeWaited, newKbits);
//      } else {
//        fprintf(stderr,"new kbps %d t %d k %d\n",
//                newKbits * 10000 / newTimeWaited, newTimeWaited, newKbits);
//      }

    // limit rate to between 10kbit/s and 40Mbit/s

    if (newTimeWaited > newKbits*1000) newTimeWaited = newKbits*1000;
    if (newTimeWaited < newKbits/4)    newTimeWaited = newKbits/4;

    timeWaitedIn100us += newTimeWaited;
    timedKbits += newKbits;
  }

  return n;
}

void FdInStream::startTiming()
{
  timing = true;

  // Carry over up to 1s worth of previous rate for smoothing.

  if (timeWaitedIn100us > 10000) {
    timedKbits = timedKbits * 10000 / timeWaitedIn100us;
    timeWaitedIn100us = 10000;
  }
}

void FdInStream::stopTiming()
{
  timing = false; 
  if (timeWaitedIn100us < timedKbits/2)
    timeWaitedIn100us = timedKbits/2; // upper limit 20Mbit/s
}

unsigned int FdInStream::kbitsPerSecond()
{
  // The following calculation will overflow 32-bit arithmetic if we have
  // received more than about 50Mbytes (400Mbits) since we started timing, so
  // it should be OK for a single RFB update.

  return timedKbits * 10000 / timeWaitedIn100us;
}
