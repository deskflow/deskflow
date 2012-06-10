/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2004 Chris Schoeneman, Nick Bolton, Sorin Sbarnea
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "CArchSystemUnix.h"
#include <sys/utsname.h>

//
// CArchSystemUnix
//

CArchSystemUnix::CArchSystemUnix()
{
	// do nothing
}

CArchSystemUnix::~CArchSystemUnix()
{
	// do nothing
}

std::string
CArchSystemUnix::getOSName() const
{
#if defined(HAVE_SYS_UTSNAME_H)
	struct utsname info;
	if (uname(&info) == 0) {
		std::string msg;
		msg += info.sysname;
		msg += " ";
		msg += info.release;
		msg += " ";
		msg += info.version;
		return msg;
	}
#endif
	return "Unix";
}

std::string
CArchSystemUnix::getPlatformName() const
{
#if defined(HAVE_SYS_UTSNAME_H)
	struct utsname info;
	if (uname(&info) == 0) {
		return std::string(info.machine);
	}
#endif
	return "unknown";
}

std::string
CArchSystemUnix::setting(const std::string&) const
{
	return "";
}

void
CArchSystemUnix::setting(const std::string&, const std::string&) const
{
}
