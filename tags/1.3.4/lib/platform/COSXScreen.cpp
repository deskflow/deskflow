/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2004 Chris Schoeneman
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "COSXScreen.h"
#include "COSXClipboard.h"
#include "COSXEventQueueBuffer.h"
#include "COSXKeyState.h"
#include "COSXScreenSaver.h"
#include "CClipboard.h"
#include "CKeyMap.h"
#include "CCondVar.h"
#include "CLock.h"
#include "CMutex.h"
#include "CThread.h"
#include "CLog.h"
#include "IEventQueue.h"
#include "TMethodEventJob.h"
#include "TMethodJob.h"
#include "XArch.h"

#include <mach-o/dyld.h>
#include <AvailabilityMacros.h>

// Set some enums for fast user switching if we're building with an SDK
// from before such support was added.
#if !defined(MAC_OS_X_VERSION_10_3) || \
	(MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_3)
enum {
	kEventClassSystem                  = 'macs',
	kEventSystemUserSessionActivated   = 10,
	kEventSystemUserSessionDeactivated = 11
};
#endif

// This isn't in any Apple SDK that I know of as of yet.
enum {
	kSynergyEventMouseScroll = 11,
	kSynergyMouseScrollAxisX = 'saxx',
	kSynergyMouseScrollAxisY = 'saxy'
};

//
// COSXScreen
//

bool					COSXScreen::s_testedForGHOM     = false;
bool					COSXScreen::s_hasGHOM           = false;
CEvent::Type			COSXScreen::s_confirmSleepEvent = CEvent::kUnknown;

COSXScreen::COSXScreen(bool isPrimary) :
	m_isPrimary(isPrimary),
	m_isOnScreen(m_isPrimary),
	m_cursorPosValid(false),
	m_cursorHidden(false),
	m_dragNumButtonsDown(0),
	m_dragTimer(NULL),
	m_keyState(NULL),
	m_sequenceNumber(0),
	m_screensaver(NULL),
	m_screensaverNotify(false),
	m_clipboardTimer(NULL),
	m_hiddenWindow(NULL),
	m_userInputWindow(NULL),
	m_switchEventHandlerRef(0),
	m_pmMutex(new CMutex),
	m_pmWatchThread(NULL),
	m_pmThreadReady(new CCondVar<bool>(m_pmMutex, false)),
	m_activeModifierHotKey(0),
	m_activeModifierHotKeyMask(0)
{
	try {
		m_displayID   = CGMainDisplayID();
		updateScreenShape(m_displayID, 0);
		m_screensaver = new COSXScreenSaver(getEventTarget());
		m_keyState	  = new COSXKeyState();

		if (m_isPrimary) {
			// 1x1 window (to minimze the back buffer allocated for this
			// window.
			Rect bounds = { 100, 100, 101, 101 };

			// m_hiddenWindow is a window meant to let us get mouse moves
			// when the focus is on another computer.  If you get your event
			// from the application event target you'll get every mouse
			// moves. On the other hand the Window event target will only
			// get events when the mouse moves over the window. 
			
			// The ignoreClicks attributes makes it impossible for the
			// user to click on our invisible window. 
			CreateNewWindow(kUtilityWindowClass, 
							kWindowNoShadowAttribute |
							kWindowIgnoreClicksAttribute |
							kWindowNoActivatesAttribute, 
							&bounds, &m_hiddenWindow);
			
			// Make it invisible
			SetWindowAlpha(m_hiddenWindow, 0);
			ShowWindow(m_hiddenWindow);

			// m_userInputWindow is a window meant to let us get mouse moves
			// when the focus is on this computer. 
			Rect inputBounds = { 100, 100, 200, 200 };
			CreateNewWindow(kUtilityWindowClass, 
							kWindowNoShadowAttribute |
							kWindowOpaqueForEventsAttribute |
							kWindowStandardHandlerAttribute, 
							&inputBounds, &m_userInputWindow);

			SetWindowAlpha(m_userInputWindow, 0);
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

		// watch for requests to sleep
		EVENTQUEUE->adoptHandler(COSXScreen::getConfirmSleepEvent(),
								getEventTarget(),
								new TMethodEventJob<COSXScreen>(this,
									&COSXScreen::handleConfirmSleep));

		// create thread for monitoring system power state.
		LOG((CLOG_DEBUG "starting watchSystemPowerThread"));
		m_pmWatchThread = new CThread(new TMethodJob<COSXScreen>
								(this, &COSXScreen::watchSystemPowerThread));
	}
	catch (...) {
		EVENTQUEUE->removeHandler(COSXScreen::getConfirmSleepEvent(),
								getEventTarget());
		if (m_switchEventHandlerRef != 0) {
			RemoveEventHandler(m_switchEventHandlerRef);
		}
		CGDisplayRemoveReconfigurationCallback(displayReconfigurationCallback, this);

		if (m_hiddenWindow) {
			CFRelease(m_hiddenWindow);
			m_hiddenWindow = NULL;
		}

		if (m_userInputWindow) {
			CFRelease(m_userInputWindow);
			m_userInputWindow = NULL;
		}
		delete m_keyState;
		delete m_screensaver;
		throw;
	}

	// install event handlers
	EVENTQUEUE->adoptHandler(CEvent::kSystem, IEventQueue::getSystemTarget(),
							new TMethodEventJob<COSXScreen>(this,
								&COSXScreen::handleSystemEvent));

	// install the platform event queue
	EVENTQUEUE->adoptBuffer(new COSXEventQueueBuffer);
}

COSXScreen::~COSXScreen()
{
	disable();
	EVENTQUEUE->adoptBuffer(NULL);
	EVENTQUEUE->removeHandler(CEvent::kSystem, IEventQueue::getSystemTarget());

	if (m_pmWatchThread) {
		// make sure the thread has setup the runloop.
		{
			CLock lock(m_pmMutex);
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

	EVENTQUEUE->removeHandler(COSXScreen::getConfirmSleepEvent(),
								getEventTarget());

	RemoveEventHandler(m_switchEventHandlerRef);

	CGDisplayRemoveReconfigurationCallback(displayReconfigurationCallback, this);
	if (m_hiddenWindow) {
		CFRelease(m_hiddenWindow);
		m_hiddenWindow = NULL;
	}

	if (m_userInputWindow) {
		CFRelease(m_userInputWindow);
		m_userInputWindow = NULL;
	}

	delete m_keyState;
	delete m_screensaver;
}

void*
COSXScreen::getEventTarget() const
{
	return const_cast<COSXScreen*>(this);
}

bool
COSXScreen::getClipboard(ClipboardID, IClipboard* dst) const
{
	CClipboard::copy(dst, &m_pasteboard);
	return true;
}

void
COSXScreen::getShape(SInt32& x, SInt32& y, SInt32& w, SInt32& h) const
{
	x = m_x;
	y = m_y;
	w = m_w;
	h = m_h;
}

void
COSXScreen::getCursorPos(SInt32& x, SInt32& y) const
{
	Point mouse;
	GetGlobalMouse(&mouse);
	x                = mouse.h;
	y                = mouse.v;
	m_cursorPosValid = true;
	m_xCursor        = x;
	m_yCursor        = y;
}

void
COSXScreen::reconfigure(UInt32)
{
	// do nothing
}

void
COSXScreen::warpCursor(SInt32 x, SInt32 y)
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
COSXScreen::fakeInputBegin()
{
	// FIXME -- not implemented
}

void
COSXScreen::fakeInputEnd()
{
	// FIXME -- not implemented
}

SInt32
COSXScreen::getJumpZoneSize() const
{
	return 1;
}

bool
COSXScreen::isAnyMouseButtonDown() const
{
	return (GetCurrentButtonState() != 0);
}

void
COSXScreen::getCursorCenter(SInt32& x, SInt32& y) const
{
	x = m_xCenter;
	y = m_yCenter;
}

UInt32
COSXScreen::registerHotKey(KeyID key, KeyModifierMask mask)
{
	// get mac virtual key and modifier mask matching synergy key and mask
	UInt32 macKey, macMask;
	if (!m_keyState->mapSynergyHotKeyToMac(key, mask, macKey, macMask)) {
		LOG((CLOG_WARN "could not map hotkey id=%04x mask=%04x", key, mask));
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
		m_hotKeyToIDMap[CHotKeyItem(macKey, macMask)] = id;
	}

	if (!okay) {
		m_oldHotKeyIDs.push_back(id);
		m_hotKeyToIDMap.erase(CHotKeyItem(macKey, macMask));
		LOG((CLOG_WARN "failed to register hotkey %s (id=%04x mask=%04x)", CKeyMap::formatKey(key, mask).c_str(), key, mask));
		return 0;
	}

	m_hotKeys.insert(std::make_pair(id, CHotKeyItem(ref, macKey, macMask)));
	
	LOG((CLOG_DEBUG "registered hotkey %s (id=%04x mask=%04x) as id=%d", CKeyMap::formatKey(key, mask).c_str(), key, mask, id));
	return id;
}

void
COSXScreen::unregisterHotKey(UInt32 id)
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
COSXScreen::postMouseEvent(CGPoint& pos) const
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
 	
	// synthesize event.  CGPostMouseEvent is a particularly good
	// example of a bad API.  we have to shadow the mouse state to
	// use this API and if we want to support more buttons we have
	// to recompile.
	//
	// the order of buttons on the mac is:
	// 1 - Left
	// 2 - Right
	// 3 - Middle
	// Whatever the USB device defined.
	//
	// It is a bit weird that the behaviour of buttons over 3 are dependent
	// on currently plugged in USB devices.
	CGPostMouseEvent(pos, true, sizeof(m_buttons) / sizeof(m_buttons[0]),
				m_buttons[0],
				m_buttons[2],
				m_buttons[1],
				m_buttons[3],
				m_buttons[4]);
}


void
COSXScreen::fakeMouseButton(ButtonID id, bool press) const
{
	// get button index
	UInt32 index = id - kButtonLeft;
	if (index >= sizeof(m_buttons) / sizeof(m_buttons[0])) {
		return;
	}

	// update state
	m_buttons[index] = press;

	CGPoint pos;
	if (!m_cursorPosValid) {
		SInt32 x, y;
		getCursorPos(x, y);
	}
	pos.x = m_xCursor;
	pos.y = m_yCursor;
	postMouseEvent(pos);
}

void
COSXScreen::fakeMouseMove(SInt32 x, SInt32 y) const
{
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
COSXScreen::fakeMouseRelativeMove(SInt32 dx, SInt32 dy) const
{
	// OS X does not appear to have a fake relative mouse move function.
	// simulate it by getting the current mouse position and adding to
	// that.  this can yield the wrong answer but there's not much else
	// we can do.

	// get current position
	Point oldPos;
	GetGlobalMouse(&oldPos);

	// synthesize event
	CGPoint pos;
	m_xCursor = static_cast<SInt32>(oldPos.h);
	m_yCursor = static_cast<SInt32>(oldPos.v);
	pos.x     = oldPos.h + dx;
	pos.y     = oldPos.v + dy;
	postMouseEvent(pos);

	// we now assume we don't know the current cursor position
	m_cursorPosValid = false;
}

void
COSXScreen::fakeMouseWheel(SInt32 xDelta, SInt32 yDelta) const
{
	if (xDelta != 0 || yDelta != 0) {
		CGPostScrollWheelEvent(2, mapScrollWheelFromSynergy(yDelta),
								-mapScrollWheelFromSynergy(xDelta));
	}
}

void
COSXScreen::enable()
{
	// watch the clipboard
	m_clipboardTimer = EVENTQUEUE->newTimer(1.0, NULL);
	EVENTQUEUE->adoptHandler(CEvent::kTimer, m_clipboardTimer,
							new TMethodEventJob<COSXScreen>(this,
								&COSXScreen::handleClipboardCheck));

	if (m_isPrimary) {
		// FIXME -- start watching jump zones
	}
	else {
		// FIXME -- prevent system from entering power save mode

		// hide cursor
		if (!m_cursorHidden) {
//			CGDisplayHideCursor(m_displayID);
			m_cursorHidden = true;
		}

		// warp the mouse to the cursor center
		fakeMouseMove(m_xCenter, m_yCenter);

		// FIXME -- prepare to show cursor if it moves
	}
}

void
COSXScreen::disable()
{
	if (m_isPrimary) {
		// FIXME -- stop watching jump zones, stop capturing input
	}
	else {
		// show cursor
		if (m_cursorHidden) {
//			CGDisplayShowCursor(m_displayID);
			m_cursorHidden = false;
		}

		// FIXME -- allow system to enter power saving mode
	}

	// disable drag handling
	m_dragNumButtonsDown = 0;
	enableDragTimer(false);

	// uninstall clipboard timer
	if (m_clipboardTimer != NULL) {
		EVENTQUEUE->removeHandler(CEvent::kTimer, m_clipboardTimer);
		EVENTQUEUE->deleteTimer(m_clipboardTimer);
		m_clipboardTimer = NULL;
	}

	m_isOnScreen = m_isPrimary;
}

void
COSXScreen::enter()
{
	if (m_isPrimary) {
		// stop capturing input, watch jump zones
		HideWindow( m_userInputWindow );
		ShowWindow( m_hiddenWindow );

		SetMouseCoalescingEnabled(true, NULL);

		CGSetLocalEventsSuppressionInterval(0.0);

		// enable global hotkeys
		//setGlobalHotKeysEnabled(true);
	}
	else {
		// show cursor
		if (m_cursorHidden) {
//			CGDisplayShowCursor(m_displayID);
			m_cursorHidden = false;
		}

		// reset buttons
		for (UInt32 i = 0; i < sizeof(m_buttons) / sizeof(m_buttons[0]); ++i) {
			m_buttons[i] = false;
		}

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

	// now on screen
	m_isOnScreen = true;
}

bool
COSXScreen::leave()
{
	if (m_isPrimary) {
		// warp to center
		warpCursor(m_xCenter, m_yCenter);

		// capture events
		HideWindow(m_hiddenWindow);
		ShowWindow(m_userInputWindow);
		RepositionWindow(m_userInputWindow,
							m_userInputWindow, kWindowCenterOnMainScreen);
		SetUserFocusWindow(m_userInputWindow);

		// The OS will coalesce some events if they are similar enough in a
		// short period of time this is bad for us since we need every event
		// to send it over to other machines.  So disable it.		
		SetMouseCoalescingEnabled(false, NULL);
		CGSetLocalEventsSuppressionInterval(0.0001);

		// disable global hotkeys
		//setGlobalHotKeysEnabled(false);
	}
	else {
		// hide cursor
		if (!m_cursorHidden) {
//			CGDisplayHideCursor(m_displayID);
			m_cursorHidden = true;
		}

		// warp the mouse to the cursor center
		fakeMouseMove(m_xCenter, m_yCenter);

		// FIXME -- prepare to show cursor if it moves

		// take keyboard focus	
		// FIXME
	}

	// now off screen
	m_isOnScreen = false;

	return true;
}

/* FIXME: The next two method really want to end up in COSXClipboard */
bool
COSXScreen::setClipboard(ClipboardID, const IClipboard* src)
{
    if(src != NULL) {
        LOG((CLOG_DEBUG "setting clipboard"));
        CClipboard::copy(&m_pasteboard, src);    
    }    
    return true;
}

void
COSXScreen::checkClipboards()
{
    
    LOG((CLOG_DEBUG "checking clipboard"));
    if (m_pasteboard.synchronize()) {
        LOG((CLOG_DEBUG "clipboard changed"));
        sendClipboardEvent(getClipboardGrabbedEvent(), kClipboardClipboard);
        sendClipboardEvent(getClipboardGrabbedEvent(), kClipboardSelection);
    }
}

void
COSXScreen::openScreensaver(bool notify)
{
	m_screensaverNotify = notify;
	if (!m_screensaverNotify) {
		m_screensaver->disable();
	}
}

void
COSXScreen::closeScreensaver()
{
	if (!m_screensaverNotify) {
		m_screensaver->enable();
	}
}

void
COSXScreen::screensaver(bool activate)
{
	if (activate) {
		m_screensaver->activate();
	}
	else {
		m_screensaver->deactivate();
	}
}

void
COSXScreen::resetOptions()
{
	// no options
}

void
COSXScreen::setOptions(const COptionsList&)
{
	// no options
}

void
COSXScreen::setSequenceNumber(UInt32 seqNum)
{
	m_sequenceNumber = seqNum;
}

bool
COSXScreen::isPrimary() const
{
	return m_isPrimary;
}

void
COSXScreen::sendEvent(CEvent::Type type, void* data) const
{
	EVENTQUEUE->addEvent(CEvent(type, getEventTarget(), data));
}

void
COSXScreen::sendClipboardEvent(CEvent::Type type, ClipboardID id) const
{
	CClipboardInfo* info   = (CClipboardInfo*)malloc(sizeof(CClipboardInfo));
	info->m_id             = id;
	info->m_sequenceNumber = m_sequenceNumber;
	sendEvent(type, info);
}

void
COSXScreen::handleSystemEvent(const CEvent& event, void*)
{
	EventRef* carbonEvent = reinterpret_cast<EventRef*>(event.getData());
	assert(carbonEvent != NULL);

	UInt32 eventClass = GetEventClass(*carbonEvent);

	switch (eventClass) {
	case kEventClassMouse:
		switch (GetEventKind(*carbonEvent)) {
		case kEventMouseDown:
		{
			UInt16 myButton;
			GetEventParameter(*carbonEvent,
					kEventParamMouseButton,
					typeMouseButton,
					NULL,
					sizeof(myButton),
					NULL,
					&myButton);
			onMouseButton(true, myButton);
			break;
		}

		case kEventMouseUp:
		{
			UInt16 myButton;
			GetEventParameter(*carbonEvent,
					kEventParamMouseButton,
					typeMouseButton,
					NULL,
					sizeof(myButton),
					NULL,
					&myButton);
			onMouseButton(false, myButton);
			break;
		}

		case kEventMouseDragged:
		case kEventMouseMoved:
		{
			HIPoint point;
			GetEventParameter(*carbonEvent,
					kEventParamMouseLocation,
					typeHIPoint,
					NULL,
					sizeof(point),
					NULL,
					&point);
			onMouseMove((SInt32)point.x, (SInt32)point.y);
			break;
		}

		case kEventMouseWheelMoved:
		{
			EventMouseWheelAxis axis;
			SInt32 delta;
			GetEventParameter(*carbonEvent,
					kEventParamMouseWheelAxis,
					typeMouseWheelAxis,
					NULL,
					sizeof(axis),
					NULL,
					&axis);
			if (axis == kEventMouseWheelAxisX ||
				axis == kEventMouseWheelAxisY) {
				GetEventParameter(*carbonEvent,
					kEventParamMouseWheelDelta,
					typeLongInteger,
					NULL,
					sizeof(delta),
					NULL,
					&delta);
				if (axis == kEventMouseWheelAxisX) {
					onMouseWheel(-mapScrollWheelToSynergy((SInt32)delta), 0);
				}
				else {
					onMouseWheel(0, mapScrollWheelToSynergy((SInt32)delta));
				}
			}
			break;
		}

		case kSynergyEventMouseScroll:
		{
			OSStatus r;
			long xScroll;
			long yScroll;

			// get scroll amount
			r = GetEventParameter(*carbonEvent,
					kSynergyMouseScrollAxisX,
					typeLongInteger,
					NULL,
					sizeof(xScroll),
					NULL,
					&xScroll);
			if (r != noErr) {
				xScroll = 0;
			}
			r = GetEventParameter(*carbonEvent,
					kSynergyMouseScrollAxisY,
					typeLongInteger,
					NULL,
					sizeof(yScroll),
					NULL,
					&yScroll);
			if (r != noErr) {
				yScroll = 0;
			}

			if (xScroll != 0 || yScroll != 0) {
				onMouseWheel(-mapScrollWheelToSynergy(xScroll),
								mapScrollWheelToSynergy(yScroll));
			}
		}
		}
		break;

	case kEventClassKeyboard: 
		switch (GetEventKind(*carbonEvent)) {
		case kEventRawKeyUp:
		case kEventRawKeyDown:
		case kEventRawKeyRepeat:
		case kEventRawKeyModifiersChanged:
			onKey(*carbonEvent);
			break;

		case kEventHotKeyPressed:
		case kEventHotKeyReleased:
			onHotKey(*carbonEvent);
			break;
		}

		break;

	case kEventClassWindow:
		SendEventToWindow(*carbonEvent, m_userInputWindow);
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
COSXScreen::onMouseMove(SInt32 mx, SInt32 my)
{
	LOG((CLOG_DEBUG2 "mouse move %+d,%+d", mx, my));

	SInt32 x = mx - m_xCursor;
	SInt32 y = my - m_yCursor;

	if ((x == 0 && y == 0) || (mx == m_xCenter && mx == m_yCenter)) {
		return true;
	}

	// save position to compute delta of next motion
	m_xCursor = mx;
	m_yCursor = my;

	if (m_isOnScreen) {
		// motion on primary screen
		sendEvent(getMotionOnPrimaryEvent(),
							CMotionInfo::alloc(m_xCursor, m_yCursor));
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
			sendEvent(getMotionOnSecondaryEvent(), CMotionInfo::alloc(x, y));
		}
	}

	return true;
}

bool				
COSXScreen::onMouseButton(bool pressed, UInt16 macButton)
{
	// Buttons 2 and 3 are inverted on the mac
	ButtonID button = mapMacButtonToSynergy(macButton);

	if (pressed) {
		LOG((CLOG_DEBUG1 "event: button press button=%d", button));
		if (button != kButtonNone) {
			KeyModifierMask mask = m_keyState->getActiveModifiers();
			sendEvent(getButtonDownEvent(), CButtonInfo::alloc(button, mask));
		}
	}
	else {
		LOG((CLOG_DEBUG1 "event: button release button=%d", button));
		if (button != kButtonNone) {
			KeyModifierMask mask = m_keyState->getActiveModifiers();
			sendEvent(getButtonUpEvent(), CButtonInfo::alloc(button, mask));
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

	return true;
}

bool
COSXScreen::onMouseWheel(SInt32 xDelta, SInt32 yDelta) const
{
	LOG((CLOG_DEBUG1 "event: button wheel delta=%+d,%+d", xDelta, yDelta));
	sendEvent(getWheelEvent(), CWheelInfo::alloc(xDelta, yDelta));
	return true;
}

void
COSXScreen::handleClipboardCheck(const CEvent&, void*)
{
	checkClipboards();
}

void 
COSXScreen::displayReconfigurationCallback(CGDirectDisplayID displayID, CGDisplayChangeSummaryFlags flags, void* inUserData)
{
	COSXScreen* screen = (COSXScreen*)inUserData;

	CGDisplayChangeSummaryFlags mask = kCGDisplayMovedFlag | 
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
COSXScreen::onKey(EventRef event)
{
	UInt32 eventKind = GetEventKind(event);

	// get the key and active modifiers
	UInt32 virtualKey, macMask;
	GetEventParameter(event, kEventParamKeyCode, typeUInt32,
							NULL, sizeof(virtualKey), NULL, &virtualKey);
	GetEventParameter(event, kEventParamKeyModifiers, typeUInt32,
							NULL, sizeof(macMask), NULL, &macMask);
	LOG((CLOG_DEBUG1 "event: Key event kind: %d, keycode=%d", eventKind, virtualKey));

	// sadly, OS X doesn't report the virtualKey for modifier keys.
	// virtualKey will be zero for modifier keys.  since that's not good
	// enough we'll have to figure out what the key was.
	if (virtualKey == 0 && eventKind == kEventRawKeyModifiersChanged) {
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
				EVENTQUEUE->addEvent(CEvent(getHotKeyDownEvent(),
								getEventTarget(),
								CHotKeyInfo::alloc(m_activeModifierHotKey)));
			}
		}

		// if a modifiers-only hot key is active and should no longer be
		// then generate a hot key up event.
		else if (m_activeModifierHotKey != 0) {
			KeyModifierMask mask = (newMask & m_activeModifierHotKeyMask);
			if (mask != m_activeModifierHotKeyMask) {
				EVENTQUEUE->addEvent(CEvent(getHotKeyUpEvent(),
								getEventTarget(),
								CHotKeyInfo::alloc(m_activeModifierHotKey)));
				m_activeModifierHotKey     = 0;
				m_activeModifierHotKeyMask = 0;
			}
		}
			
		return true;
	}

	// check for hot key.  when we're on a secondary screen we disable
	// all hotkeys so we can capture the OS defined hot keys as regular
	// keystrokes but that means we don't get our own hot keys either.
	// so we check for a key/modifier match in our hot key map.
	if (!m_isOnScreen) {
		HotKeyToIDMap::const_iterator i =
			m_hotKeyToIDMap.find(CHotKeyItem(virtualKey, macMask & 0xff00u));
		if (i != m_hotKeyToIDMap.end()) {
			UInt32 id = i->second;
	
			// determine event type
			CEvent::Type type;
			UInt32 eventKind = GetEventKind(event);
			if (eventKind == kEventRawKeyDown) {
				type = getHotKeyDownEvent();
			}
			else if (eventKind == kEventRawKeyUp) {
				type = getHotKeyUpEvent();
			}
			else {
				return false;
			}
	
			EVENTQUEUE->addEvent(CEvent(type, getEventTarget(),
										CHotKeyInfo::alloc(id)));
		
			return true;
		}
	}

	// decode event type
	bool down	  = (eventKind == kEventRawKeyDown);
	bool up		  = (eventKind == kEventRawKeyUp);
	bool isRepeat = (eventKind == kEventRawKeyRepeat);

	// map event to keys
	KeyModifierMask mask;
	COSXKeyState::CKeyIDs keys;
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
	for (COSXKeyState::CKeyIDs::const_iterator i = keys.begin();
							i != keys.end(); ++i) {
		m_keyState->sendKeyEvent(getEventTarget(), down, isRepeat,
							*i, sendMask, 1, button);
	}

	return true;
}

bool
COSXScreen::onHotKey(EventRef event) const
{
	// get the hotkey id
	EventHotKeyID hkid;
	GetEventParameter(event, kEventParamDirectObject, typeEventHotKeyID,
							NULL, sizeof(EventHotKeyID), NULL, &hkid);
	UInt32 id = hkid.id;

	// determine event type
	CEvent::Type type;
	UInt32 eventKind = GetEventKind(event);
	if (eventKind == kEventHotKeyPressed) {
		type = getHotKeyDownEvent();
	}
	else if (eventKind == kEventHotKeyReleased) {
		type = getHotKeyUpEvent();
	}
	else {
		return false;
	}

	EVENTQUEUE->addEvent(CEvent(type, getEventTarget(),
								CHotKeyInfo::alloc(id)));

	return true;
}

ButtonID 
COSXScreen::mapMacButtonToSynergy(UInt16 macButton) const
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
COSXScreen::mapScrollWheelToSynergy(SInt32 x) const
{
	// return accelerated scrolling but not exponentially scaled as it is
	// on the mac.
	double d = (1.0 + getScrollSpeed()) * x / getScrollSpeedFactor();
	return static_cast<SInt32>(120.0 * d);
}

SInt32
COSXScreen::mapScrollWheelFromSynergy(SInt32 x) const
{
	// use server's acceleration with a little boost since other platforms
	// take one wheel step as a larger step than the mac does.
	return static_cast<SInt32>(3.0 * x / 120.0);
}

double
COSXScreen::getScrollSpeed() const
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
COSXScreen::getScrollSpeedFactor() const
{
	return pow(10.0, getScrollSpeed());
}

void
COSXScreen::enableDragTimer(bool enable)
{
  UInt32 modifiers;
  MouseTrackingResult res; 

	if (enable && m_dragTimer == NULL) {
		m_dragTimer = EVENTQUEUE->newTimer(0.01, NULL);
		EVENTQUEUE->adoptHandler(CEvent::kTimer, m_dragTimer,
							new TMethodEventJob<COSXScreen>(this,
								&COSXScreen::handleDrag));
    TrackMouseLocationWithOptions(NULL, 0, 0, &m_dragLastPoint, &modifiers, &res);
	}
	else if (!enable && m_dragTimer != NULL) {
		EVENTQUEUE->removeHandler(CEvent::kTimer, m_dragTimer);
		EVENTQUEUE->deleteTimer(m_dragTimer);
		m_dragTimer = NULL;
	}
}

void
COSXScreen::handleDrag(const CEvent&, void*)
{
	Point p;
  UInt32 modifiers;
  MouseTrackingResult res; 

	TrackMouseLocationWithOptions(NULL, 0, 0, &p, &modifiers, &res);

	if (res != kMouseTrackingTimedOut && (p.h != m_dragLastPoint.h || p.v != m_dragLastPoint.v)) {
		m_dragLastPoint = p;
		onMouseMove((SInt32)p.h, (SInt32)p.v);
	}
}

void
COSXScreen::updateButtons()
{
	UInt32 buttons = GetCurrentButtonState();
	for (size_t i = 0; i < sizeof(m_buttons) / sizeof(m_buttons[0]); ++i) {
		m_buttons[i] = ((buttons & (1u << i)) != 0);
	}
}

IKeyState*
COSXScreen::getKeyState() const
{
	return m_keyState;
}

void
COSXScreen::updateScreenShape(const CGDirectDisplayID, const CGDisplayChangeSummaryFlags flags)
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
    if (m_isPrimary && !m_isOnScreen) {
        sendEvent(getShapeChangedEvent());
    }

	LOG((CLOG_DEBUG "screen shape: %d,%d %dx%d on %u %s", m_x, m_y, m_w, m_h, displayCount, (displayCount == 1) ? "display" : "displays"));
}

#pragma mark - 

//
// FAST USER SWITCH NOTIFICATION SUPPORT
//
// COSXScreen::userSwitchCallback(void*)
// 
// gets called if a fast user switch occurs
//

pascal OSStatus
COSXScreen::userSwitchCallback(EventHandlerCallRef nextHandler,
								EventRef theEvent,
								void* inUserData)
{
	COSXScreen* screen = (COSXScreen*)inUserData;
	UInt32 kind        = GetEventKind(theEvent);

	if (kind == kEventSystemUserSessionDeactivated) {
		LOG((CLOG_DEBUG "user session deactivated"));
		EVENTQUEUE->addEvent(CEvent(IScreen::getSuspendEvent(),
									screen->getEventTarget()));
	}
	else if (kind == kEventSystemUserSessionActivated) {
		LOG((CLOG_DEBUG "user session activated"));
		EVENTQUEUE->addEvent(CEvent(IScreen::getResumeEvent(),
									screen->getEventTarget()));
	}
	return (CallNextEventHandler(nextHandler, theEvent));
}

#pragma mark - 

//
// SLEEP/WAKEUP NOTIFICATION SUPPORT
//
// COSXScreen::watchSystemPowerThread(void*)
// 
// main of thread monitoring system power (sleep/wakup) using a CFRunLoop
//

void
COSXScreen::watchSystemPowerThread(void*)
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
		CLock lock(m_pmMutex);
		*m_pmThreadReady = true;
		m_pmThreadReady->signal();
	}

	// if we were unable to initialize then exit.  we must do this after
	// setting m_pmThreadReady to true otherwise the parent thread will
	// block waiting for it.
	if (m_pmRootPort == 0) {
		return;
	}

	// start the run loop
	LOG((CLOG_DEBUG "started watchSystemPowerThread"));
	CFRunLoopRun();
	
	// cleanup
	if (notificationPortRef) {
		CFRunLoopRemoveSource(m_pmRunloop,
								runloopSourceRef, kCFRunLoopDefaultMode);
		CFRunLoopSourceInvalidate(runloopSourceRef);
		CFRelease(runloopSourceRef);
	}

	CLock lock(m_pmMutex);
	IODeregisterForSystemPower(&notifier);
	m_pmRootPort = 0;
	LOG((CLOG_DEBUG "stopped watchSystemPowerThread"));
}

void
COSXScreen::powerChangeCallback(void* refcon, io_service_t service,
						  natural_t messageType, void* messageArg)
{
	((COSXScreen*)refcon)->handlePowerChangeRequest(messageType, messageArg);
}

void
COSXScreen::handlePowerChangeRequest(natural_t messageType, void* messageArg)
{
	// we've received a power change notification
	switch (messageType) {
	case kIOMessageSystemWillSleep:
		// COSXScreen has to handle this in the main thread so we have to
		// queue a confirm sleep event here.  we actually don't allow the
		// system to sleep until the event is handled.
		EVENTQUEUE->addEvent(CEvent(COSXScreen::getConfirmSleepEvent(),
								getEventTarget(), messageArg,
								CEvent::kDontFreeData));
		return;
			
	case kIOMessageSystemHasPoweredOn:
		LOG((CLOG_DEBUG "system wakeup"));
		EVENTQUEUE->addEvent(CEvent(IScreen::getResumeEvent(),
								getEventTarget()));
		break;

	default:
		break;
	}

	CLock lock(m_pmMutex);
	if (m_pmRootPort != 0) {
		IOAllowPowerChange(m_pmRootPort, (long)messageArg);
	}
}

CEvent::Type
COSXScreen::getConfirmSleepEvent()
{
	return CEvent::registerTypeOnce(s_confirmSleepEvent,
									"COSXScreen::confirmSleep");
}

void
COSXScreen::handleConfirmSleep(const CEvent& event, void*)
{
	long messageArg = (long)event.getData();
	if (messageArg != 0) {
		CLock lock(m_pmMutex);
		if (m_pmRootPort != 0) {
			// deliver suspend event immediately.
			EVENTQUEUE->addEvent(CEvent(IScreen::getSuspendEvent(),
									getEventTarget(), NULL, 
									CEvent::kDeliverImmediately));
	
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
COSXScreen::isGlobalHotKeyOperatingModeAvailable()
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
COSXScreen::setGlobalHotKeysEnabled(bool enabled)
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
COSXScreen::getGlobalHotKeysEnabled()
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
// COSXScreen::CHotKeyItem
//

COSXScreen::CHotKeyItem::CHotKeyItem(UInt32 keycode, UInt32 mask) :
	m_ref(NULL),
	m_keycode(keycode),
	m_mask(mask)
{
	// do nothing
}

COSXScreen::CHotKeyItem::CHotKeyItem(EventHotKeyRef ref,
				UInt32 keycode, UInt32 mask) :
	m_ref(ref),
	m_keycode(keycode),
	m_mask(mask)
{
	// do nothing
}

EventHotKeyRef
COSXScreen::CHotKeyItem::getRef() const
{
	return m_ref;
}

bool
COSXScreen::CHotKeyItem::operator<(const CHotKeyItem& x) const
{
	return (m_keycode < x.m_keycode ||
			(m_keycode == x.m_keycode && m_mask < x.m_mask));
}
