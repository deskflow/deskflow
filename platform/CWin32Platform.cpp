#include "CWin32Platform.h"
#include "CLog.h"
#include <string.h>
#include <windows.h>

//
// CWin32Platform
//

CWin32Platform::CWin32Platform()
{
	// do nothing
}

CWin32Platform::~CWin32Platform()
{
	// do nothing
}

bool					CWin32Platform::installDaemon(/* FIXME */)
{
	// FIXME
	return false;
}

bool					CWin32Platform::uninstallDaemon(/* FIXME */)
{
	// FIXME
	return false;
}

bool					CWin32Platform::daemonize(const char* name)
{
	// FIXME
	return false;
}

const char*				CWin32Platform::getBasename(const char* pathname) const
{
	if (pathname == NULL) {
		return NULL;
	}

	// check for last /
	const char* basename = strrchr(pathname, '/');
	if (basename != NULL) {
		++basename;
	}
	else {
		basename = pathname;
	}

	// check for last backslash
	const char* basename2 = strrchr(pathname, '\\');
	if (basename2 != NULL && basename2 > basename) {
		basename = basename2 + 1;
	}

	return basename;
}

CString					CWin32Platform::getUserDirectory() const
{
	// FIXME
	return CString();
}

CString					CWin32Platform::getSystemDirectory() const
{
	// FIXME
	return "";
}

CString					CWin32Platform::addPathComponent(
								const CString& prefix,
								const CString& suffix) const
{
	CString path;
	path.reserve(prefix.size() + 1 + suffix.size());
	path += prefix;
	path += '\\';
	path += suffix;
	return path;
}

void					CWin32Platform::serviceLogger(
								int priority, const char* msg)
{
	// FIXME
}
