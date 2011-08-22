/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2003 Chris Schoeneman, Nick Bolton, Sorin Sbarnea
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef IPRIMARYSCREEN_H
#define IPRIMARYSCREEN_H

#include "IInterface.h"
#include "KeyTypes.h"
#include "MouseTypes.h"
#include "CEvent.h"
#include "GameDeviceTypes.h"

//! Primary screen interface
/*!
This interface defines the methods common to all platform dependent
primary screen implementations.
*/
class IPrimaryScreen : public IInterface {
public:
	//! Button event data
	class CButtonInfo {
	public:
		static CButtonInfo* alloc(ButtonID, KeyModifierMask);
		static CButtonInfo* alloc(const CButtonInfo&);

		static bool			equal(const CButtonInfo*, const CButtonInfo*);

	public:
		ButtonID		m_button;
		KeyModifierMask	m_mask;
	};
	//! Motion event data
	class CMotionInfo {
	public:
		static CMotionInfo* alloc(SInt32 x, SInt32 y);

	public:
		SInt32			m_x;
		SInt32			m_y;
	};
	//! Wheel motion event data
	class CWheelInfo {
	public:
		static CWheelInfo* alloc(SInt32 xDelta, SInt32 yDelta);

	public:
		SInt32			m_xDelta;
		SInt32			m_yDelta;
	};
	//! Hot key event data
	class CHotKeyInfo {
	public:
		static CHotKeyInfo* alloc(UInt32 id);

	public:
		UInt32			m_id;
	};
	//! Game device button event data
	class CGameDeviceButtonInfo {
	public:
		CGameDeviceButtonInfo(GameDeviceID id, GameDeviceButton buttons) :
			m_id(id), m_buttons(buttons) { }
	public:
		GameDeviceID m_id;
		GameDeviceButton m_buttons;
	};
	//! Game device sticks event data
	class CGameDeviceStickInfo {
	public:
		CGameDeviceStickInfo(GameDeviceID id, SInt16 x1, SInt16 y1, SInt16 x2, SInt16 y2) :
			m_id(id), m_x1(x1), m_x2(x2), m_y1(y1), m_y2(y2) { }
	public:
		GameDeviceID m_id;
		SInt16 m_x1;
		SInt16 m_x2;
		SInt16 m_y1;
		SInt16 m_y2;
	};
	//! Game device triggers event data
	class CGameDeviceTriggerInfo {
	public:
		CGameDeviceTriggerInfo(GameDeviceID id, UInt8 t1, UInt8 t2) :
		  m_id(id), m_t1(t1), m_t2(t2) { }
	public:
		GameDeviceID m_id;
		UInt8 m_t1;
		UInt8 m_t2;
	};
	//! Game device timing response event data
	class CGameDeviceTimingRespInfo {
	public:
		CGameDeviceTimingRespInfo(UInt16 freq) :
		  m_freq(freq) { }
	public:
		UInt16 m_freq;
	};
	//! Game device feedback event data
	class CGameDeviceFeedbackInfo {
	public:
		CGameDeviceFeedbackInfo(GameDeviceID id, UInt16 m1, UInt16 m2) :
		  m_id(id), m_m1(m1), m_m2(m2) { }
	public:
		GameDeviceID m_id;
		UInt16 m_m1;
		UInt16 m_m2;
	};

	//! @name manipulators
	//@{

	//! Update configuration
	/*!
	This is called when the configuration has changed.  \c activeSides
	is a bitmask of EDirectionMask indicating which sides of the
	primary screen are linked to clients.  Override to handle the
	possible change in jump zones.
	*/
	virtual void		reconfigure(UInt32 activeSides) = 0;

	//! Warp cursor
	/*!
	Warp the cursor to the absolute coordinates \c x,y.  Also
	discard input events up to and including the warp before
	returning.
	*/
	virtual void		warpCursor(SInt32 x, SInt32 y) = 0;

	//! Register a system hotkey
	/*!
	Registers a system-wide hotkey.  The screen should arrange for an event
	to be delivered to itself when the hot key is pressed or released.  When
	that happens the screen should post a \c getHotKeyDownEvent() or
	\c getHotKeyUpEvent(), respectively.  The hot key is key \p key with
	exactly the modifiers \p mask.  Returns 0 on failure otherwise an id
	that can be used to unregister the hotkey.

	A hot key is a set of modifiers and a key, which may itself be a modifier.
	The hot key is pressed when the hot key's modifiers and only those
	modifiers are logically down (active) and the key is pressed.  The hot
	key is released when the key is released, regardless of the modifiers.

	The hot key event should be generated no matter what window or application
	has the focus.  No other window or application should receive the key
	press or release events (they can and should see the modifier key events).
	When the key is a modifier, it's acceptable to allow the user to press
	the modifiers in any order or to require the user to press the given key
	last.
	*/
	virtual UInt32		registerHotKey(KeyID key, KeyModifierMask mask) = 0;

	//! Unregister a system hotkey
	/*!
	Unregisters a previously registered hot key.
	*/
	virtual void		unregisterHotKey(UInt32 id) = 0;

	//! Prepare to synthesize input on primary screen
	/*!
	Prepares the primary screen to receive synthesized input.  We do not
	want to receive this synthesized input as user input so this method
	ensures that we ignore it.  Calls to \c fakeInputBegin() may not be
	nested.
	*/
	virtual void		fakeInputBegin() = 0;

	//! Done synthesizing input on primary screen
	/*!
	Undoes whatever \c fakeInputBegin() did.
	*/
	virtual void		fakeInputEnd() = 0;

	//@}
	//! @name accessors
	//@{

	//! Get jump zone size
	/*!
	Return the jump zone size, the size of the regions on the edges of
	the screen that cause the cursor to jump to another screen.
	*/
	virtual SInt32		getJumpZoneSize() const = 0;

	//! Test if mouse is pressed
	/*!
	Return true if any mouse button is currently pressed.  Ideally,
	"current" means up to the last processed event but it can mean
	the current physical mouse button state.
	*/
	virtual bool		isAnyMouseButtonDown() const = 0;

	//! Get cursor center position
	/*!
	Return the cursor center position which is where we park the
	cursor to compute cursor motion deltas and should be far from
	the edges of the screen, typically the center.
	*/
	virtual void		getCursorCenter(SInt32& x, SInt32& y) const = 0;
	
	//! Handle incoming game device timing responses.
	virtual void		gameDeviceTimingResp(UInt16 freq) = 0;

	//! Handle incoming game device feedback changes.
	virtual void		gameDeviceFeedback(GameDeviceID id, UInt16 m1, UInt16 m2) = 0;

	//! Get button down event type.  Event data is CButtonInfo*.
	static CEvent::Type	getButtonDownEvent();
	//! Get button up event type.  Event data is CButtonInfo*.
	static CEvent::Type	getButtonUpEvent();
	//! Get mouse motion on the primary screen event type
	/*!
	Event data is CMotionInfo* and the values are an absolute position.
	*/
	static CEvent::Type	getMotionOnPrimaryEvent();
	//! Get mouse motion on a secondary screen event type
	/*!
	Event data is CMotionInfo* and the values are motion deltas not
	absolute coordinates.
	*/
	static CEvent::Type	getMotionOnSecondaryEvent();
	//! Get mouse wheel event type.  Event data is CWheelInfo*.
	static CEvent::Type	getWheelEvent();
	//! Get screensaver activated event type
	static CEvent::Type	getScreensaverActivatedEvent();
	//! Get screensaver deactivated event type
	static CEvent::Type	getScreensaverDeactivatedEvent();
	//! Get hot key down event type.  Event data is CHotKeyInfo*.
	static CEvent::Type	getHotKeyDownEvent();
	//! Get hot key up event type.  Event data is CHotKeyInfo*.
	static CEvent::Type	getHotKeyUpEvent();
	//! Get start of fake input event type
	static CEvent::Type	getFakeInputBeginEvent();
	//! Get end of fake input event type
	static CEvent::Type	getFakeInputEndEvent();
	//! Get game device buttons event type.
	static CEvent::Type	getGameDeviceButtonsEvent();
	//! Get game device sticks event type.
	static CEvent::Type	getGameDeviceSticksEvent();
	//! Get game device triggers event type.
	static CEvent::Type	getGameDeviceTriggersEvent();
	//! Get game device timing request event type.
	static CEvent::Type	getGameDeviceTimingReqEvent();

	//@}

private:
	static CEvent::Type	s_buttonDownEvent;
	static CEvent::Type	s_buttonUpEvent;
	static CEvent::Type	s_motionPrimaryEvent;
	static CEvent::Type	s_motionSecondaryEvent;
	static CEvent::Type	s_wheelEvent;
	static CEvent::Type	s_ssActivatedEvent;
	static CEvent::Type	s_ssDeactivatedEvent;
	static CEvent::Type	s_hotKeyDownEvent;
	static CEvent::Type	s_hotKeyUpEvent;
	static CEvent::Type	s_fakeInputBegin;
	static CEvent::Type	s_fakeInputEnd;
	static CEvent::Type s_gameButtonsEvent;
	static CEvent::Type s_gameSticksEvent;
	static CEvent::Type s_gameTriggersEvent;
	static CEvent::Type s_gameTimingReqEvent;
};

#endif
