/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2004 Chris Schoeneman
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

#define WIN32_LEAN_AND_MEAN

#include "CArchSystemWindows.h"
#include <windows.h>

//
// CArchSystemWindows
//

CArchSystemWindows::CArchSystemWindows()
{
	// do nothing
}

CArchSystemWindows::~CArchSystemWindows()
{
	// do nothing
}

std::string
CArchSystemWindows::getOSName() const
{
	OSVERSIONINFO info;
	info.dwOSVersionInfoSize = sizeof(info);
	if (GetVersionEx(&info)) {
		switch (info.dwPlatformId) {
		case VER_PLATFORM_WIN32_NT:
			if (info.dwMajorVersion == 5 && info.dwMinorVersion == 2) {
				return "Microsoft Windows Server 2003";
			}
			if (info.dwMajorVersion == 5 && info.dwMinorVersion == 1) {
				return "Microsoft Windows Server XP";
			}
			if (info.dwMajorVersion == 5 && info.dwMinorVersion == 0) {
				return "Microsoft Windows Server 2000";
			}
			if (info.dwMajorVersion <= 4) {
				return "Microsoft Windows NT";
			}
			char buffer[100];
			sprintf(buffer, "Microsoft Windows %d.%d",
							info.dwMajorVersion, info.dwMinorVersion);
			return buffer;

		case VER_PLATFORM_WIN32_WINDOWS:
			if (info.dwMajorVersion == 4 && info.dwMinorVersion == 0) {
				if (info.szCSDVersion[1] == 'C' ||
					info.szCSDVersion[1] == 'B') {
					return "Microsoft Windows 95 OSR2";
				}
				return "Microsoft Windows 95";
			}
			if (info.dwMajorVersion == 4 && info.dwMinorVersion == 10) {
				if (info.szCSDVersion[1] == 'A') {
					return "Microsoft Windows 98 SE";
				}
				return "Microsoft Windows 98";
			}
			if (info.dwMajorVersion == 4 && info.dwMinorVersion == 90) {
				return "Microsoft Windows ME";
			}
			if (info.dwMajorVersion == 4) {
				return "Microsoft Windows unknown 95 family";
			}
			break;

		default:
			break;
		}
	}
	return "Microsoft Windows <unknown>";
}
