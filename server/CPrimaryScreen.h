#ifndef CPRIMARYSCREEN_H
#define CPRIMARYSCREEN_H

#include "ClipboardTypes.h"
#include "KeyTypes.h"
#include "CMutex.h"

class IClipboard;
class IScreen;
class IScreenReceiver;

// platform independent base class for primary screen implementations.
// each platform will derive a class from CPrimaryScreen to handle
// platform dependent operations.
class CPrimaryScreen {
public:
	CPrimaryScreen(IScreenReceiver*);
	virtual ~CPrimaryScreen();

	// manipulators

	// enter the screen's message loop.  this returns when it detects
	// the application should terminate or when stop() is called.
	// run() may only be called between open() and close().
	void				run();

	// cause run() to return
	void				stop();

	// initializes the screen and starts reporting events
	void				open();

	// close the screen
	void				close();

	// called when the user navigates to the primary screen.
	// forScreensaver == true means that we're entering the primary
	// screen because the screensaver has activated.
	void				enter(SInt32 x, SInt32 y, bool forScreensaver);

	// called when the user navigates off the primary screen.  returns
	// true iff successful.
	bool				leave();

	// called when the configuration has changed.  activeSides is a
	// bitmask of EDirectionMask indicating which sides of the
	// primary screen are linked to clients.
	virtual void		reconfigure(UInt32 activeSides) = 0;

	// warp the cursor to the given absolute coordinates
	virtual void		warpCursor(SInt32 x, SInt32 y) = 0;

	// set the screen's clipboard contents.  this is usually called
	// soon after an enter().
	void				setClipboard(ClipboardID, const IClipboard*);

	// synergy should own the clipboard
	void				grabClipboard(ClipboardID);

	// accessors

	// returns true iff the screen is active (i.e. the user has left
	// the screen)
	bool				isActive() const;

	// return the contents of the given clipboard
	void				getClipboard(ClipboardID, IClipboard*) const;

	// returns the size of the zone on the edges of the screen that
	// causes the cursor to jump to another screen.
	virtual SInt32		getJumpZoneSize() const = 0;

	// get the primary screen's current toggle modifier key state.
	// the returned mask should have the corresponding bit set for
	// each toggle key that is active.
	virtual KeyModifierMask	getToggleMask() const = 0;

	// return true if any key or button is being pressed or if there's
	// any other reason that the user should not be allowed to switch
	// screens.
	virtual bool		isLockedToScreen() const = 0;

	// get the platform dependent screen object
	virtual IScreen*	getScreen() const = 0;

protected:
	// template method hooks.  these are called on entry/exit to the
	// named method.  onEnterScreensaver() is called by enter() iff
	// forScreensaver is true.  onPostLeave() is passed the result of
	// showWindow().  override to do platform specific operations.
	// defaults do nothing.
	virtual void		onPreRun();
	virtual void		onPostRun();
	virtual void		onPreOpen();
	virtual void		onPostOpen();
	virtual void		onPreClose();
	virtual void		onPostClose();
	virtual void		onPreEnter();
	virtual void		onPostEnter();
	virtual void		onEnterScreensaver();
	virtual void		onPreLeave();
	virtual void		onPostLeave(bool success);

	// create/destroy the window.  this window is generally used to
	// receive events and, when the user navigates to another screen,
	// to capture keyboard and mouse input.
	virtual void		createWindow() = 0;
	virtual void		destroyWindow() = 0;

	// called when the user navigates off the primary screen.  hide the
	// cursor and grab exclusive access to the input devices.  returns
	// true iff successful.  every call to showWindow() has a matching
	// call to hideWindow() which preceeds it.  return true iff
	// successful (in particular, iff the input devices were grabbed).
	//
	// after a successful showWindow(), user input events and
	// screensaver activation/deactivation should be reported to an
	// IPrimaryScreenReceiver until hideWindow() is called.  report
	// mouse motion to IPrimaryScreenReceiver::onMouseMoveSecondary().
	// user input should not be delivered to any application except
	// synergy.
	virtual bool		showWindow() = 0;

	// called when the user navigates back to the primary screen.  show
	// the cursor and ungab the input devices.
	//
	// after hideWindow(), user input events should be delivered normally.
	// mouse motion over (at least) the jump zones must be reported to
	// an IPrimaryScreenReceiver::onMouseMovePrimary().
	virtual void		hideWindow() = 0;

	// prepare the cursor to report relative motion.  when the user has
	// navigated to another screen, synergy requires the cursor motion
	// deltas, not the absolute coordinates.  typically this is done by
	// warping the cursor to the center of the primary screen and then
	// every time it moves compute the motion and warp back to the
	// center (but without reporting that warp as motion).  this is
	// only called after a successful showWindow().
	virtual void		warpCursorToCenter() = 0;

	// check the current keyboard state.  normally a screen will save
	// the keyboard state in this method and use this shadow state
	// when handling user input and in methods like isLockedToScreen().
	virtual void		updateKeys() = 0;

private:
	void				enterNoWarp();

private:
	CMutex				m_mutex;

	// object to notify of changes
	IScreenReceiver*	m_receiver;

	// m_active is true if this screen has been left
	bool				m_active;
};

#endif
