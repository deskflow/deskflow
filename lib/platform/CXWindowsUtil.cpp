/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2002 Chris Schoeneman
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

#include "CXWindowsUtil.h"
#include "CThread.h"
#include "CLog.h"
#include <X11/Xatom.h>

//
// CXWindowsUtil
//

bool
CXWindowsUtil::getWindowProperty(Display* display, Window window,
				Atom property, CString* data, Atom* type,
				SInt32* format, bool deleteProperty)
{
	assert(display != NULL);

	Atom actualType;
	int actualDatumSize;

	// ignore errors.  XGetWindowProperty() will report failure.
	CXWindowsUtil::CErrorLock lock(display);

	// read the property
	bool okay = true;
	const long length = XMaxRequestSize(display);
	long offset = 0;
	unsigned long bytesLeft = 1;
	while (bytesLeft != 0) {
		// get more data
		unsigned long numItems;
		unsigned char* rawData;
		if (XGetWindowProperty(display, window, property,
								offset, length, False, AnyPropertyType,
								&actualType, &actualDatumSize,
								&numItems, &bytesLeft, &rawData) != Success ||
			actualType == None || actualDatumSize == 0) {
			// failed
			okay = false;
			break;
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
		if (data != NULL) {
			data->append((char*)rawData, numBytes);
		}
		else {
			// data is not required so don't try to get any more
			bytesLeft = 0;
		}

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

	if (okay) {
		LOG((CLOG_DEBUG2 "read property %d on window 0x%08x: bytes=%d", property, window, (data == NULL) ? 0 : data->size()));
		return true;
	}
	else {
		LOG((CLOG_DEBUG2 "can't read property %d on window 0x%08x", property, window));
		return false;
	}
}

bool
CXWindowsUtil::setWindowProperty(Display* display, Window window,
				Atom property, const void* vdata, UInt32 size,
				Atom type, SInt32 format)
{
	const UInt32 length       = 4 * XMaxRequestSize(display);
	const unsigned char* data = reinterpret_cast<const unsigned char*>(vdata);
	const UInt32 datumSize    = static_cast<UInt32>(format / 8);

	// save errors
	bool error = false;
	CXWindowsUtil::CErrorLock lock(display, &error);

	// how much data to send in first chunk?
	UInt32 chunkSize = size;
	if (chunkSize > length) {
		chunkSize = length;
	}

	// send first chunk
	XChangeProperty(display, window, property,
								type, format, PropModeReplace,
								data, chunkSize / datumSize);

	// append remaining chunks
	data += chunkSize;
	size -= chunkSize;
	while (!error && size > 0) {
		chunkSize = size;
		if (chunkSize > length) {
			chunkSize = length;
		}
		XChangeProperty(display, window, property,
								type, format, PropModeAppend,
								data, chunkSize / datumSize);
		data += chunkSize;
		size -= chunkSize;
	}

	return !error;
}

Time
CXWindowsUtil::getCurrentTime(Display* display, Window window)
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
	XIfEvent(display, &xevent, &CXWindowsUtil::propertyNotifyPredicate,
								(XPointer)&filter);
	assert(xevent.type             == PropertyNotify);
	assert(xevent.xproperty.window == window);
	assert(xevent.xproperty.atom   == atom);

	// restore event mask
	XSelectInput(display, window, attr.your_event_mask);

	return xevent.xproperty.time;
}

Bool
CXWindowsUtil::propertyNotifyPredicate(Display*, XEvent* xevent, XPointer arg)
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

CXWindowsUtil::CErrorLock::CErrorLock(Display* display) :
	m_display(display)
{
	install(&CXWindowsUtil::CErrorLock::ignoreHandler, NULL);
}

CXWindowsUtil::CErrorLock::CErrorLock(Display* display, bool* flag) :
	m_display(display)
{
	install(&CXWindowsUtil::CErrorLock::saveHandler, flag);
}

CXWindowsUtil::CErrorLock::CErrorLock(Display* display,
				ErrorHandler handler, void* data) :
	m_display(display)
{
	install(handler, data);
}

CXWindowsUtil::CErrorLock::~CErrorLock()
{
	// make sure everything finishes before uninstalling handler
	if (m_display != NULL) {
		XSync(m_display, False);
	}

	// restore old handler
	XSetErrorHandler(m_oldXHandler);
	s_top = m_next;
}

void
CXWindowsUtil::CErrorLock::install(ErrorHandler handler, void* data)
{
	// make sure everything finishes before installing handler
	if (m_display != NULL) {
		XSync(m_display, False);
	}

	// install handler
	m_handler     = handler;
	m_userData    = data;
	m_oldXHandler = XSetErrorHandler(
								&CXWindowsUtil::CErrorLock::internalHandler);
	m_next        = s_top;
	s_top         = this;
}

int
CXWindowsUtil::CErrorLock::internalHandler(Display* display, XErrorEvent* event)
{
	if (s_top != NULL && s_top->m_handler != NULL) {
		s_top->m_handler(display, event, s_top->m_userData);
	}
	return 0;
}

void
CXWindowsUtil::CErrorLock::ignoreHandler(Display*, XErrorEvent* e, void*)
{
	LOG((CLOG_DEBUG1 "ignoring X error: %d", e->error_code));
}

void
CXWindowsUtil::CErrorLock::saveHandler(Display*, XErrorEvent* e, void* flag)
{
	LOG((CLOG_DEBUG1 "flagging X error: %d", e->error_code));
	*reinterpret_cast<bool*>(flag) = true;
}
