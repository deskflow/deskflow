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

#ifndef __RDR_TYPES_H__
#define __RDR_TYPES_H__

namespace rdr {

  typedef unsigned char U8;
  typedef unsigned short U16;
  typedef unsigned int U32;
  typedef signed char S8;
  typedef signed short S16;
  typedef signed int S32;

  class U8Array {
  public:
    U8Array() : buf(0) {}
    U8Array(U8* a) : buf(a) {} // note: assumes ownership
    U8Array(int len) : buf(new U8[len]) {}
    ~U8Array() { delete [] buf; }

    // Get the buffer pointer & clear it (i.e. caller takes ownership)
    U8* takeBuf() { U8* tmp = buf; buf = 0; return tmp; }

    U8* buf;
  };

  class U16Array {
  public:
    U16Array() : buf(0) {}
    U16Array(U16* a) : buf(a) {} // note: assumes ownership
    U16Array(int len) : buf(new U16[len]) {}
    ~U16Array() { delete [] buf; }
    U16* takeBuf() { U16* tmp = buf; buf = 0; return tmp; }
    U16* buf;
  };

  class U32Array {
  public:
    U32Array() : buf(0) {}
    U32Array(U32* a) : buf(a) {} // note: assumes ownership
    U32Array(int len) : buf(new U32[len]) {}
    ~U32Array() { delete [] buf; }
    U32* takeBuf() { U32* tmp = buf; buf = 0; return tmp; }
    U32* buf;
  };

} // end of namespace rdr

#endif
