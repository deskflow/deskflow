/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Bolton Software Ltd.
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "CArchDaemonNone.h"

//
// CArchDaemonNone
//

CArchDaemonNone::CArchDaemonNone()
{
	// do nothing
}

CArchDaemonNone::~CArchDaemonNone()
{
	// do nothing
}

void
CArchDaemonNone::installDaemon(const char*,
				const char*,
				const char*,
				const char*,
				const char*,
				bool)
{
	// do nothing
}

void
CArchDaemonNone::uninstallDaemon(const char*, bool)
{
	// do nothing
}

int
CArchDaemonNone::daemonize(const char* name, DaemonFunc func)
{
	// simply forward the call to func.  obviously, this doesn't
	// do any daemonizing.
	return func(1, &name);
}

bool
CArchDaemonNone::canInstallDaemon(const char*, bool)
{
	return false;
}

bool
CArchDaemonNone::isDaemonInstalled(const char*, bool)
{
	return false;
}

void
CArchDaemonNone::installDaemon()
{
}

void
CArchDaemonNone::uninstallDaemon()
{
}

std::string
CArchDaemonNone::commandLine() const
{
	return "";
}
