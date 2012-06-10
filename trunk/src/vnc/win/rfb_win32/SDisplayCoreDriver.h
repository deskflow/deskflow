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

// -=- SDisplayCoreDriver.h
//
// Placeholder for SDisplayCore mirror-driver implementation.

#ifndef __RFB_SDISPLAY_CORE_DRIVER_H__
#define __RFB_SDISPLAY_CORE_DRIVER_H__

#include <rfb_win32/SDisplay.h>

namespace rfb {
  namespace win32 {

    class SDisplayCoreDriver: public SDisplayCore {
    public:
      SDisplayCoreDriver(SDisplay* display, UpdateTracker* ut) {
        throw rdr::Exception("Not supported");
      }

      // - Called by SDisplay to inform Core of the screen size
      virtual void setScreenRect(const Rect& screenRect_) {}

      // - Called by SDisplay to flush updates to the specified tracker
      virtual void flushUpdates() {}

      virtual const char* methodName() const { return "VNC Mirror Driver"; }

      // - Determine whether the display driver is installed & usable
      static bool isAvailable() { return false; }
    };

  };
};

#endif
