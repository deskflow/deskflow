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

//
// FdInStream streams from a file descriptor.
//

#ifndef __RDR_FDINSTREAM_H__
#define __RDR_FDINSTREAM_H__

#include <rdr/InStream.h>

namespace rdr {

  class FdInStreamBlockCallback {
  public:
    virtual void blockCallback() = 0;
  };

  class FdInStream : public InStream {

  public:

    FdInStream(int fd, int timeoutms=-1, int bufSize=0,
               bool closeWhenDone_=false);
    FdInStream(int fd, FdInStreamBlockCallback* blockCallback, int bufSize=0);
    virtual ~FdInStream();

    void setTimeout(int timeoutms);
    void setBlockCallback(FdInStreamBlockCallback* blockCallback);
    int getFd() { return fd; }
    int pos();
    void readBytes(void* data, int length);

    void startTiming();
    void stopTiming();
    unsigned int kbitsPerSecond();
    unsigned int timeWaited() { return timeWaitedIn100us; }

  protected:
    int overrun(int itemSize, int nItems, bool wait);

  private:
    int readWithTimeoutOrCallback(void* buf, int len, bool wait=true);

    int fd;
    bool closeWhenDone;
    int timeoutms;
    FdInStreamBlockCallback* blockCallback;

    bool timing;
    unsigned int timeWaitedIn100us;
    unsigned int timedKbits;

    int bufSize;
    int offset;
    U8* start;
  };

} // end of namespace rdr

#endif
