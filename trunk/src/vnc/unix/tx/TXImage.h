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
// TXImage.h
//
// A TXImage represents a rectangular off-screen image in any RFB pixel format.
// By default it will use the "native" pixel format for the screen, which will
// be an 8-bit colourmap unless the X display is TrueColor.  The pixel format
// can be changed via the setPF() method.  The pixel data is accessible via the
// data member inherited from FullFramePixelBuffer, or can be set via the
// fillRect(), imageRect(), copyRect() and maskRect() methods, also inherited
// from PixelBuffer.  A rectangle of the image can be drawn into an X Window
// via the put() method.  If using a colourmap, the setColourMapEntries() and
// updateColourMap() methods must be called to set up the colourmap as
// appropriate.


#ifndef __TXIMAGE_H__
#define __TXIMAGE_H__

#include <X11/Xlib.h>
#include <rfb/PixelBuffer.h>
#include <rfb/ColourMap.h>
#include <rfb/ColourCube.h>
#include <X11/extensions/XShm.h>

namespace rfb { class TransImageGetter; }

class TXImage : public rfb::FullFramePixelBuffer, public rfb::ColourMap {
public:
  TXImage(Display* dpy, int width, int height, Visual* vis=0, int depth=0);
  ~TXImage();

  // resize() resizes the image, preserving the image data where possible.
  void resize(int w, int h);

  // put causes the given rectangle to be drawn onto the given window.
  void put(Window win, GC gc, const rfb::Rect& r);

  // setColourMapEntries() changes some of the entries in the colourmap.
  // However these settings won't take effect until updateColourMap() is
  // called.  This is because recalculating the internal translation table can
  // be expensive.
  void setColourMapEntries(int firstColour, int nColours, rdr::U16* rgbs);
  void updateColourMap();

  bool usingShm() { return shminfo; }

  // PixelBuffer methods
  // width(), height(), getPF() etc are inherited from PixelBuffer
  virtual void setPF(const rfb::PixelFormat& pf);
  virtual int getStride() const;

private:

  // ColourMap method
  virtual void lookup(int index, int* r, int* g, int* b);

  void createXImage();
  void destroyXImage();
  void getNativePixelFormat(Visual* vis, int depth);

  XImage* xim;
  Display* dpy;
  Visual* vis;
  int depth;
  XShmSegmentInfo* shminfo;
  rfb::TransImageGetter* tig;
  rfb::Colour colourMap[256];
  rfb::PixelFormat nativePF;
  rfb::ColourCube* cube;
};

#endif
