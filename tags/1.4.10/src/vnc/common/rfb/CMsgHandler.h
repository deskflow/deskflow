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
// CMsgHandler - class to handle incoming messages on the client side.
//

#ifndef __RFB_CMSGHANDLER_H__
#define __RFB_CMSGHANDLER_H__

#include <rdr/types.h>
#include <rfb/Pixel.h>
#include <rfb/ConnParams.h>
#include <rfb/Rect.h>

namespace rdr { class InStream; }

namespace rfb {

  class CMsgHandler {
  public:
    CMsgHandler();
    virtual ~CMsgHandler();

    // The following methods are called as corresponding messages are read.  A
    // derived class should override these methods as desired.  Note that for
    // the setDesktopSize(), setPixelFormat() and setName() methods, a derived
    // class should call on to CMsgHandler's methods to set the members of cp
    // appropriately.

    virtual void setDesktopSize(int w, int h);
    virtual void setCursor(int width, int height, const Point& hotspot,
                           void* data, void* mask);
    virtual void setPixelFormat(const PixelFormat& pf);
    virtual void setName(const char* name);
    virtual void serverInit();

    virtual void framebufferUpdateStart();
    virtual void framebufferUpdateEnd();
    virtual void beginRect(const Rect& r, unsigned int encoding);
    virtual void endRect(const Rect& r, unsigned int encoding);

    virtual void setColourMapEntries(int firstColour, int nColours,
				     rdr::U16* rgbs);
    virtual void bell();
    virtual void serverCutText(const char* str, int len);

    virtual void fillRect(const Rect& r, Pixel pix);
    virtual void imageRect(const Rect& r, void* pixels);
    virtual void copyRect(const Rect& r, int srcX, int srcY);

    ConnParams cp;
  };
}
#endif
