/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2004 Chris Schoeneman, Nick Bolton, Sorin Sbarnea
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

#ifndef CMSWINDOWSDESKS_H
#define CMSWINDOWSDESKS_H

#include "CSynergyHook.h"
#include "KeyTypes.h"
#include "MouseTypes.h"
#include "OptionTypes.h"
#include "CCondVar.h"
#include "CMutex.h"
#include "CString.h"
#include "stdmap.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

class CEvent;
class CEventQueueTimer;
class CThread;
class IJob;
class IScreenSaver;

//! Microsoft Windows desk handling
/*!
Desks in Microsoft Windows are only remotely like desktops on X11
systems.  A desk is another virtual surface for windows but desks
impose serious restrictions:  a thread can interact with only one
desk at a time, you can't switch desks if the thread has any hooks
installed or owns any windows, windows cannot exist on multiple
desks at once, etc.  Basically, they're useless except for running
the login window or the screensaver, which is what they're used
for.  Synergy must deal with them mainly because of the login
window and screensaver but users can create their own desks and
synergy should work on those too.

This class encapsulates all the desk nastiness.  Clients of this
object don't have to know anything about desks.
*/
class CMSWindowsDesks {
public:
	//! Constructor
	/*!
	\p isPrimary is true iff the desk is for a primary screen.
	\p screensaver points to a screensaver object and it's used
	only to check if the screensaver is active.  The \p updateKeys
	job is adopted and is called when the key state should be
	updated in a thread attached to the current desk.
	\p hookLibrary must be a handle to the hook library.
	*/
	CMSWindowsDesks(bool isPrimary, bool noHooks, HINSTANCE hookLibrary,
							const IScreenSaver* screensaver, IJob* updateKeys);
	~CMSWindowsDesks();

	//! @name manipulators
	//@{

	//! Enable desk tracking
	/*!
	Enables desk tracking.  While enabled, this object checks to see
	if the desk has changed and ensures that the hooks are installed
	on the new desk.  \c setShape should be called at least once
	before calling \c enable.
	*/
	void				enable();

	//! Disable desk tracking
	/*!
	Disables desk tracking.  \sa enable.
	*/
	void				disable();

	//! Notify of entering a desk
	/*!
	Prepares a desk for when the cursor enters it.
	*/
	void				enter();

	//! Notify of leaving a desk
	/*!
	Prepares a desk for when the cursor leaves it.
	*/
	void				leave(HKL keyLayout);

	//! Notify of options changes
	/*!
	Resets all options to their default values.
	*/
	void				resetOptions();

	//! Notify of options changes
	/*!
	Set options to given values.  Ignores unknown options and doesn't
	modify options that aren't given in \c options.
	*/
	void				setOptions(const COptionsList& options);

	//! Update the key state
	/*!
	Causes the key state to get updated to reflect the physical keyboard
	state and current keyboard mapping.
	*/
	void				updateKeys();

	//! Tell desk about new size
	/*!
	This tells the desks that the display size has changed.
	*/
	void				setShape(SInt32 x, SInt32 y,
							SInt32 width, SInt32 height,
							SInt32 xCenter, SInt32 yCenter, bool isMultimon);

	//! Install/uninstall screensaver hooks
	/*!
	If \p install is true then the screensaver hooks are installed and,
	if desk tracking is enabled, updated whenever the desk changes.  If
	\p install is false then the screensaver hooks are uninstalled.
	*/
	void				installScreensaverHooks(bool install);

	//! Start ignoring user input
	/*!
	Starts ignoring user input so we don't pick up our own synthesized events.
	*/
	void				fakeInputBegin();

	//! Stop ignoring user input
	/*!
	Undoes whatever \c fakeInputBegin() did.
	*/
	void				fakeInputEnd();

	//@}
	//! @name accessors
	//@{

	//! Get cursor position
	/*!
	Return the current position of the cursor in \c x and \c y.
	*/
	void				getCursorPos(SInt32& x, SInt32& y) const;

	//! Fake key press/release
	/*!
	Synthesize a press or release of key \c button.
	*/
	void				fakeKeyEvent(KeyButton button, UINT virtualKey,
							bool press, bool isAutoRepeat) const;

	//! Fake mouse press/release
	/*!
	Synthesize a press or release of mouse button \c id.
	*/
	void				fakeMouseButton(ButtonID id, bool press);

	//! Fake mouse move
	/*!
	Synthesize a mouse move to the absolute coordinates \c x,y.
	*/
	void				fakeMouseMove(SInt32 x, SInt32 y) const;

	//! Fake mouse move
	/*!
	Synthesize a mouse move to the relative coordinates \c dx,dy.
	*/
	void				fakeMouseRelativeMove(SInt32 dx, SInt32 dy) const;

	//! Fake mouse wheel
	/*!
	Synthesize a mouse wheel event of amount \c delta in direction \c axis.
	*/
	void				fakeMouseWheel(SInt32 xDelta, SInt32 yDelta) const;

	//@}

private:
	class CDesk {
	public:
		CString			m_name;
		CThread*		m_thread;
		DWORD			m_threadID;
		DWORD			m_targetID;
		HDESK			m_desk;
		HWND			m_window;
		HWND			m_foregroundWindow;
		bool			m_lowLevel;
	};
	typedef std::map<CString, CDesk*> CDesks;

	// initialization and shutdown operations
	void				queryHookLibrary(HINSTANCE hookLibrary);
	HCURSOR				createBlankCursor() const;
	void				destroyCursor(HCURSOR cursor) const;
	ATOM				createDeskWindowClass(bool isPrimary) const;
	void				destroyClass(ATOM windowClass) const;
	HWND				createWindow(ATOM windowClass, const char* name) const;
	void				destroyWindow(HWND) const;

	// message handlers
	void				deskMouseMove(SInt32 x, SInt32 y) const;
	void				deskMouseRelativeMove(SInt32 dx, SInt32 dy) const;
	void				deskEnter(CDesk* desk);
	void				deskLeave(CDesk* desk, HKL keyLayout);
	void				deskThread(void* vdesk);
	void				xinputThread();

	// desk switch checking and handling
	CDesk*				addDesk(const CString& name, HDESK hdesk);
	void				removeDesks();
	void				checkDesk();
	bool				isDeskAccessible(const CDesk* desk) const;
	void				handleCheckDesk(const CEvent& event, void*);

	// communication with desk threads
	void				waitForDesk() const;
	void				sendMessage(UINT, WPARAM, LPARAM) const;

	// work around for messed up keyboard events from low-level hooks
	HWND				getForegroundWindow() const;

	// desk API wrappers
	HDESK				openInputDesktop();
	void				closeDesktop(HDESK);
	CString				getDesktopName(HDESK);

	// our desk window procs
	static LRESULT CALLBACK primaryDeskProc(HWND, UINT, WPARAM, LPARAM);
	static LRESULT CALLBACK secondaryDeskProc(HWND, UINT, WPARAM, LPARAM);

private:
	// true if screen is being used as a primary screen, false otherwise
	bool				m_isPrimary;

	// true if hooks are not to be installed (useful for debugging)
	bool				m_noHooks;

	// true if windows 95/98/me
	bool				m_is95Family;

	// true if windows 98/2k or higher (i.e. not 95/nt)
	bool				m_isModernFamily;

	// true if mouse has entered the screen
	bool				m_isOnScreen;

	// our resources
	ATOM				m_deskClass;
	HCURSOR				m_cursor;

	// screen shape stuff
	SInt32				m_x, m_y;
	SInt32				m_w, m_h;
	SInt32				m_xCenter, m_yCenter;

	// true if system appears to have multiple monitors
	bool				m_multimon;

	// the timer used to check for desktop switching
	CEventQueueTimer*	m_timer;

	// screen saver stuff
	DWORD				m_threadID;
	const IScreenSaver*	m_screensaver;
	bool				m_screensaverNotify;

	// the current desk and it's name
	CDesk*				m_activeDesk;
	CString				m_activeDeskName;

	// one desk per desktop and a cond var to communicate with it
	CMutex				m_mutex;
	CCondVar<bool>		m_deskReady;
	CDesks				m_desks;

	// hook library stuff
	InstallFunc			m_install;
	UninstallFunc		m_uninstall;
	InstallScreenSaverFunc		m_installScreensaver;
	UninstallScreenSaverFunc	m_uninstallScreensaver;

	// keyboard stuff
	IJob*				m_updateKeys;
	HKL					m_keyLayout;

	// options
	bool				m_leaveForegroundOption;
};

#endif
