#include "CMSWindowsScreen.h"
#include "CThread.h"
#include "CLock.h"
#include "TMethodJob.h"
#include "CLog.h"
#include "CString.h"
#include <string.h>
#include <assert.h>

//
// CMSWindowsScreen
//

HINSTANCE				CMSWindowsScreen::s_instance = NULL;
CMSWindowsScreen*		CMSWindowsScreen::s_screen = NULL;

CMSWindowsScreen::CMSWindowsScreen() :
								m_class(0),
								m_cursor(NULL),
								m_w(0), m_h(0),
								m_thread(0)
{
	assert(s_screen == NULL);
	s_screen = this;
}

CMSWindowsScreen::~CMSWindowsScreen()
{
	assert(m_class == 0);
	s_screen = NULL;
}

void					CMSWindowsScreen::init(HINSTANCE instance)
{
	s_instance = instance;
}

void					CMSWindowsScreen::doRun()
{
	m_thread = GetCurrentThreadId();
	for (;;) {
		// wait for and get the next event
		MSG msg;
		getEvent(&msg);

		// handle quit message
		if (msg.message == WM_QUIT) {
			break;
		}

		// dispatch message
		if (!onPreTranslate(&msg)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
}

void					CMSWindowsScreen::doStop()
{
	PostThreadMessage(m_thread, WM_QUIT, 0, 0);
}

void					CMSWindowsScreen::openDisplay()
{
	assert(s_instance != NULL);
	assert(m_class    == 0);

	// create a transparent cursor
	int cw = GetSystemMetrics(SM_CXCURSOR);
	int ch = GetSystemMetrics(SM_CYCURSOR);
	UInt8* cursorAND = new UInt8[ch * ((cw + 31) >> 2)];
	UInt8* cursorXOR = new UInt8[ch * ((cw + 31) >> 2)];
	memset(cursorAND, 0xff, ch * ((cw + 31) >> 2));
	memset(cursorXOR, 0x00, ch * ((cw + 31) >> 2));
	m_cursor = CreateCursor(s_instance, 0, 0, cw, ch, cursorAND, cursorXOR);
	delete[] cursorXOR;
	delete[] cursorAND;

	// register a window class
	WNDCLASSEX classInfo;
	classInfo.cbSize        = sizeof(classInfo);
	classInfo.style         = CS_DBLCLKS | CS_NOCLOSE;
	classInfo.lpfnWndProc   = &CMSWindowsScreen::wndProc;
	classInfo.cbClsExtra    = 0;
	classInfo.cbWndExtra    = 0;
	classInfo.hInstance     = s_instance;
	classInfo.hIcon         = NULL;
	classInfo.hCursor       = m_cursor;
	classInfo.hbrBackground = NULL;
	classInfo.lpszMenuName  = NULL;
	classInfo.lpszClassName = "Synergy";
	classInfo.hIconSm       = NULL;
	m_class = RegisterClassEx(&classInfo);

	// get screen size
	// FIXME -- should handle multiple screens
	m_w = GetSystemMetrics(SM_CXSCREEN);
	m_h = GetSystemMetrics(SM_CYSCREEN);
	log((CLOG_INFO "display size: %dx%d", m_w, m_h));

	// let subclass prep display
	onOpenDisplay();
}

void					CMSWindowsScreen::closeDisplay()
{
	assert(s_instance    != NULL);
	assert(m_class       != 0);

	// let subclass close down display
	onCloseDisplay();

	// unregister the window class
	UnregisterClass((LPCTSTR)m_class, s_instance);
	m_class = 0;

	// delete resources
	DestroyCursor(m_cursor);
	m_cursor = NULL;

	log((CLOG_DEBUG "closed display"));
}

HINSTANCE				CMSWindowsScreen::getInstance()
{
	return s_instance;
}

ATOM					CMSWindowsScreen::getClass() const
{
	return m_class;
}

void					CMSWindowsScreen::getScreenSize(
								SInt32* w, SInt32* h) const
{
	assert(m_class != 0);
	assert(w != NULL && h != NULL);

	*w = m_w;
	*h = m_h;
}

void					CMSWindowsScreen::getEvent(MSG* msg) const
{
	// wait for an event in a cancellable way
	while (HIWORD(GetQueueStatus(QS_ALLINPUT)) == 0) {
		CThread::sleep(0.05);
	}
	GetMessage(msg, NULL, 0, 0);
}

void					CMSWindowsScreen::getDisplayClipboard(
								IClipboard* clipboard,
								HWND requestor) const
{
/* FIXME
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
			if (expectedFormat == IClipboard::kNumFormats) {
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
			if (actualFormat == IClipboard::kNumFormats) {
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
*/
}

LRESULT CALLBACK		CMSWindowsScreen::wndProc(
								HWND hwnd, UINT msg,
								WPARAM wParam, LPARAM lParam)
{
	assert(s_screen != NULL);
	return s_screen->onEvent(hwnd, msg, wParam, lParam);
}
