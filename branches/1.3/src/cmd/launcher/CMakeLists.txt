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

IF(WIN32)
	SET(src_cmd_launcher_mswindows
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

	SET(inc_cmd_launcher_mswindows
		CAddScreen.h
		CAdvancedOptions.h
		CAutoStart.h
		CGlobalOptions.h
		CHotkeyOptions.h
		CInfo.h
		CScreensLinks.h
		LaunchUtil.h
		resource.h
	)

	SET(res_cmd_launcher_mswindows
		launcher.rc
		synergy.ico
	)

	SET(src_cmd_launcher)

	IF(WIN32)
		LIST(APPEND src_cmd_launcher 
		${inc_cmd_launcher_mswindows}
		${res_cmd_launcher_mswindows}
		${src_cmd_launcher_mswindows}
		)
	ENDIF(WIN32)

	SET(inc_dirs_cmd_launcher
		../../lib/arch
		../../lib/base
		../../lib/client
		../../lib/common
		../../lib/io
		../../lib/mt
		../../lib/net
		../../lib/platform
		../../lib/synergy
	)

	INCLUDE_DIRECTORIES(${inc_dirs_cmd_launcher})
	ADD_EXECUTABLE(launcher WIN32 ${src_cmd_launcher})
	TARGET_LINK_LIBRARIES(launcher synergy ${libs})
	
ENDIF(WIN32)
