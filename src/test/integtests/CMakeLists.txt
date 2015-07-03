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

file(GLOB_RECURSE headers "*.h")
file(GLOB_RECURSE sources "*.cpp")

# remove platform files (specific platform added later).
file(GLOB_RECURSE remove_platform "platform/*")
list(REMOVE_ITEM headers ${remove_platform})
list(REMOVE_ITEM sources ${remove_platform})

# platform
if (WIN32)
	file(GLOB platform_sources "platform/MSWindows*.cpp")
	file(GLOB platform_headers "platform/MSWindows*.h")
elseif (APPLE)
	file(GLOB platform_sources "platform/OSX*.cpp")
	file(GLOB platform_headers "platform/OSX*.h")
elseif (UNIX)
	file(GLOB platform_sources "platform/XWindows*.cpp")
	file(GLOB platform_headers "platform/XWindows*.h")
endif()

list(APPEND sources ${platform_sources})
list(APPEND headers ${platform_headers})

file(GLOB_RECURSE global_headers "../../test/global/*.h")
file(GLOB_RECURSE global_sources "../../test/global/*.cpp")

list(APPEND headers ${global_headers})
list(APPEND sources ${global_sources})

file(GLOB_RECURSE mock_headers "../../test/mock/*.h")
file(GLOB_RECURSE mock_sources "../../test/mock/*.cpp")

list(APPEND headers ${mock_headers})
list(APPEND sources ${mock_sources})

if (SYNERGY_ADD_HEADERS)
	list(APPEND sources ${headers})
endif()

include_directories(
	../../
	../../lib/
	../../../ext/gtest-1.6.0/include
	../../../ext/gmock-1.6.0/include
)

if (UNIX)
	include_directories(
		../../..
	)
endif()

add_executable(integtests ${sources})
target_link_libraries(integtests
	arch base client common io ipc mt net platform server synergy gtest gmock ${libs})
