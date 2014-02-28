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

file(GLOB headers "*.h")
file(GLOB sources "*.cpp")

# arch
if (WIN32)
	file(GLOB arch_sources "arch/CArchWindows*.cpp")
	file(GLOB arch_headers "arch/CArchWindows*.h")
elseif (UNIX)
	file(GLOB arch_sources "arch/CArchUnix*.cpp")
	file(GLOB arch_headers "arch/CArchUnix*.h")
endif()

list(APPEND sources ${arch_sources})
list(APPEND headers ${arch_headers})

# platform
if (WIN32)
	file(GLOB platform_sources "platform/CMSWindows*.cpp")
	file(GLOB platform_headers "platform/CMSWindows*.h")
elseif (APPLE)
	file(GLOB platform_sources "platform/COSX*.cpp")
	file(GLOB platform_headers "platform/COSX*.h")
elseif (UNIX)
	file(GLOB platform_sources "platform/CXWindows*.cpp")
	file(GLOB platform_headers "platform/CXWindows*.h")
endif()

list(APPEND sources ${platform_sources})
list(APPEND headers ${platform_headers})

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
	arch base client common io ipc mt net platform server synergy gtest gmock cryptopp ${libs})
