#ifndef CXWINDOWSUTIL_H
#define CXWINDOWSUTIL_H

#include "CString.h"
#include "BasicTypes.h"
#if defined(X_DISPLAY_MISSING)
#	error X11 is required to build synergy
#else
#	include <X11/Xlib.h>
#endif

class CXWindowsUtil {
public:
	static bool			getWindowProperty(Display*,
							Window window, Atom property,
							CString* data, Atom* type,
							SInt32* format, bool deleteProperty);
	static bool			setWindowProperty(Display*,
							Window window, Atom property,
							const void* data, UInt32 size,
							Atom type, SInt32 format);
	static Time			getCurrentTime(Display*, Window);

	// class to set an X error handler in the c'tor and restore the
	// previous error handler in the d'tor.  a lock should only
	// be installed while the display is locked by the thread.
	//
	// CErrorLock() ignores errors
	// CErrorLock(bool* flag) sets *flag to true if any error occurs
	class CErrorLock {
	public:
		typedef void (*ErrorHandler)(Display*, XErrorEvent*, void* userData);
		CErrorLock();
		CErrorLock(bool* errorFlag);
		CErrorLock(ErrorHandler, void* userData);
		~CErrorLock();

	private:
		void			install(ErrorHandler, void*);
		static int		internalHandler(Display*, XErrorEvent*);
		static void		ignoreHandler(Display*, XErrorEvent*, void*);
		static void		saveHandler(Display*, XErrorEvent*, void*);

	private:
		typedef int (*XErrorHandler)(Display*, XErrorEvent*);

		ErrorHandler	m_handler;
		void*			m_userData;
		XErrorHandler	m_oldXHandler;
		CErrorLock*		m_next;
		static CErrorLock*	s_top;
	};

private:
	class CPropertyNotifyPredicateInfo {
	public:
		Window			m_window;
		Atom			m_property;
	};

	static Bool			propertyNotifyPredicate(Display*,
							XEvent* xevent, XPointer arg);
};

#endif
