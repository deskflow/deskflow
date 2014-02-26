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

set(sources
	synergyp.cpp
)

if (WIN32)
	list(APPEND sources
		CMSWindowsPortableTaskBarReceiver.cpp
		CMSWindowsPortableTaskBarReceiver.h
		resource.h
		synergyp.ico
		synergyp.rc
		tb_error.ico
		tb_idle.ico
		tb_run.ico
		tb_wait.ico
	)
elseif (APPLE)
	list(APPEND sources COSXPortableTaskBarReceiver.cpp)	
elseif (UNIX)
	list(APPEND sources CXWindowsPortableTaskBarReceiver.cpp)
endif()

include_directories(
	../../lib/arch
	../../lib/base
	../../lib/common
	../../lib/io
	../../lib/ipc
	../../lib/mt
	../../lib/net
	../../lib/platform
	../../lib/synergy
	../../lib/server
	../../lib/synwinhk
	../../lib/synwinxt
)

if (UNIX)
	include_directories(
		../../..
	)
endif()

if (WIN32)
	add_executable(synergyp WIN32 ${sources})
else()
	add_executable(synergyp ${sources})
endif()	
	
target_link_libraries(synergyp
	arch base client common io mt net ipc platform server synergy client cryptopp ${libs})

if (CONF_CPACK)
	install(TARGETS
		synergyp
		COMPONENT core
		DESTINATION bin)
endif()
