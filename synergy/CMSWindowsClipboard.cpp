#include "CMSWindowsClipboard.h"
#include "CString.h"
#include "CLog.h"

//
// CMSWindowsClipboard
//

CMSWindowsClipboard::CMSWindowsClipboard()
{
}

CMSWindowsClipboard::~CMSWindowsClipboard()
{
}

void					CMSWindowsClipboard::open()
{
	log((CLOG_INFO "open clipboard"));
}

void					CMSWindowsClipboard::close()
{
	log((CLOG_INFO "close clipboard"));
}

void					CMSWindowsClipboard::add(
								EFormat format, const CString& data)
{
	log((CLOG_INFO "add clipboard format: %d\n%s", format, data.c_str()));
}

bool					CMSWindowsClipboard::has(EFormat /*format*/) const
{
	return false;
}

CString					CMSWindowsClipboard::get(EFormat /*format*/) const
{
	return CString();
}
