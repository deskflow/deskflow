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

# remove files which are not cross platform
list(REMOVE_ITEM headers "*Windows*")
list(REMOVE_ITEM headers "*Unix*")

# arch
if (WIN32)
	file(GLOB arch_sources "*Windows*.cpp")
	file(GLOB arch_headers "*Windows*.h")
elseif (UNIX)
	file(GLOB arch_sources "*Unix*.cpp")
	file(GLOB arch_headers "*Unix*.h")
endif()

list(APPEND sources ${arch_sources})
list(APPEND headers ${arch_headers})

include_directories(
	../arch
	../base
	../client
	../common
	../io
	../ipc
	../mt
	../net
	../platform
	../server
	../synergy
	../synwinhk
	../synwinxt
	../..
	../../../ext
)

if (UNIX)
	include_directories(
		../../..
	)
endif()

add_library(synergy STATIC ${sources})

if (UNIX)
	target_link_libraries(synergy arch client ipc net base platform mt server)
endif()
