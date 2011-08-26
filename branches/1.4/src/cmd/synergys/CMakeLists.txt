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
	synergys.cpp
)

if (WIN32)
	list(APPEND src
		CMSWindowsServerTaskBarReceiver.cpp
		CMSWindowsServerTaskBarReceiver.h
		resource.h
		synergys.ico
		synergys.rc
		tb_error.ico
		tb_idle.ico
		tb_run.ico
		tb_wait.ico
	)
elseif (APPLE)
	list(APPEND src COSXServerTaskBarReceiver.cpp)	
elseif (UNIX)
	list(APPEND src CXWindowsServerTaskBarReceiver.cpp)
endif()

set(inc
	../../lib/arch
	../../lib/base
	../../lib/common
	../../lib/io
	../../lib/mt
	../../lib/net
	../../lib/platform
	../../lib/synergy
	../../lib/server
)

if (UNIX)
	list(APPEND inc
		../../..
	)
endif()

include_directories(${inc})
add_executable(synergys ${src})
target_link_libraries(synergys
	arch base client common io mt net platform server synergy ${libs})

if (CONF_CPACK)
	install(TARGETS
		synergys
		COMPONENT core
		DESTINATION bin)
endif()
