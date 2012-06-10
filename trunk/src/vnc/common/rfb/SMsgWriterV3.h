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
#ifndef __RFB_SMSGWRITERV3_H__
#define __RFB_SMSGWRITERV3_H__

#include <rfb/SMsgWriter.h>

namespace rdr { class MemOutStream; }

namespace rfb {
  class SMsgWriterV3 : public SMsgWriter {
  public:
    SMsgWriterV3(ConnParams* cp, rdr::OutStream* os);
    virtual ~SMsgWriterV3();

    virtual void writeServerInit();
    virtual void startMsg(int type);
    virtual void endMsg();
    virtual bool writeSetDesktopSize();
    virtual void cursorChange(WriteSetCursorCallback* cb);
    virtual void writeSetCursor(int width, int height, const Point& hotspot,
                                void* data, void* mask);
    virtual void writeFramebufferUpdateStart(int nRects);
    virtual void writeFramebufferUpdateStart();
    virtual void writeFramebufferUpdateEnd();
    virtual bool needFakeUpdate();
    virtual void startRect(const Rect& r, unsigned int encoding);
    virtual void endRect();

  private:
    rdr::MemOutStream* updateOS;
    rdr::OutStream* realOS;
    int nRectsInUpdate;
    int nRectsInHeader;
    WriteSetCursorCallback* wsccb;
    bool needSetDesktopSize;
  };
}
#endif
