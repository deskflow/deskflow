#ifndef CXWINDOWSSCREEN_H
#define CXWINDOWSSCREEN_H

#include "ClipboardTypes.h"
#include "CMutex.h"
#include "CStopwatch.h"
#include "stdvector.h"
#if defined(X_DISPLAY_MISSING)
#	error X11 is required to build synergy
#else
#	include <X11/Xlib.h>
#endif
#include <algorithm>
#include <functional>

class IClipboard;
class IJob;
class IScreenSaver;
class CXWindowsClipboard;
class CXWindowsScreenSaver;

class CEvent {
public:
	XEvent				m_event;
	SInt32				m_result;
};

class CXWindowsScreen {
public:
	CXWindowsScreen();
	virtual ~CXWindowsScreen();

	// manipulators

	// add/remove a job to invoke every timeout seconds.  the job is
	// called with the display locked.  if a job timeout expires twice
	// or more before the job can be called then the job is called
	// just once.  the caller retains ownership of the job.
	void				addTimer(IJob*, double timeout);
	void				removeTimer(IJob*);

protected:
	class CDisplayLock {
	public:
		CDisplayLock(const CXWindowsScreen*);
		~CDisplayLock();

		operator Display*() const;

	private:
		const CMutex*	m_mutex;
		Display*		m_display;
	};
	friend class CDisplayLock;

	// runs an event loop and returns when exitMainLoop() is called
	void				mainLoop();

	// force mainLoop() to return
	void				exitMainLoop();

	// open the X display.  calls onOpenDisplay() after opening the display,
	// getting the screen, its size, and root window.  then it starts the
	// event thread.
	void				openDisplay();

	// destroy the window and close the display.  calls onCloseDisplay()
	// after the event thread has been shut down but before the display
	// is closed.
	void				closeDisplay();

	// get the Display*.  only use this when you know the display is
	// locked but don't have the CDisplayLock available.
	Display*			getDisplay() const;

	// get the opened screen and its root window.  to get the display
	// create a CDisplayLock object passing this.  while the object
	// exists no other threads may access the display.  do not save
	// the Display* beyond the lifetime of the CDisplayLock.
	int					getScreen() const;
	Window				getRoot() const;

	// initialize the clipboards
	void				initClipboards(Window);

	// update screen size cache
	void				updateScreenShape();

	// get the shape of the screen
	void				getScreenShape(SInt32& x, SInt32& y,
							SInt32& width, SInt32& height) const;

	// get the current cursor position
	void				getCursorPos(SInt32& x, SInt32& y) const;

	// get the cursor center position
	void				getCursorCenter(SInt32& x, SInt32& y) const;

	// get a cursor that is transparent everywhere
	Cursor				getBlankCursor() const;

	// set the contents of the clipboard (i.e. primary selection)
	bool				setDisplayClipboard(ClipboardID,
							const IClipboard* clipboard);

	// copy the clipboard contents to clipboard
	bool				getDisplayClipboard(ClipboardID,
							IClipboard* clipboard) const;

	// get the screen saver object
	CXWindowsScreenSaver*
						getScreenSaver() const;

	// called for each event before event translation and dispatch.  return
	// true to skip translation and dispatch.  subclasses should call the
	// superclass's version first and return true if it returns true.
	virtual bool		onPreDispatch(const CEvent* event) = 0;

	// called by mainLoop().  iff the event was handled return true and
	// store the result, if any, in m_result, which defaults to zero.
	virtual bool		onEvent(CEvent* event) = 0;

	// called if the display is unexpectedly closing.  default does nothing.
	virtual void		onUnexpectedClose();

	// called when a clipboard is lost
	virtual void		onLostClipboard(ClipboardID) = 0;

private:
	// create the transparent cursor
	void				createBlankCursor();

	// remove a timer without locking
	void				removeTimerNoLock(IJob*);

	// process timers
	bool				processTimers();

	// determine the clipboard from the X selection.  returns
	// kClipboardEnd if no such clipboard.
	ClipboardID			getClipboardID(Atom selection) const;

	// continue processing a selection request
	void				processClipboardRequest(Window window,
							Time time, Atom property);

	// terminate a selection request
	void				destroyClipboardRequest(Window window);

	// X I/O error handler
	static int			ioErrorHandler(Display*);

private:
	// a priority queue will direct access to the elements
	template <class T, class Container = std::vector<T>,
				class Compare = std::greater<typename Container::value_type> >
	class CPriorityQueue {
	public:
		typedef typename Container::value_type value_type;
		typedef typename Container::size_type size_type;
		typedef typename Container::iterator iterator;
		typedef Container container_type;

		CPriorityQueue() { }
		CPriorityQueue(Container& swappedIn);
		~CPriorityQueue() { }

		// manipulators

		void			push(const value_type& v)
		{
			c.push_back(v);
			std::push_heap(c.begin(), c.end(), comp);
		}

		void			pop()
		{
			std::pop_heap(c.begin(), c.end(), comp);
			c.pop_back();
		}

		iterator		begin()
		{
			return c.begin();
		}

		iterator		end()
		{
			return c.end();
		}

		void			swap(CPriorityQueue<T, Container, Compare>& q)
		{
			c.swap(q.c);
		}

		void			swap(Container& c2)
		{
			c.swap(c2);
			std::make_heap(c.begin(), c.end(), comp);
		}

		// accessors

		bool			empty() const
		{
			return c.empty();
		}

		size_type		size() const
		{
			return c.size();
		}

		const value_type&
						top() const
		{
			return c.front();
		}

	private:
		Container		c;
		Compare			comp;
	};

	// a timer priority queue element
	class CTimer {
	public:
		CTimer(IJob* job, double timeout);
		~CTimer();

		// manipulators

		void			run();

		void			reset();

		CTimer&			operator-=(double);

		// accessors

		IJob*			getJob() const
		{
			return m_job;
		}

		operator double() const;

		bool			operator<(const CTimer&) const;

	private:
		IJob*			m_job;
		double			m_timeout;
		double			m_time;
	};

private:
	typedef CPriorityQueue<CTimer> CTimerPriorityQueue;

	Display*			m_display;
	int					m_screen;
	Window				m_root;
	SInt32				m_x, m_y;
	SInt32				m_w, m_h;
	bool				m_stop;

	// clipboards
	CXWindowsClipboard*	m_clipboard[kClipboardEnd];

	// the transparent cursor
	Cursor				m_cursor;

	// screen saver
	CXWindowsScreenSaver*	m_screenSaver;

	// timers, the stopwatch used to time, and a mutex for the timers
	CTimerPriorityQueue	m_timers;
	CStopwatch			m_time;
	CMutex				m_timersMutex;

	// X is not thread safe
	CMutex				m_mutex;

	// pointer to (singleton) screen.  this is only needed by
	// ioErrorHandler().
	static CXWindowsScreen*	s_screen;
};

#endif
