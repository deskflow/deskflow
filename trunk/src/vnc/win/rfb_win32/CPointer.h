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

// -=- CPointer.h
//
// Client-side pointer event handling for Win32

#ifndef __RFB_WIN32_CPOINTER_H__
#define __RFB_WIN32_CPOINTER_H__

#include <rdr/Exception.h>
#include <rfb/Configuration.h>
#include <rfb/InputHandler.h>
#include <rfb/Rect.h>
#include <rfb_win32/IntervalTimer.h>

namespace rfb {

  namespace win32 {

    class CPointer {
    public:
      CPointer();
      ~CPointer();

      void pointerEvent(InputHandler* writer, const Point& pos, int buttonMask);
      void handleTimer(InputHandler* writer, int timerId);

      void setHWND(HWND w) {intervalTimer.setHWND(w); threeTimer.setHWND(w);}
      void setIntervalTimerId(int id) {intervalTimer.setId(id);}
      void set3ButtonTimerId(int id) {threeTimer.setId(id);}

      void enableEmulate3(bool enable) {emulate3 = enable;}
      void enableInterval(int millis) {pointerEventInterval = millis;}
    private:
      Point currPos;
      int currButtonMask;

      bool emulate3;
      int pointerEventInterval;

      void intervalPointerEvent(InputHandler* writer, const Point& pos, int buttonMask);
      IntervalTimer intervalTimer;
      bool intervalQueued;
      Point intervalPos;
      int intervalMask;

      void threePointerEvent(InputHandler* writer, const Point& pos, int buttonMask);
      IntervalTimer threeTimer;
      Point threePos;
      int threeMask;
      bool threeEmulating;
    };

  }; // win32

}; // rfb

#endif // __RFB_WIN32_CPOINTER_H__
