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
#ifndef __RFB_ZRLEENCODER_H__
#define __RFB_ZRLEENCODER_H__

#include <rdr/MemOutStream.h>
#include <rdr/ZlibOutStream.h>
#include <rfb/Encoder.h>

namespace rfb {

  class ZRLEEncoder : public Encoder {
  public:
    static Encoder* create(SMsgWriter* writer);
    virtual bool writeRect(const Rect& r, ImageGetter* ig, Rect* actual);
    virtual ~ZRLEEncoder();

    // setMaxLen() sets the maximum size in bytes of any ZRLE rectangle.  This
    // can be used to stop the MemOutStream from growing too large.  The value
    // must be large enough to allow for at least one row of ZRLE tiles.  So
    // for example for a screen width of 2048 32-bit pixels this is 2K*4*64 =
    // 512Kbytes plus a bit of overhead (the overhead is about 1/16 of the
    // width, in this example about 128 bytes).
    static void setMaxLen(int m) { maxLen = m; }

    // setSharedMos() sets a MemOutStream to be shared amongst all
    // ZRLEEncoders.  Should be called before any ZRLEEncoders are created.
    static void setSharedMos(rdr::MemOutStream* mos_) { sharedMos = mos_; }

  private:
    ZRLEEncoder(SMsgWriter* writer);
    SMsgWriter* writer;
    rdr::ZlibOutStream zos;
    rdr::MemOutStream* mos;
    static rdr::MemOutStream* sharedMos;
    static int maxLen;
  };
}
#endif
