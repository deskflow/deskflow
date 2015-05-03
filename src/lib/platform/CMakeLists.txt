# synergy -- mouse and keyboard sharing utility
# Copyright (C) 2012 Synergy Si Ltd.
# Copyright (C) 2009 Nick Bolton
# 
# This package is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# found in the file LICENSE that should have accompanied this file.
# 
# This package is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

if (WIN32)
	file(GLOB headers "MSWindows*.h")
	file(GLOB sources "MSWindows*.cpp")
elseif (APPLE)
	file(GLOB headers "OSX*.h")
	file(GLOB sources "OSX*.cpp" "OSX*.m")
elseif (UNIX)
	file(GLOB headers "XWindows*.h")
	file(GLOB sources "XWindows*.cpp")
endif()

if (SYNERGY_ADD_HEADERS)
	list(APPEND sources ${headers})
endif()

include_directories(
	../
)

if (UNIX)
	include_directories(
		../../..
	)
endif()

if (APPLE)
	list(APPEND inc
		/System/Library/Frameworks
	)
endif()

include_directories(${inc})
add_library(platform STATIC ${sources})
target_link_libraries(platform client ${libs})

if (UNIX)
	target_link_libraries(platform io net ipc synergy client ${libs})
endif()

if (APPLE)
	find_library(COCOA_LIBRARY Cocoa)
	target_link_libraries(platform ${COCOA_LIBRARY})
endif()
