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

set(h
	CTestEventQueue.h
)

set(src
	${h}
	Main.cpp
	CIpcTests.cpp
	NetworkTests.cpp
	CTestEventQueue.cpp
)

# arch
if (WIN32)
	list(APPEND src
		arch/CArchInternetWindowsTests.cpp
	)
elseif (UNIX)
	list(APPEND src
		arch/CArchInternetUnixTests.cpp
	)
endif()

# platform
if (WIN32)
	list(APPEND src
		platform/CMSWindowsClipboardTests.cpp
		platform/CMSWindowsKeyStateTests.cpp
	)
elseif (APPLE)
	list(APPEND src
		platform/COSXClipboardTests.cpp
		platform/COSXKeyStateTests.cpp
		platform/COSXScreenTests.cpp
	)
elseif (UNIX)
	list(APPEND src
		platform/CXWindowsClipboardTests.cpp
		platform/CXWindowsKeyStateTests.cpp
		platform/CXWindowsScreenTests.cpp
		platform/CXWindowsScreenSaverTests.cpp
	)
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
	../../lib/server
	../../lib/synergy
	../../lib/synwinhk
	../../lib/synwinxt
	../../../tools/gtest-1.6.0/include
	../../../tools/gmock-1.6.0/include
	../unittests
)

if (UNIX)
	list(APPEND inc
		../../..
	)
endif()

include_directories(${inc})
add_executable(integtests ${src})
target_link_libraries(integtests
	arch base client common io ipc mt net platform server synergy gtest gmock cryptopp ${libs})
