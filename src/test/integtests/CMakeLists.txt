# barrier -- mouse and keyboard sharing utility
# Copyright (C) 2012-2016 Symless Ltd.
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

set(headers
)
set(sources
    arch/ArchInternetTests.cpp
    ipc/IpcTests.cpp
    net/NetworkTests.cpp
    Main.cpp
)

# platform
if (WIN32)
    set(platform_sources
        platform/MSWindowsClipboardTests.cpp
        platform/MSWindowsKeyStateTests.cpp
    )
    set(platform_headers)
elseif (APPLE)
    set(platform_sources
        platform/OSXClipboardTests.cpp
        platform/OSXKeyStateTests.cpp
        platform/OSXScreenTests.cpp
    )
    set(platform_headers)
elseif (UNIX)
    set(platform_sources
        platform/XWindowsClipboardTests.cpp
        platform/XWindowsKeyStateTests.cpp
        platform/XWindowsScreenSaverTests.cpp
        platform/XWindowsScreenTests.cpp
    )
    set(platform_headers)
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

if (BARRIER_ADD_HEADERS)
    list(APPEND sources ${headers})
endif()

include_directories(
    ../../
)

if (UNIX)
    include_directories(
        ../../..
    )
endif()

add_executable(integtests ${sources})
target_link_libraries(integtests
    arch base client common io ipc mt net platform server synlib ${GTEST_LIBRARIES} ${GMOCK_LIBRARIES} ${libs} ${OPENSSL_LIBS})
