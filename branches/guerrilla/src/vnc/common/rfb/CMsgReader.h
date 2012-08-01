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
// CMsgReader - class for reading RFB messages on the server side
// (i.e. messages from client to server).
//

#ifndef __RFB_CMSGREADER_H__
#define __RFB_CMSGREADER_H__

#include <rdr/types.h>
#include <rfb/encodings.h>
#include <rfb/Decoder.h>

namespace rdr { class InStream; }

namespace rfb {
  class CMsgHandler;
  struct Rect;

  class CMsgReader {
  public:
    virtual ~CMsgReader();

    virtual void readServerInit()=0;

    // readMsg() reads a message, calling the handler as appropriate.
    virtual void readMsg()=0;

    rdr::InStream* getInStream() { return is; }
    rdr::U8* getImageBuf(int required, int requested=0, int* nPixels=0);
    int bpp();

    int imageBufIdealSize;

  protected:
    virtual void readSetColourMapEntries();
    virtual void readBell();
    virtual void readServerCutText();

    virtual void readFramebufferUpdateStart();
    virtual void readFramebufferUpdateEnd();
    virtual void readRect(const Rect& r, unsigned int encoding);

    virtual void readCopyRect(const Rect& r);

    virtual void readSetCursor(int width, int height, const Point& hotspot);

    CMsgReader(CMsgHandler* handler, rdr::InStream* is);

    CMsgHandler* handler;
    rdr::InStream* is;
    Decoder* decoders[encodingMax+1];
    rdr::U8* imageBuf;
    int imageBufSize;
  };
}
#endif
