/*
 * barrier -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2004 Chris Schoeneman
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

#include "platform/OSXScreen.h"

#include "base/EventQueue.h"
#include "client/Client.h"
#include "platform/OSXClipboard.h"
#include "platform/OSXEventQueueBuffer.h"
#include "platform/OSXKeyState.h"
#include "platform/OSXScreenSaver.h"
#include "platform/OSXDragSimulator.h"
#include "platform/OSXMediaKeySupport.h"
#include "platform/OSXPasteboardPeeker.h"
#include "barrier/Clipboard.h"
#include "barrier/KeyMap.h"
#include "barrier/ClientApp.h"
#include "mt/CondVar.h"
#include "mt/Lock.h"
#include "mt/Mutex.h"
#include "mt/Thread.h"
#include "arch/XArch.h"
#include "base/Log.h"
#include "base/IEventQueue.h"
#include "base/TMethodEventJob.h"
#include "base/TMethodJob.h"

#include <math.h>
#include <mach-o/dyld.h>
#include <AvailabilityMacros.h>
#include <IOKit/hidsystem/event_status_driver.h>
#include <AppKit/NSEvent.h>

// This isn't in any Apple SDK that I know of as of yet.
enum {
	kBarrierEventMouseScroll = 11,
	kBarrierMouseScrollAxisX = 'saxx',
	kBarrierMouseScrollAxisY = 'saxy'
};

enum {
	kCarbonLoopWaitTimeout = 10
};

// TODO: upgrade deprecated function usage in these functions.
void setZeroSuppressionInterval();
void avoidSupression();
void logCursorVisibility();
void avoidHesitatingCursor();

//
// OSXScreen
//

bool					OSXScreen::s_testedForGHOM = false;
bool					OSXScreen::s_hasGHOM	    = false;

OSXScreen::OSXScreen(IEventQueue* events, bool isPrimary, bool autoShowHideCursor) :
	PlatformScreen(events),
	m_isPrimary(isPrimary),
	m_isOnScreen(m_isPrimary),
	m_cursorPosValid(false),
	MouseButtonEventMap(NumButtonIDs),
	m_cursorHidden(false),
	m_dragNumButtonsDown(0),
	m_dragTimer(NULL),
	m_keyState(NULL),
	m_sequenceNumber(0),
	m_screensaver(NULL),
	m_screensaverNotify(false),
	m_ownClipboard(false),
	m_clipboardTimer(NULL),
	m_hiddenWindow(NULL),
	m_userInputWindow(NULL),
	m_switchEventHandlerRef(0),
	m_pmMutex(new Mutex),
	m_pmWatchThread(NULL),
	m_pmThreadReady(new CondVar<bool>(m_pmMutex, false)),
	m_pmRootPort(0),
	m_activeModifierHotKey(0),
	m_activeModifierHotKeyMask(0),
	m_eventTapPort(nullptr),
	m_eventTapRLSR(nullptr),
	m_lastClickTime(0),
	m_clickState(1),
	m_lastSingleClickXCursor(0),
	m_lastSingleClickYCursor(0),
	m_autoShowHideCursor(autoShowHideCursor),
	m_events(events),
	m_getDropTargetThread(NULL),
	m_impl(NULL)
{
	try {
		m_displayID   = CGMainDisplayID();
		updateScreenShape(m_displayID, 0);
		m_screensaver = new OSXScreenSaver(m_events, getEventTarget());
		m_keyState	  = new OSXKeyState(m_events);
		
		// only needed when running as a server.
		if (m_isPrimary) {
		
#if defined(MAC_OS_X_VERSION_10_9)
			// we can't pass options to show the dialog, this must be done by the gui.
			if (!AXIsProcessTrusted()) {
				throw XArch("assistive devices does not trust this process, allow it in system settings.");
			}
#else
			// now deprecated in mavericks.
			if (!AXAPIEnabled()) {
				throw XArch("assistive devices is not enabled, enable it in system settings.");
			}
#endif
		}
		
		// install display manager notification handler
		CGDisplayRegisterReconfigurationCallback(displayReconfigurationCallback, this);

		// install fast user switching event handler
		EventTypeSpec switchEventTypes[2];
		switchEventTypes[0].eventClass = kEventClassSystem;
		switchEventTypes[0].eventKind  = kEventSystemUserSessionDeactivated;
		switchEventTypes[1].eventClass = kEventClassSystem;
		switchEventTypes[1].eventKind  = kEventSystemUserSessionActivated;
		EventHandlerUPP switchEventHandler =
			NewEventHandlerUPP(userSwitchCallback);
		InstallApplicationEventHandler(switchEventHandler, 2, switchEventTypes,
									   this, &m_switchEventHandlerRef);
		DisposeEventHandlerUPP(switchEventHandler);

		constructMouseButtonEventMap();

		// watch for requests to sleep
		m_events->adoptHandler(m_events->forOSXScreen().confirmSleep(),
								getEventTarget(),
								new TMethodEventJob<OSXScreen>(this,
									&OSXScreen::handleConfirmSleep));

		// create thread for monitoring system power state.
		*m_pmThreadReady = false;
#if defined(MAC_OS_X_VERSION_10_7)
		m_carbonLoopMutex = new Mutex();
		m_carbonLoopReady = new CondVar<bool>(m_carbonLoopMutex, false);
#endif
		LOG((CLOG_DEBUG "starting watchSystemPowerThread"));
		m_pmWatchThread = new Thread(new TMethodJob<OSXScreen>
								(this, &OSXScreen::watchSystemPowerThread));
	}
	catch (...) {
		m_events->removeHandler(m_events->forOSXScreen().confirmSleep(),
								getEventTarget());
		if (m_switchEventHandlerRef != 0) {
			RemoveEventHandler(m_switchEventHandlerRef);
		}
		
		CGDisplayRemoveReconfigurationCallback(displayReconfigurationCallback, this);

		delete m_keyState;
		delete m_screensaver;
		throw;
	}

	// install event handlers
	m_events->adoptHandler(Event::kSystem, m_events->getSystemTarget(),
							new TMethodEventJob<OSXScreen>(this,
								&OSXScreen::handleSystemEvent));

	// install the platform event queue
	m_events->adoptBuffer(new OSXEventQueueBuffer(m_events));
}

OSXScreen::~OSXScreen()
{
	disable();
	m_events->adoptBuffer(NULL);
	m_events->removeHandler(Event::kSystem, m_events->getSystemTarget());

	if (m_pmWatchThread) {
		// make sure the thread has setup the runloop.
		{
			Lock lock(m_pmMutex);
			while (!(bool)*m_pmThreadReady) {
				m_pmThreadReady->wait();
			}
		}

		// now exit the thread's runloop and wait for it to exit
		LOG((CLOG_DEBUG "stopping watchSystemPowerThread"));
		CFRunLoopStop(m_pmRunloop);
		m_pmWatchThread->wait();
		delete m_pmWatchThread;
		m_pmWatchThread = NULL;
	}
	delete m_pmThreadReady;
	delete m_pmMutex;

	m_events->removeHandler(m_events->forOSXScreen().confirmSleep(),
								getEventTarget());

	RemoveEventHandler(m_switchEventHandlerRef);

	CGDisplayRemoveReconfigurationCallback(displayReconfigurationCallback, this);

	delete m_keyState;
	delete m_screensaver;
	
#if defined(MAC_OS_X_VERSION_10_7)
	delete m_carbonLoopMutex;
	delete m_carbonLoopReady;
#endif
}

void*
OSXScreen::getEventTarget() const
{
	return const_cast<OSXScreen*>(this);
}

bool
OSXScreen::getClipboard(ClipboardID, IClipboard* dst) const
{
	Clipboard::copy(dst, &m_pasteboard);
	return true;
}

void
OSXScreen::getShape(SInt32& x, SInt32& y, SInt32& w, SInt32& h) const
{
	x = m_x;
	y = m_y;
	w = m_w;
	h = m_h;
}

void
OSXScreen::getCursorPos(SInt32& x, SInt32& y) const
{
	CGEventRef event = CGEventCreate(NULL);
	CGPoint mouse = CGEventGetLocation(event);
	x                = mouse.x;
	y                = mouse.y;
	m_cursorPosValid = true;
	m_xCursor        = x;
	m_yCursor        = y;
	CFRelease(event);
}

void
OSXScreen::reconfigure(UInt32)
{
	// do nothing
}

void
OSXScreen::warpCursor(SInt32 x, SInt32 y)
{
	// move cursor without generating events
	CGPoint pos;
	pos.x = x;
	pos.y = y;
	CGWarpMouseCursorPosition(pos);
	
	// save new cursor position
	m_xCursor        = x;
	m_yCursor        = y;
	m_cursorPosValid = true;
}

void
OSXScreen::fakeInputBegin()
{
	// FIXME -- not implemented
}

void
OSXScreen::fakeInputEnd()
{
	// FIXME -- not implemented
}

SInt32
OSXScreen::getJumpZoneSize() const
{
	return 1;
}

bool
OSXScreen::isAnyMouseButtonDown(UInt32& buttonID) const
{
	if (m_buttonState.test(0)) {
		buttonID = kButtonLeft;
		return true;
	}

	return (GetCurrentButtonState() != 0);
}

void
OSXScreen::getCursorCenter(SInt32& x, SInt32& y) const
{
	x = m_xCenter;
	y = m_yCenter;
}

UInt32
OSXScreen::registerHotKey(KeyID key, KeyModifierMask mask)
{
	// get mac virtual key and modifier mask matching barrier key and mask
	UInt32 macKey, macMask;
	if (!m_keyState->mapBarrierHotKeyToMac(key, mask, macKey, macMask)) {
		LOG((CLOG_DEBUG "could not map hotkey id=%04x mask=%04x", key, mask));
		return 0;
	}
	
	// choose hotkey id
	UInt32 id;
	if (!m_oldHotKeyIDs.empty()) {
		id = m_oldHotKeyIDs.back();
		m_oldHotKeyIDs.pop_back();
	}
	else {
		id = m_hotKeys.size() + 1;
	}

	// if this hot key has modifiers only then we'll handle it specially
	EventHotKeyRef ref = NULL;
	bool okay;
	if (key == kKeyNone) {
		if (m_modifierHotKeys.count(mask) > 0) {
			// already registered
			okay = false;
		}
		else {
			m_modifierHotKeys[mask] = id;
			okay = true;
		}
	}
	else {
		EventHotKeyID hkid = { 'SNRG', (UInt32)id };
		OSStatus status = RegisterEventHotKey(macKey, macMask, hkid, 
								GetApplicationEventTarget(), 0,
								&ref);
		okay = (status == noErr);
		m_hotKeyToIDMap[HotKeyItem(macKey, macMask)] = id;
	}

	if (!okay) {
		m_oldHotKeyIDs.push_back(id);
		m_hotKeyToIDMap.erase(HotKeyItem(macKey, macMask));
		LOG((CLOG_WARN "failed to register hotkey %s (id=%04x mask=%04x)", barrier::KeyMap::formatKey(key, mask).c_str(), key, mask));
		return 0;
	}

	m_hotKeys.insert(std::make_pair(id, HotKeyItem(ref, macKey, macMask)));
	
	LOG((CLOG_DEBUG "registered hotkey %s (id=%04x mask=%04x) as id=%d", barrier::KeyMap::formatKey(key, mask).c_str(), key, mask, id));
	return id;
}

void
OSXScreen::unregisterHotKey(UInt32 id)
{
	// look up hotkey
	HotKeyMap::iterator i = m_hotKeys.find(id);
	if (i == m_hotKeys.end()) {
		return;
	}

	// unregister with OS
	bool okay;
	if (i->second.getRef() != NULL) {
		okay = (UnregisterEventHotKey(i->second.getRef()) == noErr);
	}
	else {
		okay = false;
		// XXX -- this is inefficient
		for (ModifierHotKeyMap::iterator j = m_modifierHotKeys.begin();
								j != m_modifierHotKeys.end(); ++j) {
			if (j->second == id) {
				m_modifierHotKeys.erase(j);
				okay = true;
				break;
			}
		}
	}
	if (!okay) {
		LOG((CLOG_WARN "failed to unregister hotkey id=%d", id));
	}
	else {
		LOG((CLOG_DEBUG "unregistered hotkey id=%d", id));
	}

	// discard hot key from map and record old id for reuse
	m_hotKeyToIDMap.erase(i->second);
	m_hotKeys.erase(i);
	m_oldHotKeyIDs.push_back(id);
	if (m_activeModifierHotKey == id) {
		m_activeModifierHotKey     = 0;
		m_activeModifierHotKeyMask = 0;
	}
}

void
OSXScreen::constructMouseButtonEventMap()
{
	const CGEventType source[NumButtonIDs][3] = {
		{kCGEventLeftMouseUp, kCGEventLeftMouseDragged, kCGEventLeftMouseDown},
		{kCGEventRightMouseUp, kCGEventRightMouseDragged, kCGEventRightMouseDown},
		{kCGEventOtherMouseUp, kCGEventOtherMouseDragged, kCGEventOtherMouseDown},
		{kCGEventOtherMouseUp, kCGEventOtherMouseDragged, kCGEventOtherMouseDown},
		{kCGEventOtherMouseUp, kCGEventOtherMouseDragged, kCGEventOtherMouseDown},
		{kCGEventOtherMouseUp, kCGEventOtherMouseDragged, kCGEventOtherMouseDown}
	};

	for (UInt16 button = 0; button < NumButtonIDs; button++) {
		MouseButtonEventMapType new_map;
		for (UInt16 state = (UInt32) kMouseButtonUp; state < kMouseButtonStateMax; state++) {
			CGEventType curEvent = source[button][state];
			new_map[state] = curEvent;
		}
		MouseButtonEventMap[button] = new_map;
	}
}

void
OSXScreen::postMouseEvent(CGPoint& pos) const
{
	// check if cursor position is valid on the client display configuration
	// stkamp@users.sourceforge.net
	CGDisplayCount displayCount = 0;
	CGGetDisplaysWithPoint(pos, 0, NULL, &displayCount);
	if (displayCount == 0) {
		// cursor position invalid - clamp to bounds of last valid display.
		// find the last valid display using the last cursor position.
		displayCount = 0;
		CGDirectDisplayID displayID;
		CGGetDisplaysWithPoint(CGPointMake(m_xCursor, m_yCursor), 1,
								&displayID, &displayCount);
		if (displayCount != 0) {
			CGRect displayRect = CGDisplayBounds(displayID);
			if (pos.x < displayRect.origin.x) {
				pos.x = displayRect.origin.x;
			}
			else if (pos.x > displayRect.origin.x +
								displayRect.size.width - 1) {
				pos.x = displayRect.origin.x + displayRect.size.width - 1;
			}
			if (pos.y < displayRect.origin.y) {
				pos.y = displayRect.origin.y;
			}
			else if (pos.y > displayRect.origin.y +
								displayRect.size.height - 1) {
				pos.y = displayRect.origin.y + displayRect.size.height - 1;
			}
		}
	}
	
	CGEventType type = kCGEventMouseMoved;

	SInt8 button = m_buttonState.getFirstButtonDown();
	if (button != -1) {
		MouseButtonEventMapType thisButtonType = MouseButtonEventMap[button];
		type = thisButtonType[kMouseButtonDragged];
	}

	CGEventRef event = CGEventCreateMouseEvent(NULL, type, pos, static_cast<CGMouseButton>(button));
    
    // Dragging events also need the click state
    CGEventSetIntegerValueField(event, kCGMouseEventClickState, m_clickState);
    
    // Fix for sticky keys
    CGEventFlags modifiers = m_keyState->getModifierStateAsOSXFlags();
    CGEventSetFlags(event, modifiers);

    // Set movement deltas to fix issues with certain 3D programs
    SInt64 deltaX = pos.x;
    deltaX -= m_xCursor;

    SInt64 deltaY = pos.y;
    deltaY -= m_yCursor;

    CGEventSetIntegerValueField(event, kCGMouseEventDeltaX, deltaX);
    CGEventSetIntegerValueField(event, kCGMouseEventDeltaY, deltaY);

    double deltaFX = deltaX;
    double deltaFY = deltaY;

    CGEventSetDoubleValueField(event, kCGMouseEventDeltaX, deltaFX);
    CGEventSetDoubleValueField(event, kCGMouseEventDeltaY, deltaFY);

	CGEventPost(kCGHIDEventTap, event);
	
	CFRelease(event);
}

void
OSXScreen::fakeMouseButton(ButtonID id, bool press)
{
	// Buttons are indexed from one, but the button down array is indexed from zero
	UInt32 index = mapBarrierButtonToMac(id) - kButtonLeft;
	if (index >= NumButtonIDs) {
		return;
	}
	
	CGPoint pos;
	if (!m_cursorPosValid) {
		SInt32 x, y;
		getCursorPos(x, y);
	}
	pos.x = m_xCursor;
	pos.y = m_yCursor;

	// variable used to detect mouse coordinate differences between
	// old & new mouse clicks. Used in double click detection.
	SInt32 xDiff = m_xCursor - m_lastSingleClickXCursor;
	SInt32 yDiff = m_yCursor - m_lastSingleClickYCursor;
	double diff = sqrt(xDiff * xDiff + yDiff * yDiff);
	// max sqrt(x^2 + y^2) difference allowed to double click
	// since we don't have double click distance in NX APIs
	// we define our own defaults.
	const double maxDiff = sqrt(2) + 0.0001;
    
    double clickTime = [NSEvent doubleClickInterval];
    
    // As long as the click is within the time window and distance window
    // increase clickState (double click, triple click, etc)
    // This will allow for higher than triple click but the quartz documenation
    // does not specify that this should be limited to triple click
    if (press) {
        if ((ARCH->time() - m_lastClickTime) <= clickTime && diff <= maxDiff){
            m_clickState++;
        }
        else {
            m_clickState = 1;
        }
        
        m_lastClickTime = ARCH->time();
    }
    
    if (m_clickState == 1){
        m_lastSingleClickXCursor = m_xCursor;
        m_lastSingleClickYCursor = m_yCursor;
    }
    
    EMouseButtonState state = press ? kMouseButtonDown : kMouseButtonUp;
    
    LOG((CLOG_DEBUG1 "faking mouse button id: %d press: %s", index, press ? "pressed" : "released"));
    
    MouseButtonEventMapType thisButtonMap = MouseButtonEventMap[index];
    CGEventType type = thisButtonMap[state];

    CGEventRef event = CGEventCreateMouseEvent(NULL, type, pos, static_cast<CGMouseButton>(index));
    
    CGEventSetIntegerValueField(event, kCGMouseEventClickState, m_clickState);
    
    // Fix for sticky keys
    CGEventFlags modifiers = m_keyState->getModifierStateAsOSXFlags();
    CGEventSetFlags(event, modifiers);
    
    m_buttonState.set(index, state);
    CGEventPost(kCGHIDEventTap, event);
    
    CFRelease(event);
    
	if (!press && (id == kButtonLeft)) {
		if (m_fakeDraggingStarted) {
			m_getDropTargetThread = new Thread(new TMethodJob<OSXScreen>(
				this, &OSXScreen::getDropTargetThread));
		}
		
		m_draggingStarted = false;
	}
}

void
OSXScreen::getDropTargetThread(void*)
{
#if defined(MAC_OS_X_VERSION_10_7)
	char* cstr = NULL;
	
	// wait for 5 secs for the drop destinaiton string to be filled.
	UInt32 timeout = ARCH->time() + 5;
	
	while (ARCH->time() < timeout) {
		CFStringRef cfstr = getCocoaDropTarget();
		cstr = CFStringRefToUTF8String(cfstr);
		CFRelease(cfstr);
		
		if (cstr != NULL) {
			break;
		}
		ARCH->sleep(.1f);
	}
	
	if (cstr != NULL) {
		LOG((CLOG_DEBUG "drop target: %s", cstr));
		m_dropTarget = cstr;
	}
	else {
		LOG((CLOG_ERR "failed to get drop target"));
		m_dropTarget.clear();
	}
#else
	LOG((CLOG_WARN "drag drop not supported"));
#endif
	m_fakeDraggingStarted = false;
}

void
OSXScreen::fakeMouseMove(SInt32 x, SInt32 y)
{
	if (m_fakeDraggingStarted) {
		m_buttonState.set(0, kMouseButtonDown);
	}
	
	// index 0 means left mouse button
	if (m_buttonState.test(0)) {
		m_draggingStarted = true;
	}
	
	// synthesize event
	CGPoint pos;
	pos.x = x;
	pos.y = y;
	postMouseEvent(pos);

	// save new cursor position
	m_xCursor        = static_cast<SInt32>(pos.x);
	m_yCursor        = static_cast<SInt32>(pos.y);
	m_cursorPosValid = true;
}

void
OSXScreen::fakeMouseRelativeMove(SInt32 dx, SInt32 dy) const
{
	// OS X does not appear to have a fake relative mouse move function.
	// simulate it by getting the current mouse position and adding to
	// that.  this can yield the wrong answer but there's not much else
	// we can do.

	// get current position
	CGEventRef event = CGEventCreate(NULL);
	CGPoint oldPos = CGEventGetLocation(event);
	CFRelease(event);

	// synthesize event
	CGPoint pos;
	m_xCursor = static_cast<SInt32>(oldPos.x);
	m_yCursor = static_cast<SInt32>(oldPos.y);
	pos.x     = oldPos.x + dx;
	pos.y     = oldPos.y + dy;
	postMouseEvent(pos);

	// we now assume we don't know the current cursor position
	m_cursorPosValid = false;
}

void
OSXScreen::fakeMouseWheel(SInt32 xDelta, SInt32 yDelta) const
{
	if (xDelta != 0 || yDelta != 0) {
		// create a scroll event, post it and release it.  not sure if kCGScrollEventUnitLine
		// is the right choice here over kCGScrollEventUnitPixel
		CGEventRef scrollEvent = CGEventCreateScrollWheelEvent(
			NULL, kCGScrollEventUnitLine, 2,
			mapScrollWheelFromBarrier(yDelta),
			-mapScrollWheelFromBarrier(xDelta));
		
        // Fix for sticky keys
        CGEventFlags modifiers = m_keyState->getModifierStateAsOSXFlags();
        CGEventSetFlags(scrollEvent, modifiers);
        
		CGEventPost(kCGHIDEventTap, scrollEvent);
		CFRelease(scrollEvent);
	}
}

void
OSXScreen::showCursor()
{
	LOG((CLOG_DEBUG "showing cursor"));

	CFStringRef propertyString = CFStringCreateWithCString(
		NULL, "SetsCursorInBackground", kCFStringEncodingMacRoman);

	CGSSetConnectionProperty(
		_CGSDefaultConnection(), _CGSDefaultConnection(),
		propertyString, kCFBooleanTrue);

	CFRelease(propertyString);

	CGError error = CGDisplayShowCursor(m_displayID);
	if (error != kCGErrorSuccess) {
		LOG((CLOG_ERR "failed to show cursor, error=%d", error));
	}

	// appears to fix "mouse randomly not showing" bug
	CGAssociateMouseAndMouseCursorPosition(true);

	logCursorVisibility();

	m_cursorHidden = false;
}

void
OSXScreen::hideCursor()
{
	LOG((CLOG_DEBUG "hiding cursor"));

	CFStringRef propertyString = CFStringCreateWithCString(
		NULL, "SetsCursorInBackground", kCFStringEncodingMacRoman);

	CGSSetConnectionProperty(
		_CGSDefaultConnection(), _CGSDefaultConnection(),
		propertyString, kCFBooleanTrue);

	CFRelease(propertyString);

	CGError error = CGDisplayHideCursor(m_displayID);
	if (error != kCGErrorSuccess) {
		LOG((CLOG_ERR "failed to hide cursor, error=%d", error));
	}

	// appears to fix "mouse randomly not hiding" bug
	CGAssociateMouseAndMouseCursorPosition(true);

	logCursorVisibility();

	m_cursorHidden = true;
}

void
OSXScreen::enable()
{
	// watch the clipboard
	m_clipboardTimer = m_events->newTimer(1.0, NULL);
	m_events->adoptHandler(Event::kTimer, m_clipboardTimer,
							new TMethodEventJob<OSXScreen>(this,
								&OSXScreen::handleClipboardCheck));

	if (m_isPrimary) {
		// FIXME -- start watching jump zones
		
		// kCGEventTapOptionDefault = 0x00000000 (Missing in 10.4, so specified literally)
		m_eventTapPort = CGEventTapCreate(kCGHIDEventTap, kCGHeadInsertEventTap, kCGEventTapOptionDefault,
										kCGEventMaskForAllEvents, 
										handleCGInputEvent, 
										this);
	}
	else {
		// FIXME -- prevent system from entering power save mode

		if (m_autoShowHideCursor) {
			hideCursor();
		}

		// warp the mouse to the cursor center
		fakeMouseMove(m_xCenter, m_yCenter);

                // there may be a better way to do this, but we register an event handler even if we're
                // not on the primary display (acting as a client). This way, if a local event comes in
                // (either keyboard or mouse), we can make sure to show the cursor if we've hidden it. 
		m_eventTapPort = CGEventTapCreate(kCGHIDEventTap, kCGHeadInsertEventTap, kCGEventTapOptionDefault,
										kCGEventMaskForAllEvents, 
										handleCGInputEventSecondary, 
										this);
	}

	if (!m_eventTapPort) {
		LOG((CLOG_ERR "failed to create quartz event tap"));
	}

	m_eventTapRLSR = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, m_eventTapPort, 0);
	if (!m_eventTapRLSR) {
		LOG((CLOG_ERR "failed to create a CFRunLoopSourceRef for the quartz event tap"));
	}

	CFRunLoopAddSource(CFRunLoopGetCurrent(), m_eventTapRLSR, kCFRunLoopDefaultMode);
}

void
OSXScreen::disable()
{
	if (m_autoShowHideCursor) {
		showCursor();
	}
    
	// FIXME -- stop watching jump zones, stop capturing input
	
	if (m_eventTapRLSR) {
		CFRunLoopRemoveSource(CFRunLoopGetCurrent(), m_eventTapRLSR, kCFRunLoopDefaultMode);
		CFRelease(m_eventTapRLSR);
		m_eventTapRLSR = nullptr;
	}

	if (m_eventTapPort) {
		CGEventTapEnable(m_eventTapPort, false);
		CFRelease(m_eventTapPort);
		m_eventTapPort = nullptr;
	}
	// FIXME -- allow system to enter power saving mode

	// disable drag handling
	m_dragNumButtonsDown = 0;
	enableDragTimer(false);

	// uninstall clipboard timer
	if (m_clipboardTimer != NULL) {
		m_events->removeHandler(Event::kTimer, m_clipboardTimer);
		m_events->deleteTimer(m_clipboardTimer);
		m_clipboardTimer = NULL;
	}

	m_isOnScreen = m_isPrimary;
}

void
OSXScreen::enter()
{
	showCursor();

	if (m_isPrimary) {
		setZeroSuppressionInterval();
	}
	else {
		// reset buttons
		m_buttonState.reset();

		// patch by Yutaka Tsutano
		// wakes the client screen
		// http://symless.com/spit/issues/details/3287#c12
		io_registry_entry_t entry = IORegistryEntryFromPath(
			kIOMasterPortDefault,
			"IOService:/IOResources/IODisplayWrangler");
		
		if (entry != MACH_PORT_NULL) {
			IORegistryEntrySetCFProperty(entry, CFSTR("IORequestIdle"), kCFBooleanFalse);
			IOObjectRelease(entry);
		}

		avoidSupression();
	}

	// now on screen
	m_isOnScreen = true;
}

bool
OSXScreen::leave()
{
    hideCursor();
    
	if (isDraggingStarted()) {
		String& fileList = getDraggingFilename();
		
		if (!m_isPrimary) {
			if (fileList.empty() == false) {
				ClientApp& app = ClientApp::instance();
				Client* client = app.getClientPtr();
				
				DragInformation di;
				di.setFilename(fileList);
				DragFileList dragFileList;
				dragFileList.push_back(di);
				String info;
				UInt32 fileCount = DragInformation::setupDragInfo(
					dragFileList, info);
				client->sendDragInfo(fileCount, info, info.size());
				LOG((CLOG_DEBUG "send dragging file to server"));
				
				// TODO: what to do with multiple file or even
				// a folder
				client->sendFileToServer(fileList.c_str());
			}
		}
		m_draggingStarted = false;
	}
	
	if (m_isPrimary) {
		avoidHesitatingCursor();

	}

	// now off screen
	m_isOnScreen = false;

	return true;
}

bool
OSXScreen::setClipboard(ClipboardID, const IClipboard* src)
{
	if (src != NULL) {
		LOG((CLOG_DEBUG "setting clipboard"));
		Clipboard::copy(&m_pasteboard, src);	
	}	
	return true;
}

void
OSXScreen::checkClipboards()
{
	LOG((CLOG_DEBUG2 "checking clipboard"));
	if (m_pasteboard.synchronize()) {
		LOG((CLOG_DEBUG "clipboard changed"));
		sendClipboardEvent(m_events->forClipboard().clipboardGrabbed(), kClipboardClipboard);
		sendClipboardEvent(m_events->forClipboard().clipboardGrabbed(), kClipboardSelection);
	}
}

void
OSXScreen::openScreensaver(bool notify)
{
	m_screensaverNotify = notify;
	if (!m_screensaverNotify) {
		m_screensaver->disable();
	}
}

void
OSXScreen::closeScreensaver()
{
	if (!m_screensaverNotify) {
		m_screensaver->enable();
	}
}

void
OSXScreen::screensaver(bool activate)
{
	if (activate) {
		m_screensaver->activate();
	}
	else {
		m_screensaver->deactivate();
	}
}

void
OSXScreen::resetOptions()
{
	// no options
}

void
OSXScreen::setOptions(const OptionsList&)
{
	// no options
}

void
OSXScreen::setSequenceNumber(UInt32 seqNum)
{
	m_sequenceNumber = seqNum;
}

bool
OSXScreen::isPrimary() const
{
	return m_isPrimary;
}

void
OSXScreen::sendEvent(Event::Type type, void* data) const
{
	m_events->addEvent(Event(type, getEventTarget(), data));
}

void
OSXScreen::sendClipboardEvent(Event::Type type, ClipboardID id) const
{
	ClipboardInfo* info   = (ClipboardInfo*)malloc(sizeof(ClipboardInfo));
	info->m_id             = id;
	info->m_sequenceNumber = m_sequenceNumber;
	sendEvent(type, info);
}

void
OSXScreen::handleSystemEvent(const Event& event, void*)
{
	EventRef* carbonEvent = static_cast<EventRef*>(event.getData());
	assert(carbonEvent != NULL);

	UInt32 eventClass = GetEventClass(*carbonEvent);

	switch (eventClass) {
	case kEventClassMouse:
		switch (GetEventKind(*carbonEvent)) {
		case kBarrierEventMouseScroll:
		{
			OSStatus r;
			long xScroll;
			long yScroll;

			// get scroll amount
			r = GetEventParameter(*carbonEvent,
					kBarrierMouseScrollAxisX,
					typeSInt32,
					NULL,
					sizeof(xScroll),
					NULL,
					&xScroll);
			if (r != noErr) {
				xScroll = 0;
			}
			r = GetEventParameter(*carbonEvent,
					kBarrierMouseScrollAxisY,
					typeSInt32,
					NULL,
					sizeof(yScroll),
					NULL,
					&yScroll);
			if (r != noErr) {
				yScroll = 0;
			}

			if (xScroll != 0 || yScroll != 0) {
				onMouseWheel(-mapScrollWheelToBarrier(xScroll),
								mapScrollWheelToBarrier(yScroll));
			}
		}
		}
		break;

	case kEventClassKeyboard: 
			switch (GetEventKind(*carbonEvent)) {
				case kEventHotKeyPressed:
				case kEventHotKeyReleased:
					onHotKey(*carbonEvent);
					break;
			}
			
			break;
			
	case kEventClassWindow:
		// 2nd param was formerly GetWindowEventTarget(m_userInputWindow) which is 32-bit only,
		// however as m_userInputWindow is never initialized to anything we can take advantage of
		// the fact that GetWindowEventTarget(NULL) == NULL
		SendEventToEventTarget(*carbonEvent, NULL);
		switch (GetEventKind(*carbonEvent)) {
		case kEventWindowActivated:
			LOG((CLOG_DEBUG1 "window activated"));
			break;

		case kEventWindowDeactivated:
			LOG((CLOG_DEBUG1 "window deactivated"));
			break;

		case kEventWindowFocusAcquired:
			LOG((CLOG_DEBUG1 "focus acquired"));
			break;

		case kEventWindowFocusRelinquish:
			LOG((CLOG_DEBUG1 "focus released"));
			break;
		}
		break;

	default:
		SendEventToEventTarget(*carbonEvent, GetEventDispatcherTarget());
		break;
	}
}

bool 
OSXScreen::onMouseMove(CGFloat mx, CGFloat my)
{
	LOG((CLOG_DEBUG2 "mouse move %+f,%+f", mx, my));

	CGFloat x = mx - m_xCursor;
	CGFloat y = my - m_yCursor;

	if ((x == 0 && y == 0) || (mx == m_xCenter && mx == m_yCenter)) {
		return true;
	}

	// save position to compute delta of next motion
	m_xCursor = (SInt32)mx;
	m_yCursor = (SInt32)my;

	if (m_isOnScreen) {
		// motion on primary screen
		sendEvent(m_events->forIPrimaryScreen().motionOnPrimary(),
							MotionInfo::alloc(m_xCursor, m_yCursor));
		if (m_buttonState.test(0)) {
			m_draggingStarted = true;
		}
	}
	else {
		// motion on secondary screen.  warp mouse back to
		// center.
		warpCursor(m_xCenter, m_yCenter);

		// examine the motion.  if it's about the distance
		// from the center of the screen to an edge then
		// it's probably a bogus motion that we want to
		// ignore (see warpCursorNoFlush() for a further
		// description).
		static SInt32 bogusZoneSize = 10;
		if (-x + bogusZoneSize > m_xCenter - m_x ||
			 x + bogusZoneSize > m_x + m_w - m_xCenter ||
			-y + bogusZoneSize > m_yCenter - m_y ||
			 y + bogusZoneSize > m_y + m_h - m_yCenter) {
			LOG((CLOG_DEBUG "dropped bogus motion %+d,%+d", x, y));
		}
		else {
			// send motion
			// Accumulate together the move into the running total
			static CGFloat m_xFractionalMove = 0;
			static CGFloat m_yFractionalMove = 0;

			m_xFractionalMove += x;
			m_yFractionalMove += y;

			// Return the integer part
			SInt32 intX = (SInt32)m_xFractionalMove;
			SInt32 intY = (SInt32)m_yFractionalMove;

			// And keep only the fractional part
			m_xFractionalMove -= intX;
			m_yFractionalMove -= intY;
			sendEvent(m_events->forIPrimaryScreen().motionOnSecondary(), MotionInfo::alloc(intX, intY));
		}
	}

	return true;
}

bool				
OSXScreen::onMouseButton(bool pressed, UInt16 macButton)
{
	// Buttons 2 and 3 are inverted on the mac
	ButtonID button = mapMacButtonToBarrier(macButton);

	if (pressed) {
		LOG((CLOG_DEBUG1 "event: button press button=%d", button));
		if (button != kButtonNone) {
			KeyModifierMask mask = m_keyState->getActiveModifiers();
			sendEvent(m_events->forIPrimaryScreen().buttonDown(), ButtonInfo::alloc(button, mask));
		}
	}
	else {
		LOG((CLOG_DEBUG1 "event: button release button=%d", button));
		if (button != kButtonNone) {
			KeyModifierMask mask = m_keyState->getActiveModifiers();
			sendEvent(m_events->forIPrimaryScreen().buttonUp(), ButtonInfo::alloc(button, mask));
		}
	}

	// handle drags with any button other than button 1 or 2
	if (macButton > 2) {
		if (pressed) {
			// one more button
			if (m_dragNumButtonsDown++ == 0) {
				enableDragTimer(true);
			}
		}
		else {
			// one less button
			if (--m_dragNumButtonsDown == 0) {
				enableDragTimer(false);
			}
		}
	}
	
	if (macButton == kButtonLeft) {
		EMouseButtonState state = pressed ? kMouseButtonDown : kMouseButtonUp;
		m_buttonState.set(kButtonLeft - 1, state);
		if (pressed) {
			m_draggingFilename.clear();
			LOG((CLOG_DEBUG2 "dragging file directory is cleared"));
		}
		else {
			if (m_fakeDraggingStarted) {
				m_getDropTargetThread = new Thread(new TMethodJob<OSXScreen>(
																			   this, &OSXScreen::getDropTargetThread));
			}
			
			m_draggingStarted = false;
		}
	}

	return true;
}

bool
OSXScreen::onMouseWheel(SInt32 xDelta, SInt32 yDelta) const
{
	LOG((CLOG_DEBUG1 "event: button wheel delta=%+d,%+d", xDelta, yDelta));
	sendEvent(m_events->forIPrimaryScreen().wheel(), WheelInfo::alloc(xDelta, yDelta));
	return true;
}

void
OSXScreen::handleClipboardCheck(const Event&, void*)
{
	checkClipboards();
}

void
OSXScreen::displayReconfigurationCallback(CGDirectDisplayID displayID, CGDisplayChangeSummaryFlags flags, void* inUserData)
{
	OSXScreen* screen = (OSXScreen*)inUserData;

	// Closing or opening the lid when an external monitor is
    // connected causes an kCGDisplayBeginConfigurationFlag event
	CGDisplayChangeSummaryFlags mask = kCGDisplayBeginConfigurationFlag | kCGDisplayMovedFlag | 
		kCGDisplaySetModeFlag | kCGDisplayAddFlag | kCGDisplayRemoveFlag | 
		kCGDisplayEnabledFlag | kCGDisplayDisabledFlag | 
		kCGDisplayMirrorFlag | kCGDisplayUnMirrorFlag | 
		kCGDisplayDesktopShapeChangedFlag;
 
	LOG((CLOG_DEBUG1 "event: display was reconfigured: %x %x %x", flags, mask, flags & mask));

	if (flags & mask) { /* Something actually did change */
		
		LOG((CLOG_DEBUG1 "event: screen changed shape; refreshing dimensions"));
		screen->updateScreenShape(displayID, flags);
	}
}

bool
OSXScreen::onKey(CGEventRef event)
{
	CGEventType eventKind = CGEventGetType(event);

	// get the key and active modifiers
	UInt32 virtualKey = CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode);
	CGEventFlags macMask = CGEventGetFlags(event);
	LOG((CLOG_DEBUG1 "event: Key event kind: %d, keycode=%d", eventKind, virtualKey));

	// Special handling to track state of modifiers
	if (eventKind == kCGEventFlagsChanged) {
		// get old and new modifier state
		KeyModifierMask oldMask = getActiveModifiers();
		KeyModifierMask newMask = m_keyState->mapModifiersFromOSX(macMask);
		m_keyState->handleModifierKeys(getEventTarget(), oldMask, newMask);

		// if the current set of modifiers exactly matches a modifiers-only
		// hot key then generate a hot key down event.
		if (m_activeModifierHotKey == 0) {
			if (m_modifierHotKeys.count(newMask) > 0) {
				m_activeModifierHotKey     = m_modifierHotKeys[newMask];
				m_activeModifierHotKeyMask = newMask;
				m_events->addEvent(Event(m_events->forIPrimaryScreen().hotKeyDown(),
								getEventTarget(),
								HotKeyInfo::alloc(m_activeModifierHotKey)));
			}
		}

		// if a modifiers-only hot key is active and should no longer be
		// then generate a hot key up event.
		else if (m_activeModifierHotKey != 0) {
			KeyModifierMask mask = (newMask & m_activeModifierHotKeyMask);
			if (mask != m_activeModifierHotKeyMask) {
				m_events->addEvent(Event(m_events->forIPrimaryScreen().hotKeyUp(),
								getEventTarget(),
								HotKeyInfo::alloc(m_activeModifierHotKey)));
				m_activeModifierHotKey     = 0;
				m_activeModifierHotKeyMask = 0;
			}
		}
			
		return true;
	}

	HotKeyToIDMap::const_iterator i = m_hotKeyToIDMap.find(HotKeyItem(virtualKey, m_keyState->mapModifiersToCarbon(macMask) & 0xff00u));
	if (i != m_hotKeyToIDMap.end()) {
		UInt32 id = i->second;
		// determine event type
		Event::Type type;
		//UInt32 eventKind = GetEventKind(event);
		if (eventKind == kCGEventKeyDown) {
			type = m_events->forIPrimaryScreen().hotKeyDown();
		}
		else if (eventKind == kCGEventKeyUp) {
			type = m_events->forIPrimaryScreen().hotKeyUp();
		}
		else {
			return false;
		}
		m_events->addEvent(Event(type, getEventTarget(), HotKeyInfo::alloc(id)));
		return true;
	}

	// decode event type
	bool down	  = (eventKind == kCGEventKeyDown);
	bool up		  = (eventKind == kCGEventKeyUp);
	bool isRepeat = (CGEventGetIntegerValueField(event, kCGKeyboardEventAutorepeat) == 1);

	// map event to keys
	KeyModifierMask mask;
	OSXKeyState::KeyIDs keys;
	KeyButton button = m_keyState->mapKeyFromEvent(keys, &mask, event);
	if (button == 0) {
		return false;
	}

	// check for AltGr in mask.  if set we send neither the AltGr nor
	// the super modifiers to clients then remove AltGr before passing
	// the modifiers to onKey.
	KeyModifierMask sendMask = (mask & ~KeyModifierAltGr);
	if ((mask & KeyModifierAltGr) != 0) {
		sendMask &= ~KeyModifierSuper;
	}
	mask &= ~KeyModifierAltGr;

	// update button state
	if (down) {
		m_keyState->onKey(button, true, mask);
	}
	else if (up) {
		if (!m_keyState->isKeyDown(button)) {
			// up event for a dead key.  throw it away.
			return false;
		}
		m_keyState->onKey(button, false, mask);
	}

	// send key events
	for (OSXKeyState::KeyIDs::const_iterator i = keys.begin();
							i != keys.end(); ++i) {
		m_keyState->sendKeyEvent(getEventTarget(), down, isRepeat,
							*i, sendMask, 1, button);
	}

	return true;
}

void
OSXScreen::onMediaKey(CGEventRef event) 
{
	KeyID keyID;
	bool down;
	bool isRepeat;

	if (!getMediaKeyEventInfo (event, &keyID, &down, &isRepeat)) {
		LOG ((CLOG_ERR "Failed to decode media key event"));
		return;
	}

	LOG ((CLOG_DEBUG2 "Media key event: keyID=0x%02x, %s, repeat=%s",
						keyID, (down ? "down": "up"),
						(isRepeat ? "yes" : "no")));

	KeyButton button = 0;
	KeyModifierMask mask = m_keyState->getActiveModifiers();
	m_keyState->sendKeyEvent(getEventTarget(), down, isRepeat, keyID, mask, 1, button);
}

bool
OSXScreen::onHotKey(EventRef event) const
{
	// get the hotkey id
	EventHotKeyID hkid;
	GetEventParameter(event, kEventParamDirectObject, typeEventHotKeyID,
							NULL, sizeof(EventHotKeyID), NULL, &hkid);
	UInt32 id = hkid.id;

	// determine event type
	Event::Type type;
	UInt32 eventKind = GetEventKind(event);
	if (eventKind == kEventHotKeyPressed) {
		type = m_events->forIPrimaryScreen().hotKeyDown();
	}
	else if (eventKind == kEventHotKeyReleased) {
		type = m_events->forIPrimaryScreen().hotKeyUp();
	}
	else {
		return false;
	}

	m_events->addEvent(Event(type, getEventTarget(),
								HotKeyInfo::alloc(id)));

	return true;
}

ButtonID
OSXScreen::mapBarrierButtonToMac(UInt16 button) const
{
    switch (button) {
    case 1:
        return kButtonLeft;
    case 2:
        return kMacButtonMiddle;
    case 3:
        return kMacButtonRight;
    }

    return static_cast<ButtonID>(button);
}

ButtonID 
OSXScreen::mapMacButtonToBarrier(UInt16 macButton) const
{
	switch (macButton) {
	case 1:
		return kButtonLeft;

	case 2:
		return kButtonRight;

	case 3:
		return kButtonMiddle;
	}
	
	return static_cast<ButtonID>(macButton);
}

SInt32
OSXScreen::mapScrollWheelToBarrier(float x) const
{
	// return accelerated scrolling but not exponentially scaled as it is
	// on the mac.
	double d = (1.0 + getScrollSpeed()) * x / getScrollSpeedFactor();
	return static_cast<SInt32>(120.0 * d);
}

SInt32
OSXScreen::mapScrollWheelFromBarrier(float x) const
{
	// use server's acceleration with a little boost since other platforms
	// take one wheel step as a larger step than the mac does.
	return static_cast<SInt32>(3.0 * x / 120.0);
}

double
OSXScreen::getScrollSpeed() const
{
	double scaling = 0.0;

	CFPropertyListRef pref = ::CFPreferencesCopyValue(
							CFSTR("com.apple.scrollwheel.scaling") , 
							kCFPreferencesAnyApplication, 
							kCFPreferencesCurrentUser,
							kCFPreferencesAnyHost);
	if (pref != NULL) {
		CFTypeID id = CFGetTypeID(pref);
		if (id == CFNumberGetTypeID()) {
			CFNumberRef value = static_cast<CFNumberRef>(pref);
			if (CFNumberGetValue(value, kCFNumberDoubleType, &scaling)) {
				if (scaling < 0.0) {
					scaling = 0.0;
				}
			}
		}
		CFRelease(pref);
	}

	return scaling;
}

double
OSXScreen::getScrollSpeedFactor() const
{
	return pow(10.0, getScrollSpeed());
}

void
OSXScreen::enableDragTimer(bool enable)
{
	if (enable && m_dragTimer == NULL) {
		m_dragTimer = m_events->newTimer(0.01, NULL);
		m_events->adoptHandler(Event::kTimer, m_dragTimer,
							new TMethodEventJob<OSXScreen>(this,
								&OSXScreen::handleDrag));
		CGEventRef event = CGEventCreate(NULL);
		CGPoint mouse = CGEventGetLocation(event);
		m_dragLastPoint.h = (short)mouse.x;
		m_dragLastPoint.v = (short)mouse.y;
		CFRelease(event);
	}
	else if (!enable && m_dragTimer != NULL) {
		m_events->removeHandler(Event::kTimer, m_dragTimer);
		m_events->deleteTimer(m_dragTimer);
		m_dragTimer = NULL;
	}
}

void
OSXScreen::handleDrag(const Event&, void*)
{
	CGEventRef event = CGEventCreate(NULL);
	CGPoint p = CGEventGetLocation(event);
	CFRelease(event);

	if ((short)p.x != m_dragLastPoint.h || (short)p.y != m_dragLastPoint.v) {
		m_dragLastPoint.h = (short)p.x;
		m_dragLastPoint.v = (short)p.y;
		onMouseMove((SInt32)p.x, (SInt32)p.y);
	}
}

void
OSXScreen::updateButtons()
{
	UInt32 buttons = GetCurrentButtonState();

	m_buttonState.overwrite(buttons);
}

IKeyState*
OSXScreen::getKeyState() const
{
	return m_keyState;
}

void
OSXScreen::updateScreenShape(const CGDirectDisplayID, const CGDisplayChangeSummaryFlags flags)
{
	updateScreenShape();
}

void
OSXScreen::updateScreenShape()
{
	// get info for each display
	CGDisplayCount displayCount = 0;

	if (CGGetActiveDisplayList(0, NULL, &displayCount) != CGDisplayNoErr) {
		return;
	}
	
	if (displayCount == 0) {
		return;
	}

	CGDirectDisplayID* displays = new CGDirectDisplayID[displayCount];
	if (displays == NULL) {
		return;
	}

	if (CGGetActiveDisplayList(displayCount,
							displays, &displayCount) != CGDisplayNoErr) {
		delete[] displays;
		return;
	}

	// get smallest rect enclosing all display rects
	CGRect totalBounds = CGRectZero;
	for (CGDisplayCount i = 0; i < displayCount; ++i) {
		CGRect bounds = CGDisplayBounds(displays[i]);
		totalBounds   = CGRectUnion(totalBounds, bounds);
	}

	// get shape of default screen
	m_x = (SInt32)totalBounds.origin.x;
	m_y = (SInt32)totalBounds.origin.y;
	m_w = (SInt32)totalBounds.size.width;
	m_h = (SInt32)totalBounds.size.height;

	// get center of default screen
  CGDirectDisplayID main = CGMainDisplayID();
  const CGRect rect = CGDisplayBounds(main);
  m_xCenter = (rect.origin.x + rect.size.width) / 2;
  m_yCenter = (rect.origin.y + rect.size.height) / 2;

	delete[] displays;
	// We want to notify the peer screen whether we are primary screen or not
	sendEvent(m_events->forIScreen().shapeChanged());

	LOG((CLOG_DEBUG "screen shape: center=%d,%d size=%dx%d on %u %s",
         m_x, m_y, m_w, m_h, displayCount,
         (displayCount == 1) ? "display" : "displays"));
}

#pragma mark - 

//
// FAST USER SWITCH NOTIFICATION SUPPORT
//
// OSXScreen::userSwitchCallback(void*)
// 
// gets called if a fast user switch occurs
//

pascal OSStatus
OSXScreen::userSwitchCallback(EventHandlerCallRef nextHandler,
								EventRef theEvent,
								void* inUserData)
{
	OSXScreen* screen = (OSXScreen*)inUserData;
	UInt32 kind        = GetEventKind(theEvent);
	IEventQueue* events = screen->getEvents();

	if (kind == kEventSystemUserSessionDeactivated) {
		LOG((CLOG_DEBUG "user session deactivated"));
		events->addEvent(Event(events->forIScreen().suspend(),
									screen->getEventTarget()));
	}
	else if (kind == kEventSystemUserSessionActivated) {
		LOG((CLOG_DEBUG "user session activated"));
		events->addEvent(Event(events->forIScreen().resume(),
									screen->getEventTarget()));
	}
	return (CallNextEventHandler(nextHandler, theEvent));
}

#pragma mark - 

//
// SLEEP/WAKEUP NOTIFICATION SUPPORT
//
// OSXScreen::watchSystemPowerThread(void*)
// 
// main of thread monitoring system power (sleep/wakup) using a CFRunLoop
//

void
OSXScreen::watchSystemPowerThread(void*)
{
	io_object_t				notifier;
	IONotificationPortRef	notificationPortRef;
	CFRunLoopSourceRef		runloopSourceRef = 0;

	m_pmRunloop = CFRunLoopGetCurrent();
	// install system power change callback
	m_pmRootPort = IORegisterForSystemPower(this, &notificationPortRef,
											powerChangeCallback, &notifier);
	if (m_pmRootPort == 0) {
		LOG((CLOG_WARN "IORegisterForSystemPower failed"));
	}
	else {
		runloopSourceRef =
			IONotificationPortGetRunLoopSource(notificationPortRef);
		CFRunLoopAddSource(m_pmRunloop, runloopSourceRef,
								kCFRunLoopCommonModes);
	}
	
	// thread is ready
	{
		Lock lock(m_pmMutex);
		*m_pmThreadReady = true;
		m_pmThreadReady->signal();
	}

	// if we were unable to initialize then exit.  we must do this after
	// setting m_pmThreadReady to true otherwise the parent thread will
	// block waiting for it.
	if (m_pmRootPort == 0) {
		LOG((CLOG_WARN "failed to init watchSystemPowerThread"));
		return;
	}

	LOG((CLOG_DEBUG "started watchSystemPowerThread"));
	
	LOG((CLOG_DEBUG "waiting for event loop"));
	m_events->waitForReady();
    
#if defined(MAC_OS_X_VERSION_10_7)
	{
		Lock lockCarbon(m_carbonLoopMutex);
		if (*m_carbonLoopReady == false) {
			
			// we signalling carbon loop ready before starting
			// unless we know how to do it within the loop
			LOG((CLOG_DEBUG "signalling carbon loop ready"));

			*m_carbonLoopReady = true;
			m_carbonLoopReady->signal();
		}
	}
#endif
	
	// start the run loop
	LOG((CLOG_DEBUG "starting carbon loop"));
	CFRunLoopRun();
	LOG((CLOG_DEBUG "carbon loop has stopped"));
	
	// cleanup
	if (notificationPortRef) {
		CFRunLoopRemoveSource(m_pmRunloop,
								runloopSourceRef, kCFRunLoopDefaultMode);
		CFRunLoopSourceInvalidate(runloopSourceRef);
		CFRelease(runloopSourceRef);
	}

	Lock lock(m_pmMutex);
	IODeregisterForSystemPower(&notifier);
	m_pmRootPort = 0;
	LOG((CLOG_DEBUG "stopped watchSystemPowerThread"));
}

void
OSXScreen::powerChangeCallback(void* refcon, io_service_t service,
						  natural_t messageType, void* messageArg)
{
	((OSXScreen*)refcon)->handlePowerChangeRequest(messageType, messageArg);
}

void
OSXScreen::handlePowerChangeRequest(natural_t messageType, void* messageArg)
{
	// we've received a power change notification
	switch (messageType) {
	case kIOMessageSystemWillSleep:
		// OSXScreen has to handle this in the main thread so we have to
		// queue a confirm sleep event here.  we actually don't allow the
		// system to sleep until the event is handled.
		m_events->addEvent(Event(m_events->forOSXScreen().confirmSleep(),
								getEventTarget(), messageArg,
								Event::kDontFreeData));
		return;
			
	case kIOMessageSystemHasPoweredOn:
		LOG((CLOG_DEBUG "system wakeup"));
		m_events->addEvent(Event(m_events->forIScreen().resume(),
								getEventTarget()));
		break;

	default:
		break;
	}

	Lock lock(m_pmMutex);
	if (m_pmRootPort != 0) {
		IOAllowPowerChange(m_pmRootPort, (long)messageArg);
	}
}

void
OSXScreen::handleConfirmSleep(const Event& event, void*)
{
	long messageArg = (long)event.getData();
	if (messageArg != 0) {
		Lock lock(m_pmMutex);
		if (m_pmRootPort != 0) {
			// deliver suspend event immediately.
			m_events->addEvent(Event(m_events->forIScreen().suspend(),
									getEventTarget(), NULL, 
									Event::kDeliverImmediately));
	
			LOG((CLOG_DEBUG "system will sleep"));
			IOAllowPowerChange(m_pmRootPort, messageArg);
		}
	}
}

#pragma mark - 

//
// GLOBAL HOTKEY OPERATING MODE SUPPORT (10.3)
//
// CoreGraphics private API (OSX 10.3)
// Source: http://ichiro.nnip.org/osx/Cocoa/GlobalHotkey.html
//
// We load the functions dynamically because they're not available in
// older SDKs.  We don't use weak linking because we want users of
// older SDKs to build an app that works on newer systems and older
// SDKs will not provide the symbols.
//
// FIXME: This is hosed as of OS 10.5; patches to repair this are
// a good thing.
//
#if 0

#ifdef	__cplusplus
extern "C" {
#endif

typedef int CGSConnection;
typedef enum {
	CGSGlobalHotKeyEnable = 0,
	CGSGlobalHotKeyDisable = 1,
} CGSGlobalHotKeyOperatingMode;

extern CGSConnection _CGSDefaultConnection(void) WEAK_IMPORT_ATTRIBUTE;
extern CGError CGSGetGlobalHotKeyOperatingMode(CGSConnection connection, CGSGlobalHotKeyOperatingMode *mode) WEAK_IMPORT_ATTRIBUTE;
extern CGError CGSSetGlobalHotKeyOperatingMode(CGSConnection connection, CGSGlobalHotKeyOperatingMode mode) WEAK_IMPORT_ATTRIBUTE;

typedef CGSConnection (*_CGSDefaultConnection_t)(void);
typedef CGError (*CGSGetGlobalHotKeyOperatingMode_t)(CGSConnection connection, CGSGlobalHotKeyOperatingMode *mode);
typedef CGError (*CGSSetGlobalHotKeyOperatingMode_t)(CGSConnection connection, CGSGlobalHotKeyOperatingMode mode);

static _CGSDefaultConnection_t				s__CGSDefaultConnection;
static CGSGetGlobalHotKeyOperatingMode_t	s_CGSGetGlobalHotKeyOperatingMode;
static CGSSetGlobalHotKeyOperatingMode_t	s_CGSSetGlobalHotKeyOperatingMode;

#ifdef	__cplusplus
}
#endif

#define LOOKUP(name_)													\
	s_ ## name_ = NULL;													\
	if (NSIsSymbolNameDefinedWithHint("_" #name_, "CoreGraphics")) {	\
		s_ ## name_ = (name_ ## _t)NSAddressOfSymbol(					\
							NSLookupAndBindSymbolWithHint(				\
								"_" #name_, "CoreGraphics"));			\
	}

bool
OSXScreen::isGlobalHotKeyOperatingModeAvailable()
{
	if (!s_testedForGHOM) {
		s_testedForGHOM = true;
		LOOKUP(_CGSDefaultConnection);
		LOOKUP(CGSGetGlobalHotKeyOperatingMode);
		LOOKUP(CGSSetGlobalHotKeyOperatingMode);
		s_hasGHOM = (s__CGSDefaultConnection != NULL &&
					s_CGSGetGlobalHotKeyOperatingMode != NULL &&
					s_CGSSetGlobalHotKeyOperatingMode != NULL);
	}
	return s_hasGHOM;
}

void
OSXScreen::setGlobalHotKeysEnabled(bool enabled)
{
	if (isGlobalHotKeyOperatingModeAvailable()) {
		CGSConnection conn = s__CGSDefaultConnection();

		CGSGlobalHotKeyOperatingMode mode;
		s_CGSGetGlobalHotKeyOperatingMode(conn, &mode);

		if (enabled && mode == CGSGlobalHotKeyDisable) {
			s_CGSSetGlobalHotKeyOperatingMode(conn, CGSGlobalHotKeyEnable);
		}
		else if (!enabled && mode == CGSGlobalHotKeyEnable) {
			s_CGSSetGlobalHotKeyOperatingMode(conn, CGSGlobalHotKeyDisable);
		}
	}
}

bool
OSXScreen::getGlobalHotKeysEnabled()
{
	CGSGlobalHotKeyOperatingMode mode;
	if (isGlobalHotKeyOperatingModeAvailable()) {
		CGSConnection conn = s__CGSDefaultConnection();
		s_CGSGetGlobalHotKeyOperatingMode(conn, &mode);
	}
	else {
		mode = CGSGlobalHotKeyEnable;
	}
	return (mode == CGSGlobalHotKeyEnable);
}

#endif

//
// OSXScreen::HotKeyItem
//

OSXScreen::HotKeyItem::HotKeyItem(UInt32 keycode, UInt32 mask) :
	m_ref(NULL),
	m_keycode(keycode),
	m_mask(mask)
{
	// do nothing
}

OSXScreen::HotKeyItem::HotKeyItem(EventHotKeyRef ref,
				UInt32 keycode, UInt32 mask) :
	m_ref(ref),
	m_keycode(keycode),
	m_mask(mask)
{
	// do nothing
}

EventHotKeyRef
OSXScreen::HotKeyItem::getRef() const
{
	return m_ref;
}

bool
OSXScreen::HotKeyItem::operator<(const HotKeyItem& x) const
{
	return (m_keycode < x.m_keycode ||
			(m_keycode == x.m_keycode && m_mask < x.m_mask));
}

// Quartz event tap support for the secondary display. This makes sure that we
// will show the cursor if a local event comes in while barrier has the cursor
// off the screen.
CGEventRef
OSXScreen::handleCGInputEventSecondary(
	CGEventTapProxy proxy,
	CGEventType type,
	CGEventRef event,
	void* refcon)
{
	// this fix is really screwing with the correct show/hide behavior. it
	// should be tested better before reintroducing.
	return event;

	OSXScreen* screen = (OSXScreen*)refcon;
	if (screen->m_cursorHidden && type == kCGEventMouseMoved) {

		CGPoint pos = CGEventGetLocation(event);
		if (pos.x != screen->m_xCenter || pos.y != screen->m_yCenter) {

			LOG((CLOG_DEBUG "show cursor on secondary, type=%d pos=%d,%d",
					type, pos.x, pos.y));
			screen->showCursor();
		}
	}
	return event;
}

// Quartz event tap support
CGEventRef
OSXScreen::handleCGInputEvent(CGEventTapProxy proxy,
							   CGEventType type,
							   CGEventRef event,
							   void* refcon)
{
	OSXScreen* screen = (OSXScreen*)refcon;
	CGPoint pos;

	switch(type) {
		case kCGEventLeftMouseDown:
		case kCGEventRightMouseDown:
		case kCGEventOtherMouseDown:
			screen->onMouseButton(true, CGEventGetIntegerValueField(event, kCGMouseEventButtonNumber) + 1);
			break;
		case kCGEventLeftMouseUp:
		case kCGEventRightMouseUp:
		case kCGEventOtherMouseUp:
			screen->onMouseButton(false, CGEventGetIntegerValueField(event, kCGMouseEventButtonNumber) + 1);
			break;
		case kCGEventLeftMouseDragged:
		case kCGEventRightMouseDragged:
		case kCGEventOtherMouseDragged:
		case kCGEventMouseMoved:
			pos = CGEventGetLocation(event);
			screen->onMouseMove(pos.x, pos.y);
			
			// The system ignores our cursor-centering calls if
			// we don't return the event. This should be harmless,
			// but might register as slight movement to other apps
			// on the system. It hasn't been a problem before, though.
			return event;
			break;
		case kCGEventScrollWheel:
			screen->onMouseWheel(screen->mapScrollWheelToBarrier(
								 CGEventGetIntegerValueField(event, kCGScrollWheelEventFixedPtDeltaAxis2) / 65536.0f),
								 screen->mapScrollWheelToBarrier(
								 CGEventGetIntegerValueField(event, kCGScrollWheelEventFixedPtDeltaAxis1) / 65536.0f));
			break;
		case kCGEventKeyDown:
		case kCGEventKeyUp:
		case kCGEventFlagsChanged:
			screen->onKey(event);
			break;
		case kCGEventTapDisabledByTimeout:
			// Re-enable our event-tap
			CGEventTapEnable(screen->m_eventTapPort, true);
			LOG((CLOG_INFO "quartz event tap was disabled by timeout, re-enabling"));
			break;
		case kCGEventTapDisabledByUserInput:
			LOG((CLOG_ERR "quartz event tap was disabled by user input"));
			break;
		case NX_NULLEVENT:
			break;
		default:
			if (type == NX_SYSDEFINED) {
			if (isMediaKeyEvent (event)) {
				LOG((CLOG_DEBUG2 "detected media key event"));
				screen->onMediaKey (event);
			} else {
				LOG((CLOG_DEBUG2 "ignoring unknown system defined event"));
				return event;
			}
			break;
			}
			
			LOG((CLOG_DEBUG3 "unknown quartz event type: 0x%02x", type));
	}
	
	if (screen->m_isOnScreen) {
		return event;
	} else {
		return NULL;
	}
}

void
OSXScreen::MouseButtonState::set(UInt32 button, EMouseButtonState state) 
{
	bool newState = (state == kMouseButtonDown);
	m_buttons.set(button, newState);
}

bool
OSXScreen::MouseButtonState::any() 
{
	return m_buttons.any();
}

void
OSXScreen::MouseButtonState::reset() 
{
	m_buttons.reset();
}

void
OSXScreen::MouseButtonState::overwrite(UInt32 buttons) 
{
	m_buttons = std::bitset<NumButtonIDs>(buttons);
}

bool
OSXScreen::MouseButtonState::test(UInt32 button) const 
{
	return m_buttons.test(button);
}

SInt8
OSXScreen::MouseButtonState::getFirstButtonDown() const 
{
	if (m_buttons.any()) {
		for (unsigned short button = 0; button < m_buttons.size(); button++) {
			if (m_buttons.test(button)) {
				return button;
			}
		}
	}
	return -1;
}

char*
OSXScreen::CFStringRefToUTF8String(CFStringRef aString)
{
	if (aString == NULL) {
		return NULL;
	}
	
	CFIndex length = CFStringGetLength(aString);
	CFIndex maxSize = CFStringGetMaximumSizeForEncoding(
		length,
		kCFStringEncodingUTF8);
	char* buffer = (char*)malloc(maxSize);
	if (CFStringGetCString(aString, buffer, maxSize, kCFStringEncodingUTF8)) {
		return buffer;
	}
	return NULL;
}

void
OSXScreen::fakeDraggingFiles(DragFileList fileList)
{
	m_fakeDraggingStarted = true;
	String fileExt;
	if (fileList.size() == 1) {
		fileExt = DragInformation::getDragFileExtension(
			fileList.at(0).getFilename());
	}

#if defined(MAC_OS_X_VERSION_10_7)
	fakeDragging(fileExt.c_str(), m_xCursor, m_yCursor);
#else
	LOG((CLOG_WARN "drag drop not supported"));
#endif
}

String&
OSXScreen::getDraggingFilename()
{
	if (m_draggingStarted) {
		CFStringRef dragInfo = getDraggedFileURL();
		char* info = NULL;
		info = CFStringRefToUTF8String(dragInfo);
		if (info == NULL) {
			m_draggingFilename.clear();
		}
		else {
			LOG((CLOG_DEBUG "drag info: %s", info));
			CFRelease(dragInfo);
			String fileList(info);
			m_draggingFilename = fileList;
		}

		// fake a escape key down and up then left mouse button up
		fakeKeyDown(kKeyEscape, 8192, 1);
		fakeKeyUp(1);
		fakeMouseButton(kButtonLeft, false);
	}
	return m_draggingFilename;
}

void
OSXScreen::waitForCarbonLoop() const
{
#if defined(MAC_OS_X_VERSION_10_7)
	if (*m_carbonLoopReady) {
		LOG((CLOG_DEBUG "carbon loop already ready"));
		return;
	}

	Lock lock(m_carbonLoopMutex);

	LOG((CLOG_DEBUG "waiting for carbon loop"));

	double timeout = ARCH->time() + kCarbonLoopWaitTimeout;
	while (!m_carbonLoopReady->wait()) {
		if (ARCH->time() > timeout) {
			LOG((CLOG_DEBUG "carbon loop not ready, waiting again"));
			timeout = ARCH->time() + kCarbonLoopWaitTimeout;
		}
	}

	LOG((CLOG_DEBUG "carbon loop ready"));
#endif

}

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

void
setZeroSuppressionInterval()
{
	CGSetLocalEventsSuppressionInterval(0.0);
}

void
avoidSupression()
{
	// avoid suppression of local hardware events
	// stkamp@users.sourceforge.net
	CGSetLocalEventsFilterDuringSupressionState(
							kCGEventFilterMaskPermitAllEvents,
							kCGEventSupressionStateSupressionInterval);
	CGSetLocalEventsFilterDuringSupressionState(
							(kCGEventFilterMaskPermitLocalKeyboardEvents |
							kCGEventFilterMaskPermitSystemDefinedEvents),
							kCGEventSupressionStateRemoteMouseDrag);
}

void
logCursorVisibility()
{
	// CGCursorIsVisible is probably deprecated because its unreliable.
	if (!CGCursorIsVisible()) {
		LOG((CLOG_WARN "cursor may not be visible"));
	}
}

void
avoidHesitatingCursor()
{
	// This used to be necessary to get smooth mouse motion on other screens,
	// but now is just to avoid a hesitating cursor when transitioning to
	// the primary (this) screen.
	CGSetLocalEventsSuppressionInterval(0.0001);
}

#pragma GCC diagnostic error "-Wdeprecated-declarations"
