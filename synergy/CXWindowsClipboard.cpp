#include "CXWindowsClipboard.h"
#include "CString.h"
#include "CLog.h"

//
// CXWindowsClipboard
//

CXWindowsClipboard::CXWindowsClipboard()
{
}

CXWindowsClipboard::~CXWindowsClipboard()
{
}

bool					CXWindowsClipboard::open()
{
	log((CLOG_INFO "open clipboard"));
	return true;
}

void					CXWindowsClipboard::close()
{
	log((CLOG_INFO "close clipboard"));
}

void					CXWindowsClipboard::add(
								EFormat format, const CString& data)
{
	log((CLOG_INFO "add clipboard format: %d\n%s", format, data.c_str()));
}

bool					CXWindowsClipboard::has(EFormat /*format*/) const
{
	return false;
}

CString					CXWindowsClipboard::get(EFormat /*format*/) const
{
	return CString();
}
