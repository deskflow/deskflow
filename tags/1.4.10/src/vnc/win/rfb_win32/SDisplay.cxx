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

// -=- SDisplay.cxx
//
// The SDisplay class encapsulates a particular system display.

#include <rfb_win32/SDisplay.h>
#include <rfb_win32/Service.h>
#include <rfb_win32/TsSessions.h>
#include <rfb_win32/CleanDesktop.h>
#include <rfb_win32/CurrentUser.h>
#include <rfb_win32/DynamicFn.h>
#include <rfb_win32/MonitorInfo.h>
#include <rfb_win32/SDisplayCorePolling.h>
#include <rfb_win32/SDisplayCoreWMHooks.h>
#include <rfb_win32/SDisplayCoreDriver.h>
#include <rfb/Exception.h>
#include <rfb/LogWriter.h>

#pragma warning(disable: 4996)

using namespace rdr;
using namespace rfb;
using namespace rfb::win32;

static LogWriter vlog("SDisplay");

// - SDisplay-specific configuration options

IntParameter rfb::win32::SDisplay::updateMethod("UpdateMethod",
  "How to discover desktop updates; 0 - Polling, 1 - Application hooking, 2 - Driver hooking.", 1);
BoolParameter rfb::win32::SDisplay::disableLocalInputs("DisableLocalInputs",
  "Disable local keyboard and pointer input while the server is in use", false);
StringParameter rfb::win32::SDisplay::disconnectAction("DisconnectAction",
  "Action to perform when all clients have disconnected.  (None, Lock, Logoff)", "None");
StringParameter displayDevice("DisplayDevice",
  "Display device name of the monitor to be remoted, or empty to export the whole desktop.", "");
BoolParameter rfb::win32::SDisplay::removeWallpaper("RemoveWallpaper",
  "Remove the desktop wallpaper when the server is in use.", false);
BoolParameter rfb::win32::SDisplay::removePattern("RemovePattern",
  "Remove the desktop background pattern when the server is in use.", false);
BoolParameter rfb::win32::SDisplay::disableEffects("DisableEffects",
  "Disable desktop user interface effects when the server is in use.", false);


//////////////////////////////////////////////////////////////////////////////
//
// SDisplay
//

typedef BOOL (WINAPI *_LockWorkStation_proto)();
DynamicFn<_LockWorkStation_proto> _LockWorkStation(_T("user32.dll"), "LockWorkStation");

// -=- Constructor/Destructor

SDisplay::SDisplay()
  : server(0), pb(0), device(0),
    core(0), ptr(0), kbd(0), clipboard(0),
    inputs(0), monitor(0), cleanDesktop(0), cursor(0),
    statusLocation(0)
{
  updateEvent.h = CreateEvent(0, TRUE, FALSE, 0);
}

SDisplay::~SDisplay()
{
  // XXX when the VNCServer has been deleted with clients active, stop()
  // doesn't get called - this ought to be fixed in VNCServerST.  In any event,
  // we should never call any methods on VNCServer once we're being deleted.
  // This is because it is supposed to be guaranteed that the SDesktop exists
  // throughout the lifetime of the VNCServer.  So if we're being deleted, then
  // the VNCServer ought not to exist and therefore we shouldn't invoke any
  // methods on it.  Setting server to zero here ensures that stop() doesn't
  // call setPixelBuffer(0) on the server.
  server = 0;
  if (core) stop();
}


// -=- SDesktop interface

void SDisplay::start(VNCServer* vs)
{
  vlog.debug("starting");

  // Try to make session zero the console session
  if (!inConsoleSession())
    setConsoleSession();

  // Start the SDisplay core
  server = vs;
  startCore();

  vlog.debug("started");

  if (statusLocation) *statusLocation = true;
}

void SDisplay::stop()
{
  vlog.debug("stopping");

  // If we successfully start()ed then perform the DisconnectAction
  if (core) {
    CurrentUserToken cut;
    CharArray action = disconnectAction.getData();
    if (stricmp(action.buf, "Logoff") == 0) {
      if (!cut.h)
        vlog.info("ignoring DisconnectAction=Logoff - no current user");
      else
        ExitWindowsEx(EWX_LOGOFF, 0);
    } else if (stricmp(action.buf, "Lock") == 0) {
      if (!cut.h) {
        vlog.info("ignoring DisconnectAction=Lock - no current user");
      } else {
        if (_LockWorkStation.isValid())
          (*_LockWorkStation)();
        else
          ExitWindowsEx(EWX_LOGOFF, 0);
      }
    }
  }

  // Stop the SDisplayCore
  if (server)
    server->setPixelBuffer(0);
  stopCore();
  server = 0;

  vlog.debug("stopped");

  if (statusLocation) *statusLocation = false;
}


void SDisplay::startCore() {

  // Currently, we just check whether we're in the console session, and
  //   fail if not
  if (!inConsoleSession())
    throw rdr::Exception("Console is not session zero - oreconnect to restore Console sessin");
  
  // Switch to the current input desktop
  if (rfb::win32::desktopChangeRequired()) {
    if (!rfb::win32::changeDesktop())
      throw rdr::Exception("unable to switch into input desktop");
  }

  // Initialise the change tracker and clipper
  updates.clear();
  clipper.setUpdateTracker(server);

  // Create the framebuffer object
  recreatePixelBuffer(true);

  // Create the SDisplayCore
  updateMethod_ = updateMethod;
  int tryMethod = updateMethod_;
  while (!core) {
    try {
      if (tryMethod == 2)
        core = new SDisplayCoreDriver(this, &updates);
      else if (tryMethod == 1)
        core = new SDisplayCoreWMHooks(this, &updates);
      else
        core = new SDisplayCorePolling(this, &updates);
      core->setScreenRect(screenRect);
    } catch (rdr::Exception& e) {
      delete core; core = 0;
      if (tryMethod == 0)
        throw rdr::Exception("unable to access desktop");
      tryMethod--;
      vlog.error(e.str());
    }
  }
  vlog.info("Started %s", core->methodName());

  // Start display monitor, clipboard handler and input handlers
  monitor = new WMMonitor;
  monitor->setNotifier(this);
  clipboard = new Clipboard;
  clipboard->setNotifier(this);
  ptr = new SPointer;
  kbd = new SKeyboard;
  inputs = new WMBlockInput;
  cursor = new WMCursor;

  // Apply desktop optimisations
  cleanDesktop = new CleanDesktop;
  if (removePattern)
    cleanDesktop->disablePattern();
  if (removeWallpaper)
    cleanDesktop->disableWallpaper();
  if (disableEffects)
    cleanDesktop->disableEffects();
  isWallpaperRemoved = removeWallpaper;
  isPatternRemoved = removePattern;
  areEffectsDisabled = disableEffects;
}

void SDisplay::stopCore() {
  if (core)
    vlog.info("Stopping %s", core->methodName());
  delete core; core = 0;
  delete pb; pb = 0;
  delete device; device = 0;
  delete monitor; monitor = 0;
  delete clipboard; clipboard = 0;
  delete inputs; inputs = 0;
  delete ptr; ptr = 0;
  delete kbd; kbd = 0;
  delete cleanDesktop; cleanDesktop = 0;
  delete cursor; cursor = 0;
  ResetEvent(updateEvent);
}


bool SDisplay::areHooksAvailable() {
  return WMHooks::areAvailable();
}

bool SDisplay::isDriverAvailable() {
  return SDisplayCoreDriver::isAvailable();
}


bool SDisplay::isRestartRequired() {
  // - We must restart the SDesktop if:
  // 1. We are no longer in the input desktop.
  // 2. The any setting has changed.

  // - Check that our session is the Console
  if (!inConsoleSession())
    return true;

  // - Check that we are in the input desktop
  if (rfb::win32::desktopChangeRequired())
    return true;

  // - Check that the update method setting hasn't changed
  //   NB: updateMethod reflects the *selected* update method, not
  //   necessarily the one in use, since we fall back to simpler
  //   methods if more advanced ones fail!
  if (updateMethod_ != updateMethod)
    return true;

  // - Check that the desktop optimisation settings haven't changed
  //   This isn't very efficient, but it shouldn't change very often!
  if ((isWallpaperRemoved != removeWallpaper) ||
      (isPatternRemoved != removePattern) ||
      (areEffectsDisabled != disableEffects))
    return true;

  return false;
}


void SDisplay::restartCore() {
  vlog.info("restarting");

  // Stop the existing Core  related resources
  stopCore();
  try {
    // Start a new Core if possible
    startCore();
    vlog.info("restarted");
  } catch (rdr::Exception& e) {
    // If startCore() fails then we MUST disconnect all clients,
    // to cause the server to stop() the desktop.
    // Otherwise, the SDesktop is in an inconsistent state
    // and the server will crash.
    server->closeClients(e.str());
  }
}


void SDisplay::pointerEvent(const Point& pos, int buttonmask) {
  if (pb->getRect().contains(pos)) {
    Point screenPos = pos.translate(screenRect.tl);
    // - Check that the SDesktop doesn't need restarting
    if (isRestartRequired())
      restartCore();
    if (ptr)
      ptr->pointerEvent(screenPos, buttonmask);
  }
}

void SDisplay::keyEvent(rdr::U32 key, bool down) {
  // - Check that the SDesktop doesn't need restarting
  if (isRestartRequired())
    restartCore();
  if (kbd)
    kbd->keyEvent(key, down);
}

void SDisplay::clientCutText(const char* text, int len) {
  CharArray clip_sz(len+1);
  memcpy(clip_sz.buf, text, len);
  clip_sz.buf[len] = 0;
  clipboard->setClipText(clip_sz.buf);
}


void SDisplay::framebufferUpdateRequest()
{
  SetEvent(updateEvent);
}

Point SDisplay::getFbSize() {
  bool startAndStop = !core;

  // If not started, do minimal initialisation to get desktop size.
  if (startAndStop)
    recreatePixelBuffer();
  Point result = Point(pb->width(), pb->height());

  // Destroy the initialised structures.
  if (startAndStop)
    stopCore();
  return result;
}


void
SDisplay::notifyClipboardChanged(const char* text, int len) {
  vlog.debug("clipboard text changed");
  if (server)
    server->serverCutText(text, len);
}


void
SDisplay::notifyDisplayEvent(WMMonitor::Notifier::DisplayEventType evt) {
  switch (evt) {
  case WMMonitor::Notifier::DisplaySizeChanged:
    vlog.debug("desktop size changed");
    recreatePixelBuffer();
    break;
  case WMMonitor::Notifier::DisplayPixelFormatChanged:
    vlog.debug("desktop format changed");
    recreatePixelBuffer();
    break;
  case WMMonitor::Notifier::DisplayColourMapChanged:
    vlog.debug("desktop colourmap changed");
    pb->updateColourMap();
    if (server)
      server->setColourMapEntries();
    break;
  default:
    vlog.error("unknown display event received");
  }
}

void
SDisplay::processEvent(HANDLE event) {
  if (event == updateEvent) {
    vlog.write(120, "processEvent");
    ResetEvent(updateEvent);

    // - If the SDisplay isn't even started then quit now
    if (!core) {
      vlog.error("not start()ed");
      return;
    }

    // - Ensure that the disableLocalInputs flag is respected
    inputs->blockInputs(disableLocalInputs);

    // - Only process updates if the server is ready
    if (server && server->clientsReadyForUpdate()) {
      bool try_update = false;

      // - Check that the SDesktop doesn't need restarting
      if (isRestartRequired()) {
        restartCore();
        return;
      }

      // - Flush any updates from the core
      try {
        core->flushUpdates();
      } catch (rdr::Exception& e) {
        vlog.error(e.str());
        restartCore();
        return;
      }

      // Ensure the cursor is up to date
      WMCursor::Info info = cursor->getCursorInfo();
      if (old_cursor != info) {
        // Update the cursor shape if the visibility has changed
        bool set_cursor = info.visible != old_cursor.visible;
        // OR if the cursor is visible and the shape has changed.
        set_cursor |= info.visible && (old_cursor.cursor != info.cursor);

        // Update the cursor shape
        if (set_cursor)
          pb->setCursor(info.visible ? info.cursor : 0, server);

        // Update the cursor position
        // NB: First translate from Screen coordinates to Desktop
        Point desktopPos = info.position.translate(screenRect.tl.negate());
        server->setCursorPos(desktopPos);
        try_update = true;

        old_cursor = info;
      }

      // Flush any changes to the server
      try_update = flushChangeTracker() || try_update;
      if (try_update) {
        server->tryUpdate();
      }
    }
    return;
  }
  throw rdr::Exception("No such event");
}


// -=- Protected methods

void
SDisplay::recreatePixelBuffer(bool force) {
  // Open the specified display device
  //   If no device is specified, open entire screen using GetDC().
  //   Opening the whole display with CreateDC doesn't work on multi-monitor
  //   systems for some reason.
  DeviceContext* new_device = 0;
  TCharArray deviceName(displayDevice.getData());
  if (deviceName.buf[0]) {
    vlog.info("Attaching to device %s", (const char*)CStr(deviceName.buf));
    new_device = new DeviceDC(deviceName.buf);
  }
  if (!new_device) {
    vlog.info("Attaching to virtual desktop");
    new_device = new WindowDC(0);
  }

  // Get the coordinates of the specified dispay device
  Rect newScreenRect;
  if (deviceName.buf[0]) {
    MonitorInfo info(CStr(deviceName.buf));
    newScreenRect = Rect(info.rcMonitor.left, info.rcMonitor.top,
                         info.rcMonitor.right, info.rcMonitor.bottom);
  } else {
    newScreenRect = new_device->getClipBox();
  }

  // If nothing has changed & a recreate has not been forced, delete
  // the new device context and return
  if (pb && !force &&
    newScreenRect.equals(screenRect) &&
    new_device->getPF().equal(pb->getPF())) {
    delete new_device;
    return;
  }

  // Flush any existing changes to the server
  flushChangeTracker();

  // Delete the old pixelbuffer and device context
  vlog.debug("deleting old pixel buffer & device");
  if (pb)
    delete pb;
  if (device)
    delete device;

  // Create a DeviceFrameBuffer attached to the new device
  vlog.debug("creating pixel buffer");
  DeviceFrameBuffer* new_buffer = new DeviceFrameBuffer(*new_device);

  // Replace the old PixelBuffer
  screenRect = newScreenRect;
  pb = new_buffer;
  device = new_device;

  // Initialise the pixels
  pb->grabRegion(pb->getRect());

  // Prevent future grabRect operations from throwing exceptions
  pb->setIgnoreGrabErrors(true);

  // Update the clipping update tracker
  clipper.setClipRect(pb->getRect());

  // Inform the core of the changes
  if (core)
    core->setScreenRect(screenRect);

  // Inform the server of the changes
  if (server)
    server->setPixelBuffer(pb);
}

bool SDisplay::flushChangeTracker() {
  if (updates.is_empty())
    return false;

  vlog.write(120, "flushChangeTracker");

  // Translate the update coordinates from Screen coords to Desktop
  updates.translate(screenRect.tl.negate());

  // Clip the updates & flush them to the server
  updates.copyTo(&clipper);
  updates.clear();
  return true;
}
