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

#include "CMSWindowsDesktop.h"
#include "CLog.h"
#include "CArchMiscWindows.h"
#include <malloc.h>

//
// CMSWindowsDesktop
//

HDESK
CMSWindowsDesktop::openInputDesktop()
{
	if (CArchMiscWindows::isWindows95Family()) {
		// there's only one desktop on windows 95 et al.
		return GetThreadDesktop(GetCurrentThreadId());
	}
	else {
		return OpenInputDesktop(DF_ALLOWOTHERACCOUNTHOOK, TRUE,
								DESKTOP_CREATEWINDOW |
									DESKTOP_HOOKCONTROL |
									GENERIC_WRITE);
	}
}

void
CMSWindowsDesktop::closeDesktop(HDESK desk)
{
	// on 95/98/me we don't need to close the desktop returned by
	// openInputDesktop().
	if (desk != NULL && !CArchMiscWindows::isWindows95Family()) {
		CloseDesktop(desk);
	}
}

bool
CMSWindowsDesktop::setDesktop(HDESK desk)
{
	// 95/98/me doesn't support multiple desktops so just return
	// true on those platforms.
	return (CArchMiscWindows::isWindows95Family() ||
			SetThreadDesktop(desk) != 0);
}

CString
CMSWindowsDesktop::getDesktopName(HDESK desk)
{
	if (desk == NULL) {
		return CString();
	}
	else if (CArchMiscWindows::isWindows95Family()) {
		return "desktop";
	}
	else {
		DWORD size;
		GetUserObjectInformation(desk, UOI_NAME, NULL, 0, &size);
		TCHAR* name = (TCHAR*)alloca(size + sizeof(TCHAR));
		GetUserObjectInformation(desk, UOI_NAME, name, size, &size);
		CString result(name);
		return result;
	}
}
