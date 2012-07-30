/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Nick Bolton
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

#include "CMSWindowsDebugOutputter.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <string>

CMSWindowsDebugOutputter::CMSWindowsDebugOutputter()
{
}

CMSWindowsDebugOutputter::~CMSWindowsDebugOutputter()
{
}

void
CMSWindowsDebugOutputter::open(const char* title)
{
}

void
CMSWindowsDebugOutputter::close()
{
}

void
CMSWindowsDebugOutputter::show(bool showIfEmpty)
{
}

bool
CMSWindowsDebugOutputter::write(ELevel level, const char* msg)
{
	OutputDebugString((std::string(msg) + "\n").c_str());
	return true;
}

void
CMSWindowsDebugOutputter::flush()
{
}
