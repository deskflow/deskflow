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

#include <rdr/RandomStream.h>
#include <rdr/Exception.h>
#include <time.h>
#include <stdlib.h>
#ifndef WIN32
#include <unistd.h>
#include <errno.h>
#else
#define getpid() GetCurrentProcessId()
#ifndef RFB_HAVE_WINCRYPT
#pragma message("  NOTE: Not building WinCrypt-based RandomStream")
#endif
#endif

using namespace rdr;

const int DEFAULT_BUF_LEN = 256;

unsigned int RandomStream::seed;

RandomStream::RandomStream()
  : offset(0)
{
  ptr = end = start = new U8[DEFAULT_BUF_LEN];

#ifdef RFB_HAVE_WINCRYPT
  provider = 0;
  if (!CryptAcquireContext(&provider, 0, 0, PROV_RSA_FULL, 0)) {
    if (GetLastError() == NTE_BAD_KEYSET) {
      if (!CryptAcquireContext(&provider, 0, 0, PROV_RSA_FULL, CRYPT_NEWKEYSET)) {
        fprintf(stderr, "RandomStream: unable to create keyset\n");
        provider = 0;
      }
    } else {
      fprintf(stderr, "RandomStream: unable to acquire context\n");
      provider = 0;
    }
  }
  if (!provider) {
#else
#ifndef WIN32
  fp = fopen("/dev/urandom", "r");
  if (!fp)
    fp = fopen("/dev/random", "r");
  if (!fp) {
#else
  {
#endif
#endif
    fprintf(stderr,"RandomStream: warning: no OS supplied random source - using rand()\n");
    seed += (unsigned int) time(0) + getpid() + getpid() * 987654 + rand();
    srand(seed);
  }
}

RandomStream::~RandomStream() {
  delete [] start;

#ifdef RFB_HAVE_WINCRYPT
  if (provider)
    CryptReleaseContext(provider, 0);
#endif
#ifndef WIN32
  if (fp) fclose(fp);
#endif
}

int RandomStream::pos() {
  return offset + ptr - start;
}

int RandomStream::overrun(int itemSize, int nItems, bool wait) {
  if (itemSize > DEFAULT_BUF_LEN)
    throw Exception("RandomStream overrun: max itemSize exceeded");

  if (end - ptr != 0)
    memmove(start, ptr, end - ptr);

  end -= ptr - start;
  offset += ptr - start;
  ptr = start;

  int length = start + DEFAULT_BUF_LEN - end;

#ifdef RFB_HAVE_WINCRYPT
  if (provider) {
    if (!CryptGenRandom(provider, length, (U8*)end))
      throw rdr::SystemException("unable to CryptGenRandom", GetLastError());
    end += length;
  } else {
#else
#ifndef WIN32
  if (fp) {
    int n = fread((U8*)end, length, 1, fp);
    if (n != 1)
      throw rdr::SystemException("reading /dev/urandom or /dev/random failed",
                                 errno);
    end += length;
  } else {
#else
  {
#endif
#endif
    for (int i=0; i<length; i++)
      *(U8*)end++ = (int) (256.0*rand()/(RAND_MAX+1.0));
  }

  if (itemSize * nItems > end - ptr)
    nItems = (end - ptr) / itemSize;

  return nItems;
}
