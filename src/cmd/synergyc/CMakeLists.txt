# synergy -- mouse and keyboard sharing utility
# Copyright (C) 2012 Bolton Software Ltd.
# Copyright (C) 2009 Nick Bolton
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
	synergyc.cpp
)

if (WIN32)
	list(APPEND src
		CMSWindowsClientTaskBarReceiver.cpp
		CMSWindowsClientTaskBarReceiver.h
		resource.h
		synergyc.ico
		synergyc.rc
		tb_error.ico
		tb_idle.ico
		tb_run.ico
		tb_wait.ico
	)
elseif (APPLE)
	list(APPEND src COSXClientTaskBarReceiver.cpp)
elseif (UNIX)
	list(APPEND src CXWindowsClientTaskBarReceiver.cpp)	
endif()

set(inc
	../../lib/arch
	../../lib/base
	../../lib/client
	../../lib/common
	../../lib/io
	../../lib/ipc
	../../lib/mt
	../../lib/net
	../../lib/platform
	../../lib/synergy
)

if (UNIX)
	list(APPEND inc
		../../..
	)
endif()

if (VNC_SUPPORT)
	list(APPEND libs vnc_server vnc_client)
endif()

include_directories(${inc})
add_executable(synergyc ${src})
target_link_libraries(synergyc
	arch base client common io mt net ipc platform server synergy cryptopp ${libs})

if (CONF_CPACK)
	install(TARGETS
		synergyc
		COMPONENT core
		DESTINATION bin)
endif()
