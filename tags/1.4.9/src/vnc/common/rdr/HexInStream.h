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

#ifndef __RDR_HEX_INSTREAM_H__
#define __RDR_HEX_INSTREAM_H__

#include <rdr/InStream.h>

namespace rdr {

  class HexInStream : public InStream {
  public:

    HexInStream(InStream& is, int bufSize=0);
    virtual ~HexInStream();

    int pos();

    static bool readHexAndShift(char c, int* v);
    static bool hexStrToBin(const char* s, char** data, int* length);

  protected:
    int overrun(int itemSize, int nItems, bool wait);

  private:
    int bufSize;
    U8* start;
    int offset;

    InStream& in_stream;
  };

} // end of namespace rdr

#endif
