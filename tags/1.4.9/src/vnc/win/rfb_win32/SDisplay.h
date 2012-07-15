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

// -=- SDisplay.h
//
// The SDisplay class encapsulates a system display.

#ifndef __RFB_SDISPLAY_H__
#define __RFB_SDISPLAY_H__

#include <rfb/SDesktop.h>
#include <rfb/UpdateTracker.h>
#include <rfb/Configuration.h>
#include <rfb_win32/Handle.h>
#include <rfb_win32/EventManager.h>
#include <rfb_win32/SInput.h>
#include <rfb_win32/Clipboard.h>
#include <rfb_win32/CleanDesktop.h>
#include <rfb_win32/WMCursor.h>
#include <rfb_win32/WMNotifier.h>
#include <rfb_win32/DeviceFrameBuffer.h>
#include <rfb_win32/DeviceContext.h>

namespace rfb {

  namespace win32 {

    //
    // -=- SDisplay
    //

    class SDisplayCore {
    public:
      virtual ~SDisplayCore() {};
      virtual void setScreenRect(const Rect& screenRect_) = 0;
      virtual void flushUpdates() = 0;
      virtual const char* methodName() const = 0;
    };

    class SDisplay : public SDesktop,
      WMMonitor::Notifier,
      Clipboard::Notifier,
      public EventHandler
    {
    public:
      SDisplay();
      virtual ~SDisplay();

      // -=- SDesktop interface

      virtual void start(VNCServer* vs);
      virtual void stop();
      virtual void pointerEvent(const Point& pos, int buttonmask);
      virtual void keyEvent(rdr::U32 key, bool down);
      virtual void clientCutText(const char* str, int len);
      virtual void framebufferUpdateRequest();
      virtual Point getFbSize();

      // -=- Clipboard
      
      virtual void notifyClipboardChanged(const char* text, int len);

      // -=- Display events
      
      virtual void notifyDisplayEvent(WMMonitor::Notifier::DisplayEventType evt);

      // -=- EventHandler interface

      HANDLE getUpdateEvent() {return updateEvent;}
      virtual void processEvent(HANDLE event);

      // -=- Notification of whether or not SDisplay is started

      void setStatusLocation(bool* status) {statusLocation = status;}

      friend class SDisplayCore;

      static IntParameter updateMethod;
      static BoolParameter disableLocalInputs;
      static StringParameter disconnectAction;
      static BoolParameter removeWallpaper;
      static BoolParameter removePattern;
      static BoolParameter disableEffects;

      // -=- Use by VNC Config to determine whether hooks, driver, etc are available
      static bool areHooksAvailable();
      static bool isDriverAvailable();


    protected:
      bool isRestartRequired();
      void startCore();
      void stopCore();
      void restartCore();
      void recreatePixelBuffer(bool force=false);
      bool flushChangeTracker();  // true if flushed, false if empty

      VNCServer* server;

      // -=- Display pixel buffer
      DeviceFrameBuffer* pb;
      DeviceContext* device;

      // -=- The coordinates of Window's entire virtual Screen
      Rect screenRect;

      // -=- All changes are collected in UN-CLIPPED Display coords and merged
      //     When they are to be flushed to the VNCServer, they are changed
      //     to server coords and clipped appropriately.
      SimpleUpdateTracker updates;
      ClippingUpdateTracker clipper;

      // -=- Internal SDisplay implementation
      SDisplayCore* core;
      int updateMethod_;

      // Inputs
      SPointer* ptr;
      SKeyboard* kbd;
      Clipboard* clipboard;
      WMBlockInput* inputs;

      // Desktop state
      WMMonitor* monitor;

      // Desktop optimisation
      CleanDesktop* cleanDesktop;
      bool isWallpaperRemoved;
      bool isPatternRemoved;
      bool areEffectsDisabled;

      // Cursor
      WMCursor* cursor;
      WMCursor::Info old_cursor;
      Region old_cursor_region;
      Point cursor_renderpos;

      // -=- Event signalled to trigger an update to be flushed
      Handle updateEvent;

      // -=- Where to write the active/inactive indicator to
      bool* statusLocation;
    };

  }
}

#endif // __RFB_SDISPLAY_H__
