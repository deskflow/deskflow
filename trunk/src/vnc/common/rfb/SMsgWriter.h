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
// SMsgWriter - class for writing RFB messages on the server side.
//

#ifndef __RFB_SMSGWRITER_H__
#define __RFB_SMSGWRITER_H__

#include <rdr/types.h>
#include <rfb/encodings.h>
#include <rfb/Encoder.h>

namespace rdr { class OutStream; }

namespace rfb {

  class PixelFormat;
  class ConnParams;
  class ImageGetter;
  class ColourMap;
  class Region;
  class UpdateInfo;

  class WriteSetCursorCallback {
  public:
    virtual void writeSetCursorCallback() = 0;
  };

  class SMsgWriter {
  public:
    virtual ~SMsgWriter();

    // writeServerInit() must only be called at the appropriate time in the
    // protocol initialisation.
    virtual void writeServerInit()=0;

    // Methods to write normal protocol messages

    // writeSetColourMapEntries() writes a setColourMapEntries message, using
    // the given ColourMap object to lookup the RGB values of the given range
    // of colours.
    virtual void writeSetColourMapEntries(int firstColour, int nColours,
                                          ColourMap* cm);

    // writeBell() and writeServerCutText() do the obvious thing.
    virtual void writeBell();
    virtual void writeServerCutText(const char* str, int len);

    // writeSetDesktopSize() on a V3 writer won't actually write immediately,
    // but will write the relevant pseudo-rectangle as part of the next update.
    virtual bool writeSetDesktopSize()=0;

    // Like setDestkopSize, we can't just write out a setCursor message
    // immediately on a V3 writer.  Instead of calling writeSetCursor()
    // directly, you must call cursorChange(), and then invoke writeSetCursor()
    // in response to the writeSetCursorCallback() callback.  For a V3 writer
    // this will happen when the next update is sent.
    virtual void cursorChange(WriteSetCursorCallback* cb)=0;
    virtual void writeSetCursor(int width, int height, const Point& hotspot,
                                void* data, void* mask)=0;

    // needFakeUpdate() returns true when an immediate update is needed in
    // order to flush out setDesktopSize or setCursor pseudo-rectangles to the
    // client.
    virtual bool needFakeUpdate();

    // writeFramebufferUpdate() writes a framebuffer update using the given
    // UpdateInfo and ImageGetter.  On a V3 writer this may have
    // pseudo-rectangles for setDesktopSize and setCursor added to it, and so
    // may invoke writeSetCursorCallback().
    virtual void writeFramebufferUpdate(const UpdateInfo& ui, ImageGetter* ig,
                                        Region* updatedRegion);

    // writeRects() accepts an UpdateInfo (changed & copied regions) and an
    // ImageGetter to fetch pixels from.  It then calls writeCopyRect() and
    // writeRect() as appropriate.  writeFramebufferUpdateStart() must be used
    // before the first writeRects() call and writeFrameBufferUpdateEnd() after
    // the last one.  It returns the actual region sent to the client, which
    // may be smaller than the update passed in.
    virtual void writeRects(const UpdateInfo& update, ImageGetter* ig,
                            Region* updatedRegion);

    // To construct a framebuffer update you can call
    // writeFramebufferUpdateStart(), followed by a number of writeCopyRect()s
    // and writeRect()s, finishing with writeFramebufferUpdateEnd().  If you
    // know the exact number of rectangles ahead of time you can specify it to
    // writeFramebufferUpdateStart() which can be more efficient.
    virtual void writeFramebufferUpdateStart(int nRects)=0;
    virtual void writeFramebufferUpdateStart()=0;
    virtual void writeFramebufferUpdateEnd()=0;

    // writeRect() tries to write the given rectangle.  If it is unable to
    // write the whole rectangle it returns false and sets actual to the actual
    // rectangle which was updated.
    virtual bool writeRect(const Rect& r, ImageGetter* ig, Rect* actual);
    virtual bool writeRect(const Rect& r, unsigned int encoding,
                           ImageGetter* ig, Rect* actual);

    virtual void writeCopyRect(const Rect& r, int srcX, int srcY);

    virtual void startRect(const Rect& r, unsigned int enc)=0;
    virtual void endRect()=0;

    ConnParams* getConnParams() { return cp; }
    rdr::OutStream* getOutStream() { return os; }
    rdr::U8* getImageBuf(int required, int requested=0, int* nPixels=0);
    int bpp();

    int getUpdatesSent()           { return updatesSent; }
    int getRectsSent(int encoding) { return rectsSent[encoding]; }
    int getBytesSent(int encoding) { return bytesSent[encoding]; }
    int getRawBytesEquivalent()    { return rawBytesEquivalent; }

    int imageBufIdealSize;

  protected:
    SMsgWriter(ConnParams* cp, rdr::OutStream* os);

    virtual void startMsg(int type)=0;
    virtual void endMsg()=0;

    ConnParams* cp;
    rdr::OutStream* os;

    Encoder* encoders[encodingMax+1];
    int lenBeforeRect;
    unsigned int currentEncoding;
    int updatesSent;
    int bytesSent[encodingMax+1];
    int rectsSent[encodingMax+1];
    int rawBytesEquivalent;

    rdr::U8* imageBuf;
    int imageBufSize;
  };
}
#endif
