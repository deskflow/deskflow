#include "CXWindowsScreen.h"
#include "CThread.h"
#include "CLock.h"
#include "TMethodJob.h"
#include "CLog.h"
#include "CString.h"
#include <string.h>
#include <assert.h>
#include <X11/X.h>
#include <X11/Xatom.h>
#include <set>

//
// CXWindowsScreen
//

CXWindowsScreen::CXWindowsScreen() :
								m_display(NULL),
								m_root(None),
								m_w(0), m_h(0),
								m_stop(false)
{
	// do nothing
}

CXWindowsScreen::~CXWindowsScreen()
{
	assert(m_display == NULL);
}

void					CXWindowsScreen::openDisplay()
{
	assert(m_display == NULL);

	// open the display
	log((CLOG_DEBUG "XOpenDisplay(%s)", "NULL"));
	m_display = XOpenDisplay(NULL);	// FIXME -- allow non-default
	if (m_display == NULL)
		throw int(5);	// FIXME -- make exception for this

	// get default screen
	m_screen = DefaultScreen(m_display);
	Screen* screen = ScreenOfDisplay(m_display, m_screen);

	// get screen size
	m_w = WidthOfScreen(screen);
	m_h = HeightOfScreen(screen);
	log((CLOG_INFO "display size: %dx%d", m_w, m_h));

	// get the root window
	m_root = RootWindow(m_display, m_screen);

	// get some atoms
	m_atomTargets      = XInternAtom(m_display, "TARGETS", False);
	m_atomData         = XInternAtom(m_display, "DESTINATION", False);
	m_atomINCR         = XInternAtom(m_display, "INCR", False);
	m_atomText         = XInternAtom(m_display, "TEXT", False);
	m_atomCompoundText = XInternAtom(m_display, "COMPOUND_TEXT", False);

	// let subclass prep display
	onOpenDisplay();
}

void					CXWindowsScreen::closeDisplay()
{
	assert(m_display != NULL);

	// let subclass close down display
	onCloseDisplay();

	// close the display
	XCloseDisplay(m_display);
	m_display = NULL;
	log((CLOG_DEBUG "closed display"));
}

int						CXWindowsScreen::getScreen() const
{
	assert(m_display != NULL);
	return m_screen;
}

Window					CXWindowsScreen::getRoot() const
{
	assert(m_display != NULL);
	return m_root;
}

void					CXWindowsScreen::getScreenSize(
								SInt32* w, SInt32* h) const
{
	assert(m_display != NULL);
	assert(w != NULL && h != NULL);

	*w = m_w;
	*h = m_h;
}

Cursor					CXWindowsScreen::createBlankCursor() const
{
	// this seems just a bit more complicated than really necessary

	// get the closet cursor size to 1x1
	unsigned int w, h;
	XQueryBestCursor(m_display, m_root, 1, 1, &w, &h);

	// make bitmap data for cursor of closet size.  since the cursor
	// is blank we can use the same bitmap for shape and mask:  all
	// zeros.
	const int size = ((w + 7) >> 3) * h;
	char* data = new char[size];
	memset(data, 0, size);

	// make bitmap
	Pixmap bitmap = XCreateBitmapFromData(m_display, m_root, data, w, h);

	// need an arbitrary color for the cursor
	XColor color;
	color.pixel = 0;
	color.red   = color.green = color.blue = 0;
	color.flags = DoRed | DoGreen | DoBlue;

	// make cursor from bitmap
	Cursor cursor = XCreatePixmapCursor(m_display, bitmap, bitmap,
								&color, &color, 0, 0);

	// don't need bitmap or the data anymore
	delete[] data;
	XFreePixmap(m_display, bitmap);

	return cursor;
}

bool					CXWindowsScreen::getEvent(XEvent* xevent) const
{
	// wait for an event in a cancellable way and don't lock the
	// display while we're waiting.
	m_mutex.lock();
	while (!m_stop && XPending(m_display) == 0) {
		m_mutex.unlock();
		CThread::sleep(0.05);
		m_mutex.lock();
	}
	if (m_stop) {
		m_mutex.unlock();
		return true;
	}
	else {
		XNextEvent(m_display, xevent);
		m_mutex.unlock();
		return false;
	}
}

void					CXWindowsScreen::doStop()
{
	CLock lock(&m_mutex);
	m_stop = true;
}

void					CXWindowsScreen::getDisplayClipboard(
								IClipboard* clipboard,
								Window requestor, Time timestamp) const
{
	assert(clipboard != NULL);
	assert(requestor != None);

	// clear the clipboard object
	clipboard->open();

	// block others from using the display while we get the clipboard.
	// in particular, this prevents the event thread from stealing the
	// selection notify event we're expecting.
	CLock lock(&m_mutex);

	// use PRIMARY selection as the "clipboard"
	Atom selection = XA_PRIMARY;

	// ask the selection for all the formats it has.  some owners return
	// the TARGETS atom and some the ATOM atom when TARGETS is requested.
	Atom format;
	CString targets;
	if (getDisplayClipboard(selection, m_atomTargets,
								requestor, timestamp, &format, &targets) &&
		(format == m_atomTargets || format == XA_ATOM)) {
		// get each target (that we can interpret).  some owners return
		// some targets multiple times in the list so don't try to get
		// those multiple times.
		const Atom* targetAtoms = reinterpret_cast<const Atom*>(targets.data());
		const SInt32 numTargets = targets.size() / sizeof(Atom);
		std::set<IClipboard::EFormat> clipboardFormats;
		std::set<Atom> targets;
		log((CLOG_DEBUG "selection has %d targets", numTargets));
		for (SInt32 i = 0; i < numTargets; ++i) {
			Atom format = targetAtoms[i];
			log((CLOG_DEBUG " source target %d", format));

			// skip already handled targets
			if (targets.count(format) > 0) {
				log((CLOG_DEBUG "  skipping handled target %d", format));
				continue;
			}

			// mark this target as done
			targets.insert(format);

			// determine the expected clipboard format
			IClipboard::EFormat expectedFormat = getFormat(format);

			// if we can use the format and we haven't already retrieved
			// it then get it
			if (expectedFormat == IClipboard::kNum) {
				log((CLOG_DEBUG "  no format for target", format));
				continue;
			}
			if (clipboardFormats.count(expectedFormat) > 0) {
				log((CLOG_DEBUG "  skipping handled format %d", expectedFormat));
				continue;
			}

			CString data;
			if (!getDisplayClipboard(selection, format,
							requestor, timestamp, &format, &data)) {
				log((CLOG_DEBUG "  no data for target", format));
				continue;
			}

			// use the actual format, not the expected
			IClipboard::EFormat actualFormat = getFormat(format);
			if (actualFormat == IClipboard::kNum) {
				log((CLOG_DEBUG "  no format for target", format));
				continue;
			}
			if (clipboardFormats.count(actualFormat) > 0) {
				log((CLOG_DEBUG "  skipping handled format %d", actualFormat));
				continue;
			}

			// add to clipboard and note we've done it
			clipboard->add(actualFormat, data);
			clipboardFormats.insert(actualFormat);
		}
	}
	else {
		// non-ICCCM conforming selection owner.  try TEXT format.
		// FIXME
		log((CLOG_DEBUG "selection doesn't support TARGETS, format is %d", format));
	}

	// done with clipboard
	clipboard->close();
}

bool					CXWindowsScreen::getDisplayClipboard(
								Atom selection, Atom type,
								Window requestor, Time timestamp,
								Atom* outputType, CString* outputData) const
{
	assert(outputType != NULL);
	assert(outputData != NULL);

	// delete data property
	XDeleteProperty(m_display, requestor, m_atomData);

	// request data conversion
	XConvertSelection(m_display, selection, type,
								m_atomData, requestor, timestamp);

	// wait for the selection notify event.  can't just mask out other
	// events because X stupidly doesn't provide a mask for selection
	// events, so we use a predicate to find our event.
	XEvent xevent;
	while (XCheckIfEvent(m_display, &xevent,
								&CXWindowsScreen::findSelectionNotify,
								(XPointer)&requestor) != True) {
		// wait a bit
		CThread::sleep(0.05);
	}
	assert(xevent.type                 == SelectionNotify);
	assert(xevent.xselection.requestor == requestor);

	// make sure the transfer worked
	Atom property = xevent.xselection.property;
	if (property == None) {
		// cannot convert
		*outputType = type;
		log((CLOG_DEBUG "selection conversion failed for %d", type));
		return false;
	}

	// get the data and discard the property
	SInt32 datumSize;
	CString data;
	bool okay = getData(requestor, property, outputType, &datumSize, &data);
	XDeleteProperty(m_display, requestor, property);

	// fail if we couldn't get the data
	if (!okay) {
		log((CLOG_DEBUG "can't get data for selection format %d", type));
		return false;
	}

	// handle INCR type specially.  it means we'll be receiving the data
	// piecemeal so we just loop until we've collected all the data.
	if (*outputType == m_atomINCR) {
		log((CLOG_DEBUG "selection data for format %d is incremental", type));
		// the data is a lower bound on the amount of data to be
		// transferred.  use it as a hint to size our buffer.
		UInt32 size;
		switch (datumSize) {
		  case 8:
			size = *(reinterpret_cast<const UInt8*>(data.data()));
			break;

		  case 16:
			size = *(reinterpret_cast<const UInt16*>(data.data()));
			break;

		  case 32:
			size = *(reinterpret_cast<const UInt32*>(data.data()));
			break;

		  default:
			assert(0 && "invalid datum size");
		}

		// empty the buffer and reserve the lower bound
		data.erase();
		data.reserve(size);

		// look for property notify events with the following
		PropertyNotifyInfo filter;
		filter.m_window   = requestor;
		filter.m_property = property;

		// now enter the INCR loop
		bool error = false;
		*outputType = (Atom)0;
		for (;;) {
			// wait for more data
			while (XCheckIfEvent(m_display, &xevent,
								&CXWindowsScreen::findPropertyNotify,
								(XPointer)&filter) != True) {
				// wait a bit
				CThread::sleep(0.05);
			}
			assert(xevent.type             == PropertyNotify);
			assert(xevent.xproperty.window == requestor);
			assert(xevent.xproperty.atom   == property);

			// get the additional data then delete the property to
			// ask the clipboard owner for the next chunk.
			Atom newType;
			CString newData;
			okay = getData(requestor, property, &newType, NULL, &newData);
			XDeleteProperty(m_display, requestor, property);

			// transfer has failed if we can't get the data
			if (!okay)
				error = true;

			// a zero length property means we got the last chunk
			if (newData.size() == 0)
				break;

			// if this is the first chunk then save the type.  otherwise
			// note that the new type is the same as the first chunk's
			// type.  if they're not the the clipboard owner is busted
			// but we have to continue the transfer because there's no
			// way to cancel it.
			if (*outputType == (Atom)0)
				*outputType = newType;
			else if (*outputType != newType)
				error = true;

			// append the data
			data += newData;
		}

		// if there was an error we could say the transferred failed
		// but we'll be liberal in what we accept.
		if (error) {
			log((CLOG_WARN "ICCCM violation by clipboard owner"));
//			return false;
		}
	}

	*outputData = data;
	return true;
}

bool					CXWindowsScreen::getData(
								Window window, Atom property,
								Atom* type, SInt32* datumSize,
								CString* data) const
{
	assert(type != NULL);
	assert(data != NULL);

	// clear out any existing data
	data->erase();

	// read the property
	long offset = 0;
	long length = 8192 / 4;
	for (;;) {
		// get more data
		int actualDatumSize;
		unsigned long numItems, bytesLeft;
		unsigned char* rawData;
		const int result = XGetWindowProperty(m_display, window, property,
								offset, length, False, AnyPropertyType,
								type, &actualDatumSize,
								&numItems, &bytesLeft,
								&rawData);
		if (result != Success) {
			// failed
			return false;
		}

		// save datum size
		if (datumSize != NULL)
			*datumSize = (SInt32)actualDatumSize;
		const SInt32 bytesPerDatum = (SInt32)actualDatumSize / 8;

		// advance read pointer.  since we can only read at offsets that
		// are multiples of 4 byte we take care to write multiples of 4
		// bytes to data, except when we've retrieved the last chunk.
		SInt32 quadCount = (numItems * bytesPerDatum) / 4;
		offset += quadCount;

		// append data
		if (bytesLeft == 0)
			data->append((char*)rawData, bytesPerDatum * numItems);
		else
			data->append((char*)rawData, 4 * quadCount);

		// done with returned data
		XFree(rawData);

		// done if no data is left
		if (bytesLeft == 0)
			return true;
	}
}

IClipboard::EFormat		CXWindowsScreen::getFormat(Atom src) const
{
	// FIXME -- handle more formats (especially mime-type-like formats
	// and various character encodings like unicode).
	if (src == XA_STRING ||
		src == m_atomText ||
		src == m_atomCompoundText)
		return IClipboard::kText;
	return IClipboard::kNum;
}

Bool					CXWindowsScreen::findSelectionNotify(
								Display*, XEvent* xevent, XPointer arg)
{
	Window requestor = *((Window*)arg);
	return (xevent->type                 == SelectionNotify &&
			xevent->xselection.requestor == requestor) ? True : False;
}

Bool					CXWindowsScreen::findPropertyNotify(
								Display*, XEvent* xevent, XPointer arg)
{
	PropertyNotifyInfo* filter = (PropertyNotifyInfo*)arg;
	return (xevent->type             == PropertyNotify &&
			xevent->xproperty.window == filter->m_window &&
			xevent->xproperty.atom   == filter->m_property &&
			xevent->xproperty.state  == PropertyNewValue) ? True : False;
}


//
// CXWindowsScreen::CDisplayLock
//

CXWindowsScreen::CDisplayLock::CDisplayLock(const CXWindowsScreen* screen) :
								m_mutex(&screen->m_mutex),
								m_display(screen->m_display)
{
	assert(m_display != NULL);

	m_mutex->lock();
}

CXWindowsScreen::CDisplayLock::~CDisplayLock()
{
	m_mutex->unlock();
}

CXWindowsScreen::CDisplayLock::operator Display*() const
{
	return m_display;
}
