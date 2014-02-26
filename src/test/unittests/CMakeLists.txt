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

file(GLOB_RECURSE headers "*.h")
file(GLOB_RECURSE sources "*.cpp")

include_directories(
	../../lib/arch
	../../lib/base
	../../lib/client
	../../lib/server
	../../lib/common
	../../lib/io
	../../lib/mt
	../../lib/net
	../../lib/platform
	../../lib/synergy
	../../../ext/gtest-1.6.0/include
	../../../ext/gmock-1.6.0/include
	../../../ext
	io
	synergy
)

if (UNIX)
	include_directories(
		../../..
	)
endif()

if (SYNERGY_ADD_HEADERS)
	list(APPEND sources ${headers})
endif()

add_executable(unittests ${sources})
target_link_libraries(unittests
	arch base client server common io net platform server synergy mt gtest gmock cryptopp ${libs})
