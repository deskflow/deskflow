# synergy -- mouse and keyboard sharing utility
# Copyright (C) 2009 Chris Schoeneman, Nick Bolton, Sorin Sbarnea
# 
# This package is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# found in the file COPYING that should have accompanied this file.
# 
# This package is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

set(src
	CAddScreen.h
	CAdvancedOptions.h
	CAutoStart.h
	CGlobalOptions.h
	CHotkeyOptions.h
	CInfo.h
	CScreensLinks.h
	LaunchUtil.h
	resource.h
	launcher.rc
	synergy.ico
	CAddScreen.cpp
	CAdvancedOptions.cpp
	CAutoStart.cpp
	CGlobalOptions.cpp
	CHotkeyOptions.cpp
	CInfo.cpp
	CScreensLinks.cpp
	LaunchUtil.cpp
	launcher.cpp
)

set(inc
	../../lib/server
	../../lib/arch
	../../lib/base
	../../lib/common
	../../lib/net
	../../lib/platform
	../../lib/synergy
)

include_directories(${inc})
add_executable(launcher WIN32 ${src})
target_link_libraries(launcher synergy ${libs})
target_link_libraries(launcher
	arch base common io mt net platform server ${libs})
