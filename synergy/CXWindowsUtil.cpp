#include "CXWindowsUtil.h"
#include "CLog.h"
#include "CThread.h"
#include <assert.h>
#include <X11/Xatom.h>

//
// CXWindowsUtil
//

bool					CXWindowsUtil::getWindowProperty(
								Display* display,
								Window window, Atom property,
								CString* data, Atom* type,
								int* format, bool deleteProperty)
{
	assert(display != NULL);
	assert(data != NULL);

	Atom actualType;
	int actualDatumSize;

	// ignore errors.  XGetWindowProperty() will report failure.
	CXWindowsUtil::CErrorLock lock;

	// read the property
	const long length = XMaxRequestSize(display);
	long offset = 0;
	unsigned long bytesLeft = 1;
	while (bytesLeft != 0) {
		// get more data
		unsigned long numItems;
		unsigned char* rawData;
		const int result = XGetWindowProperty(display, window, property,
								offset, length, False, AnyPropertyType,
								&actualType, &actualDatumSize,
								&numItems, &bytesLeft, &rawData);
		if (result != Success || actualType == None || actualDatumSize == 0) {
			// failed
			return false;
		}

		// compute bytes read and advance offset
		unsigned long numBytes;
		switch (actualDatumSize) {
		case 8:
		default:
			numBytes = numItems;
			offset  += numItems / 4;
			break;

		case 16:
			numBytes = 2 * numItems;
			offset  += numItems / 2;
			break;

		case 32:
			numBytes = 4 * numItems;
			offset  += numItems;
			break;
		}

		// append data
		data->append((char*)rawData, numBytes);

		// done with returned data
		XFree(rawData);
	}

	// delete the property if requested
	if (deleteProperty) {
		XDeleteProperty(display, window, property);
	}

	// save property info
	if (type != NULL) {
		*type = actualType;
	}
	if (format != NULL) {
		*format = static_cast<SInt32>(actualDatumSize);
	}

	log((CLOG_DEBUG1 "read property %d on window 0x%08x: bytes=%d", property, window, data->size()));
	return true;
}

bool					CXWindowsUtil::setWindowProperty(
								Display* display,
								Window window, Atom property,
								const void* vdata, UInt32 size,
								Atom type, SInt32 format)
{
	const UInt32 length       = 4 * XMaxRequestSize(display);
	const unsigned char* data = reinterpret_cast<const unsigned char*>(vdata);
	const UInt32 datumSize    = static_cast<UInt32>(format / 8);

	// save errors
	bool error = false;
	CXWindowsUtil::CErrorLock lock(&error);

	// how much data to send in first chunk?
	UInt32 chunkSize = size;
	if (chunkSize > length)
		chunkSize = length;

	// send first chunk
	XChangeProperty(display, window, property,
								type, format, PropModeReplace,
								data, chunkSize / datumSize);

	// append remaining chunks
	data += chunkSize;
	size -= chunkSize;
	while (!error && size > 0) {
		chunkSize = size;
		if (chunkSize > length)
			chunkSize = length;
		XChangeProperty(display, window, property,
								type, format, PropModeAppend,
								data, chunkSize / datumSize);
		data += chunkSize;
		size -= chunkSize;
	}

	return !error;
}

Time					CXWindowsUtil::getCurrentTime(
								Display* display, Window window)
{
	// select property events on window
	XWindowAttributes attr;
	XGetWindowAttributes(display, window, &attr);
	XSelectInput(display, window, attr.your_event_mask | PropertyChangeMask);

	// make a property name to receive dummy change
	Atom atom = XInternAtom(display, "TIMESTAMP", False);

	// do a zero-length append to get the current time
	unsigned char dummy;
	XChangeProperty(display, window, atom,
								XA_INTEGER, 8,
								PropModeAppend,
								&dummy, 0);

	// look for property notify events with the following
	CPropertyNotifyPredicateInfo filter;
	filter.m_window   = window;
	filter.m_property = atom;

	// wait for reply
	XEvent xevent;
	while (XCheckIfEvent(display, &xevent,
								&CXWindowsUtil::propertyNotifyPredicate,
								(XPointer)&filter) != True) {
		// wait a bit
		CThread::sleep(0.05);
	}
	assert(xevent.type             == PropertyNotify);
	assert(xevent.xproperty.window == window);
	assert(xevent.xproperty.atom   == atom);

	// restore event mask
	XSelectInput(display, window, attr.your_event_mask);

	return xevent.xproperty.time;
}

Bool					CXWindowsUtil::propertyNotifyPredicate(
								Display*, XEvent* xevent, XPointer arg)
{
	CPropertyNotifyPredicateInfo* filter =
						reinterpret_cast<CPropertyNotifyPredicateInfo*>(arg);
	return (xevent->type             == PropertyNotify &&
			xevent->xproperty.window == filter->m_window &&
			xevent->xproperty.atom   == filter->m_property &&
			xevent->xproperty.state  == PropertyNewValue) ? True : False;
}


//
// CXWindowsUtil::CErrorLock
//

CXWindowsUtil::CErrorLock*	CXWindowsUtil::CErrorLock::s_top = NULL;

CXWindowsUtil::CErrorLock::CErrorLock()
{
	install(&CXWindowsUtil::CErrorLock::ignoreHandler, NULL);
}

CXWindowsUtil::CErrorLock::CErrorLock(bool* flag)
{
	install(&CXWindowsUtil::CErrorLock::saveHandler, flag);
}

CXWindowsUtil::CErrorLock::CErrorLock(ErrorHandler handler, void* data)
{
	install(handler, data);
}

CXWindowsUtil::CErrorLock::~CErrorLock()
{
	XSetErrorHandler(m_oldXHandler);
	s_top = m_next;
}

void					CXWindowsUtil::CErrorLock::install(
								ErrorHandler handler, void* data)
{
	m_handler     = handler;
	m_userData    = data;
	m_oldXHandler = XSetErrorHandler(
								&CXWindowsUtil::CErrorLock::internalHandler);
	m_next        = s_top;
	s_top         = this;
}

int						CXWindowsUtil::CErrorLock::internalHandler(
								Display* display, XErrorEvent* event)
{
	if (s_top != NULL && s_top->m_handler != NULL) {
		s_top->m_handler(display, event, s_top->m_userData);
	}
	return 0;
}

void					CXWindowsUtil::CErrorLock::ignoreHandler(
								Display*, XErrorEvent*, void*)
{
	// do nothing
}

void					CXWindowsUtil::CErrorLock::saveHandler(
								Display*, XErrorEvent*, void* flag)
{
	*reinterpret_cast<bool*>(flag) = true;
}
