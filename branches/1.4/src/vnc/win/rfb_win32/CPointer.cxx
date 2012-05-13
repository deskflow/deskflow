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

#include <windows.h>
#include <rfb/LogWriter.h>
#include <rfb_win32/CPointer.h>

using namespace rfb;
using namespace win32;

static LogWriter vlog("CPointer");


CPointer::CPointer() : currButtonMask(0), intervalQueued(false), threeEmulating(false) {
}

CPointer::~CPointer() {
  intervalTimer.stop();
  threeTimer.stop();
}


void CPointer::pointerEvent(InputHandler* writer, const Point& pos, int buttonMask) {
  //
  // - Duplicate Event Filtering
  //

  bool maskChanged = buttonMask != currButtonMask;
  bool posChanged = !pos.equals(currPos);
  if (!(posChanged || maskChanged))
    return;

  // Pass on the event to the event-interval handler
  threePointerEvent(writer, pos, buttonMask);

  // Save the position and mask
  currPos = pos;
  currButtonMask = buttonMask;
}


//inline abs(int x) {return x>0 ? x : 0;}

int emulate3Mask(int buttonMask) {
  // - Release left & right and press middle
  vlog.debug("emulate3: active");
  buttonMask &= ~5;
  buttonMask |= 2;
  return buttonMask;
}

void CPointer::threePointerEvent(InputHandler* writer, const Point& pos, int buttonMask) {
  //
  // - 3-Button Mouse Emulation
  //

  if (emulate3) {

    bool leftChanged = (buttonMask & 1) != (currButtonMask & 1);
    bool rightChanged = (buttonMask & 4) != (currButtonMask & 4);

    if (leftChanged || rightChanged) {
      // - One of left or right have changed

      if ((buttonMask & 5) == 1 || (buttonMask & 5) == 4) {
        // - One is up, one is down.  Start a timer, so that if it
        //   expires then we know we should actually send this event
        vlog.debug("emulate3: start timer");
        threeTimer.start(100);
        threePos = pos;
        threeMask = buttonMask;
        return;

      } else if (threeTimer.isActive()) {
        // - Both are up or both are down, and we were timing for an emulation event
        //   Stop the timer and flush the stored event
        vlog.debug("emulate3: stop timer (state)");
        threeTimer.stop();
        if (threeEmulating == ((buttonMask & 5) == 5))
          intervalPointerEvent(writer, threePos, threeMask);
        else
          threeEmulating = ((buttonMask & 5) == 5);
      }

    } else {
    
      if (threeTimer.isActive()) {
        // - We are timing for an emulation event

        if (abs(threePos.x - pos.x) <= 4 || abs(threePos.y - pos.y) <= 4) {
          //   If the mouse has moved too far since the button-change event then flush
          vlog.debug("emulate3: stop timer (moved)");
          threeTimer.stop();
          intervalPointerEvent(writer, threePos, threeMask);

        } else {
          //   Otherwise, we ignore the new event
          return;
        }
      }

    }

    // - If neither left nor right are down, stop emulating
    if ((buttonMask & 5) == 0)
      threeEmulating = false;

    // - If emulating, release left & right and press middle
    if (threeEmulating)
      buttonMask = emulate3Mask(buttonMask);

  }

  // - Let the event pass through to the next stage of processing
  intervalPointerEvent(writer, pos, buttonMask);
}

void CPointer::intervalPointerEvent(InputHandler* writer, const Point& pos, int buttonMask) {
  //
  // - Pointer Event Interval
  //
  vlog.write(101, "ptrEvent: %d,%d (%lx)", pos.x, pos.y, buttonMask);

  // Send the event immediately if we haven't sent one for a while
  bool sendNow = !intervalTimer.isActive();

  if (intervalMask != buttonMask) {
    // If the buttons have changed then flush queued events and send now
    sendNow = true;
    if (intervalQueued)
      writer->pointerEvent(intervalPos, intervalMask);
    intervalQueued = false;
  }

  if (!sendNow) {
    // If we're not sending now then just queue the event
    intervalQueued = true;
    intervalPos = pos;
    intervalMask = buttonMask;
  } else {
    // Start the interval timer if required, and send the event
    intervalQueued = false;
    intervalMask = buttonMask;
    if (pointerEventInterval)
      intervalTimer.start(pointerEventInterval);
    writer->pointerEvent(pos, buttonMask);
  }
}

void CPointer::handleTimer(InputHandler* writer, int timerId) {
  if (timerId == intervalTimer.getId()) {
    // Pointer interval has expired - send any queued events
    if (intervalQueued) {
      writer->pointerEvent(intervalPos, intervalMask);
      intervalQueued = false;
    } else {
      intervalTimer.stop();
    }

  } else if (timerId = threeTimer.getId()) {
    // 3-Button emulation timer has expired - send what we've got
    vlog.debug("emulate3: timeout");
    threeTimer.stop();

    // If emulating, release left & right and press middle
    if (threeEmulating)
      threeMask = emulate3Mask(threeMask);

    intervalPointerEvent(writer, threePos, threeMask);
  }
}
