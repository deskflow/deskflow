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

// -=- DeviceFrameBuffer.h
//
// The DeviceFrameBuffer class encapsulates the pixel data of a supplied
// Device Context Handle (HDC)

// *** THIS INTERFACE NEEDS TIDYING TO SEPARATE COORDINATE SYSTEMS BETTER ***

#ifndef __RFB_WIN32_DEVICE_FRAME_BUFFER_H__
#define __RFB_WIN32_DEVICE_FRAME_BUFFER_H__

#include <windows.h>
#include <rfb_win32/DIBSectionBuffer.h>
#include <rfb/Cursor.h>
#include <rfb/Region.h>
#include <rfb/Exception.h>
#include <rfb/Configuration.h>

namespace rfb {

  class VNCServer;

  namespace win32 {

    // -=- DeviceFrameBuffer interface

    // DeviceFrameBuffer is passed an HDC referring to a window or to
    // the entire display.  It may also be passed a rectangle specifying
    // the Device-relative coordinates of the actual rectangle to treat
    // as the desktop.

    // Coordinate systems start getting really annoying here.  There are
    // three different "origins" to which coordinates might be relative:
    //
    // Desktop - VNC coordinates, top-left always (0,0)
    // Device - DC coordinates.  Top-left *usually (0,0) but could be other.
    // Window - coordinates relative to the specified sub-rectangle within
    //          the supplied DC.
    // Screen - Coordinates relative to the entire Windows virtual screen.
    //          The virtual screen includes all monitors that are part of
    //          the Windows desktop.

    // The data member is made to point to an internal mirror of the
    // current display data.  Individual rectangles or regions of the
    // buffer can be brought up to date by calling the grab functions.

    class DeviceFrameBuffer : public DIBSectionBuffer {
    public:
      DeviceFrameBuffer(HDC deviceContext, const Rect& area_=Rect());
      virtual ~DeviceFrameBuffer();

      // - FrameBuffer overrides

      virtual void grabRect(const Rect &rect);
      virtual void grabRegion(const Region &region);

      // - DIBSectionBuffer overrides
      
      virtual void setPF(const PixelFormat& pf);
      virtual void setSize(int w, int h);
      
      // - DeviceFrameBuffer specific methods

      void setCursor(HCURSOR c, VNCServer* server);
      void updateColourMap();

      // Set whether grabRect should ignore errors or throw exceptions
      // Only set this if you are sure you'll capture the errors some other way!
      void setIgnoreGrabErrors(bool ie) {ignoreGrabErrors=ie;}
      
      static BoolParameter useCaptureBlt;

    protected:
      // Translate supplied Desktop coordinates into Device-relative coordinates
      // This translation may have been affected at start-time by the supplied sub-rect.
      Point desktopToDevice(const Point p) const {return p.translate(deviceCoords.tl);}

      HDC device;
      DIBSectionBuffer cursorBm;
      Cursor cursor;
      Rect deviceCoords;
      bool ignoreGrabErrors;
    };

  };

};

#endif // __RFB_WIN32_DEVICE_FRAME_BUFFER_H__
