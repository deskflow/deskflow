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

#include <rdr/HexInStream.h>
#include <rdr/Exception.h>

#include <stdlib.h>
#include <ctype.h>

using namespace rdr;

const int DEFAULT_BUF_LEN = 16384;

static inline int min(int a, int b) {return a<b ? a : b;}

HexInStream::HexInStream(InStream& is, int bufSize_)
: bufSize(bufSize_ ? bufSize_ : DEFAULT_BUF_LEN), offset(0), in_stream(is)
{
  ptr = end = start = new U8[bufSize];
}

HexInStream::~HexInStream() {
  delete [] start;
}


bool HexInStream::readHexAndShift(char c, int* v) {
  c=tolower(c);
  if ((c >= '0') && (c <= '9'))
    *v = (*v << 4) + (c - '0');
  else if ((c >= 'a') && (c <= 'f'))
    *v = (*v << 4) + (c - 'a' + 10);
  else
    return false;
  return true;
}

bool HexInStream::hexStrToBin(const char* s, char** data, int* length) {
  int l=strlen(s);
  if ((l % 2) == 0) {
    delete [] *data;
    *data = 0; *length = 0;
    if (l == 0)
      return true;
    *data = new char[l/2];
    *length = l/2;
    for(int i=0;i<l;i+=2) {
      int byte = 0;
      if (!readHexAndShift(s[i], &byte) ||
        !readHexAndShift(s[i+1], &byte))
        goto decodeError;
      (*data)[i/2] = byte;
    }
    return true;
  }
decodeError:
  delete [] *data;
  *data = 0;
  *length = 0;
  return false;
}


int HexInStream::pos() {
  return offset + ptr - start;
}

int HexInStream::overrun(int itemSize, int nItems, bool wait) {
  if (itemSize > bufSize)
    throw Exception("HexInStream overrun: max itemSize exceeded");

  if (end - ptr != 0)
    memmove(start, ptr, end - ptr);

  end -= ptr - start;
  offset += ptr - start;
  ptr = start;

  while (end < ptr + itemSize) {
    int n = in_stream.check(2, 1, wait);
    if (n == 0) return 0;
    const U8* iptr = in_stream.getptr();
    const U8* eptr = in_stream.getend();
    int length = min((eptr - iptr)/2, start + bufSize - end);

    U8* optr = (U8*) end;
    for (int i=0; i<length; i++) {
      int v = 0;
      readHexAndShift(iptr[i*2], &v);
      readHexAndShift(iptr[i*2+1], &v);
      optr[i] = v;
    }

    in_stream.setptr(iptr + length*2);
    end += length;
  }

  if (itemSize * nItems > end - ptr)
    nItems = (end - ptr) / itemSize;

  return nItems;
}
