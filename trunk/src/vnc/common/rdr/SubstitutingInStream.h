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

#ifndef __RDR_SUBSTITUTINGINSTREAM_H__
#define __RDR_SUBSTITUTINGINSTREAM_H__

#include <rdr/InStream.h>
#include <rdr/Exception.h>

namespace rdr {

  class Substitutor {
  public:
    virtual char* substitute(const char* varName) = 0;
  };

  class SubstitutingInStream : public InStream {
  public:
    SubstitutingInStream(InStream* underlying_, Substitutor* s,
                         int maxVarNameLen_)
      : underlying(underlying_), dollar(0), substitutor(s), subst(0),
        maxVarNameLen(maxVarNameLen_)
    {
      ptr = end = underlying->getptr();
      varName = new char[maxVarNameLen+1];
    }
    ~SubstitutingInStream() {
      delete underlying;
      delete [] varName;
      delete [] subst;
    }

    int pos() { return underlying->pos(); }

    virtual int overrun(int itemSize, int nItems, bool wait=true) {
      if (itemSize != 1)
        throw new rdr::Exception("SubstitutingInStream: itemSize must be 1");

      if (subst) {
        delete [] subst;
        subst = 0;
      } else {
        underlying->setptr(ptr);
      }

      underlying->check(1);
      ptr = underlying->getptr();
      end = underlying->getend();
      dollar = (const U8*)memchr(ptr, '$', end-ptr);
      if (dollar) {
        if (dollar == ptr) {
          try {
            int i = 0;
            while (i < maxVarNameLen) {
              varName[i++] = underlying->readS8();
              varName[i] = 0;
              subst = substitutor->substitute(varName);
              if (subst) {
                ptr = (U8*)subst;
                end = (U8*)subst + strlen(subst);
                break;
              }
            }
          } catch (EndOfStream&) {
          }

          if (!subst)
            dollar = (const U8*)memchr(ptr+1, '$', end-ptr-1);
        }
        if (!subst && dollar) end = dollar;
      }

      if (itemSize * nItems > end - ptr)
        nItems = (end - ptr) / itemSize;

      return nItems;
    }

    InStream* underlying;
    const U8* dollar;
    Substitutor* substitutor;
    char* varName;
    char* subst;
    int maxVarNameLen;
  };
}
#endif
