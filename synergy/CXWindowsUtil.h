#ifndef CXWINDOWSUTIL_H
#define CXWINDOWSUTIL_H

#include "BasicTypes.h"
#include "CString.h"
#include <X11/Xlib.h>

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
