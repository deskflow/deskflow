#include "CMSWindowsClipboard.h"
#include "CString.h"
#include "CLog.h"

//
// CMSWindowsClipboard
//

CMSWindowsClipboard::CMSWindowsClipboard(HWND window) : m_window(window)
{
	// do nothing
}

CMSWindowsClipboard::~CMSWindowsClipboard()
{
	// do nothing
}

bool					CMSWindowsClipboard::open()
{
	log((CLOG_INFO "open clipboard"));

	if (!OpenClipboard(m_window)) {
		log((CLOG_WARN "failed to open clipboard"));
		return false;
	}
	if (EmptyClipboard()) {
		log((CLOG_DEBUG "grabbed clipboard"));
	}
	else {
		log((CLOG_WARN "failed to grab clipboard"));
	}
	return true;
}

void					CMSWindowsClipboard::close()
{
	log((CLOG_INFO "close clipboard"));
	CloseClipboard();
}

void					CMSWindowsClipboard::add(
								EFormat format, const CString& data)
{
	log((CLOG_INFO "add %d bytes to clipboard format: %d", data.size(), format));

	if (!OpenClipboard(m_window)) {
		log((CLOG_WARN "failed to open clipboard"));
		return;
	}

	// convert data to win32 required form
	const UINT win32Format = convertFormatToWin32(format);
	HANDLE win32Data;
	switch (win32Format) {
	  case CF_TEXT:
		win32Data = convertTextToWin32(data);
		break;

	  default:
		win32Data = NULL;
		break;
	}

	// put the data on the clipboard
	if (win32Data != NULL) {
		SetClipboardData(win32Format, win32Data);
	}

	// done with clipboard
	CloseClipboard();
}

bool					CMSWindowsClipboard::has(EFormat format) const
{
	const UINT win32Format = convertFormatToWin32(format);
	return (win32Format != 0 && IsClipboardFormatAvailable(win32Format) != 0);
}

CString					CMSWindowsClipboard::get(EFormat format) const
{
	// get the win32 format.  return empty data if unknown format.
	const UINT win32Format = convertFormatToWin32(format);
	if (win32Format == 0)
		return CString();

	if (!OpenClipboard(m_window)) {
		log((CLOG_WARN "failed to open clipboard"));
		return CString();
	}

	// get a handle to the clipboard data and convert it
	HANDLE win32Data = GetClipboardData(win32Format);
	CString data;
	if (win32Data != NULL) {
		// convert the data
		switch (win32Format) {
		  case CF_TEXT:
			data = convertTextFromWin32(win32Data);
		}
	}

	// close the clipboard
	CloseClipboard();

	return data;
}

UINT					CMSWindowsClipboard::convertFormatToWin32(
								EFormat format) const
{
	switch (format) {
	  case kText:
		return CF_TEXT;

	  default:
		return 0;
	}
}

HANDLE					CMSWindowsClipboard::convertTextToWin32(
								const CString& data) const
{
	// compute size of converted text
	UInt32 dstSize = 0;
	const UInt32 srcSize = data.size();
	const char* src = data.c_str();
	for (UInt32 index = 0; index < srcSize; ++index) {
		if (src[index] == '\n') {
			// add \r
			++dstSize;
		}
		++dstSize;
	}

	// allocate
	HGLOBAL gData = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, dstSize);
	if (gData != NULL) {
		// get a pointer to the allocated memory
		char* dst = (char*)GlobalLock(gData);
		if (dst != NULL) {
			// convert text.  we change LF to CRLF.
			dstSize = 0;
			for (UInt32 index = 0; index < srcSize; ++index) {
				if (src[index] == '\n') {
					// add \r
					dst[dstSize++] = '\r';
				}
				dst[dstSize++] = src[index];
			}

			// done converting
			GlobalUnlock(gData);
		}
	}
	return gData;
}

CString					CMSWindowsClipboard::convertTextFromWin32(
								HANDLE handle) const
{
	// get source data and it's size
	const char* src = (const char*)GlobalLock(handle);
	UInt32 srcSize = (SInt32)GlobalSize(handle);
	if (src == NULL || srcSize == 0)
		return CString();

	// compute size of converted text
	UInt32 dstSize = 0;
	UInt32 index;
	for (index = 0; index < srcSize; ++index) {
		if (src[index] == '\r') {
			// skip \r
			if (index + 1 < srcSize && src[index + 1] == '\n')
				++index;
		}
		++dstSize;
	}

	// allocate
	CString data;
	data.reserve(dstSize);

	// convert text.  we change CRLF to LF.
	for (index = 0; index < srcSize; ++index) {
		if (src[index] == '\r') {
			// skip \r
			if (index + 1 < srcSize && src[index + 1] == '\n')
				++index;
		}
		data += src[index];
	}

	return data;
}
