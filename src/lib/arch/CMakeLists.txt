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

file(GLOB headers "*.h")
file(GLOB sources "*.cpp")

if (BARRIER_ADD_HEADERS)
    list(APPEND sources ${headers})
endif()

# arch
if (WIN32)
    file(GLOB arch_headers "win32/*.h")
    file(GLOB arch_sources "win32/*.cpp")
elseif (UNIX)
    file(GLOB arch_headers "unix/*.h")
    file(GLOB arch_sources "unix/*.cpp")
endif()

list(APPEND sources ${arch_sources})
list(APPEND headers ${arch_headers})

if (BARRIER_ADD_HEADERS)
    list(APPEND sources ${headers})
endif()

add_library(arch STATIC ${sources})

if (UNIX)
    target_link_libraries(arch ${libs})
    if (NOT CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
        target_link_libraries(arch dl)
    endif()
endif()
