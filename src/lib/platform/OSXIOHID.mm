/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "platform/OSXIOHID.h"

#include "base/Log.h"

#include <IOKit/hidsystem/event_status_driver.h>
#include <AppKit/NSEvent.h>
#include <IOKit/hidsystem/IOHIDLib.h>

static io_connect_t getEventDriver(void)
{
    static mach_port_t sEventDrvrRef = 0;
    mach_port_t masterPort, service, iter;
    kern_return_t kr;

    if (!sEventDrvrRef) {
        // Get master device port
        kr = IOMasterPort(bootstrap_port, &masterPort);
        assert(KERN_SUCCESS == kr);

        kr = IOServiceGetMatchingServices(masterPort,
                IOServiceMatching(kIOHIDSystemClass), &iter);
        assert(KERN_SUCCESS == kr);

        service = IOIteratorNext(iter);
        assert(service);

        kr = IOServiceOpen(service, mach_task_self(),
                kIOHIDParamConnectType, &sEventDrvrRef);
        assert(KERN_SUCCESS == kr);

        IOObjectRelease(service);
        IOObjectRelease(iter);
    }

    return sEventDrvrRef;
}

void
OSXIOHID::postModifierKeys(UInt32 mask)
{
    NXEventData event;
    bzero(&event, sizeof(NXEventData));
    IOGPoint loc = { 0, 0 };
    kern_return_t kr;
    kr = IOHIDPostEvent(getEventDriver(), NX_FLAGSCHANGED, loc,
            &event, kNXEventDataVersion, mask, true);
    assert(KERN_SUCCESS == kr);
}

void
OSXIOHID::postKey(const UInt8 virtualKeyCode,
                  const bool down)
{
    NXEventData event;
    bzero(&event, sizeof(NXEventData));
    IOGPoint loc = { 0, 0 };
    event.key.repeat = false;
    event.key.keyCode = virtualKeyCode;
    event.key.origCharSet = event.key.charSet = NX_ASCIISET;
    event.key.origCharCode = event.key.charCode = 0;
    kern_return_t kr;
    kr = IOHIDPostEvent(getEventDriver(),
            down ? NX_KEYDOWN : NX_KEYUP,
            loc, &event, kNXEventDataVersion, 0, false);
    assert(KERN_SUCCESS == kr);
}

void
OSXIOHID::fakeMouseButton(UInt32 button, bool press)
{
    NXEventData event;
    memset (&event, 0, sizeof(event));

    // Mouse presses actually generate two events, one with a bitfield of buttons, one with a button number
    event.compound.subType = NX_SUBTYPE_AUX_MOUSE_BUTTONS;
    event.compound.misc.L[0] = (1 << button);
    event.compound.misc.L[1] = press ? (1 << button) : 0;
    postMouseEvent(getEventDriver(), NX_SYSDEFINED, &event, 0, 0);

    memset(&event, 0, sizeof(event));
    event.mouse.buttonNumber = button;
    UInt32 type;

    switch (button){
    case 0:
        type = press ? NX_LMOUSEDOWN : NX_LMOUSEUP;
        break;
    case 1:
        type = press ? NX_RMOUSEDOWN : NX_RMOUSEUP;
        break;
    default:
        type = press ? NX_OMOUSEDOWN : NX_OMOUSEUP;
    }

    if (press) {
        event.mouse.pressure = 255;
    }

    postMouseEvent(getEventDriver(), type, &event, 0, 0);
}

void
OSXIOHID::postMouseEvent(
        io_connect_t event, UInt32 type,
        NXEventData* ev, IOOptionBits flags,
        IOOptionBits options)
{

    IOGPoint location = {0, 0};
    if ((options & kIOHIDSetRelativeCursorPosition) && type != NX_MOUSEMOVED){
        // Mouse button only accepts absolute coordinates
        CGEventRef cge = CGEventCreate(nil);
        CGPoint loc = CGEventGetLocation(cge);
        CFRelease(cge);
        location.x = floor(loc.x + ev->mouseMove.dx);
        location.y = floor(loc.y + ev->mouseMove.dy);
        options = (options & ~kIOHIDSetRelativeCursorPosition) | kIOHIDSetCursorPosition;
    }

    kern_return_t res = IOHIDPostEvent(event, type, location, ev, kNXEventDataVersion, flags | NX_NONCOALSESCEDMASK, options);
    if (res != kIOReturnSuccess) {
        LOG((CLOG_DEBUG1 "IOHIDPostEvent event failed: %x\n", res));
    }
}
