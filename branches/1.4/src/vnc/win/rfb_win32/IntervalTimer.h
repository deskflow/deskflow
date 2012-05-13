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

// -=- IntervalTimer.h
//
// Simple wrapper for standard Win32 timers

#ifndef __RFB_WIN32_INTERVAL_TIMER_H__
#define __RFB_WIN32_INTERVAL_TIMER_H__

namespace rfb {

  namespace win32 {

    struct IntervalTimer {
      IntervalTimer(HWND hwnd_, int id_)
        : active(false), hwnd(hwnd_), id(id_) {
      }
      IntervalTimer() : active(false), hwnd(0), id(0) {
      }
      ~IntervalTimer() {
        stop();
      }

      void start(int interval_) {
        if (!active || interval_ != interval) {
          interval = interval_;
          if (!SetTimer(hwnd, id, interval, 0))
            throw rdr::SystemException("SetTimer", GetLastError());
          active = true;
        }
      }
      void stop() {
        if (active)
          KillTimer(hwnd, id);
        active = false;
      }

      void setHWND(HWND hwnd_) {hwnd=hwnd_;}
      void setId(int id_) {id = id_;}
      int getId() const {return id;}
      bool isActive() const {return active;}

    private:
      HWND hwnd;
      int id;
      bool active;
      int interval;
    };

  }; // win32

}; // rfb

#endif // __RFB_WIN32_INTERVAL_TIMER_H__
