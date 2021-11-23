# synergy -- mouse and keyboard sharing utility
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

file(GLOB headers "*.h" "languages/*.h")
file(GLOB sources "*.cpp" "languages/*.cpp")

# arch
if (WIN32)
    file(GLOB arch_headers "win32/*.h")
    file(GLOB arch_sources "win32/*.cpp")
    include_directories("../../../ext/WinToast/src")
    list(APPEND arch_sources "../../../ext/WinToast/src/wintoastlib.cpp")
elseif (UNIX)
    file(GLOB arch_headers "unix/*.h")
    file(GLOB arch_sources "unix/*.cpp")
endif()

option(SYSTEM_PUGIXML "Use system pugixml instead of vendored one" OFF)
if (SYSTEM_PUGIXML)
    find_package(pugixml REQUIRED)
else()
    include_directories("../../../ext/pugixml/src")
    list(APPEND arch_sources "../../../ext/pugixml/src/pugixml.cpp")
endif()

list(APPEND sources ${arch_sources})
list(APPEND headers ${arch_headers})

if (SYNERGY_ADD_HEADERS)
    list(APPEND sources ${headers})
endif()

add_library(synlib STATIC ${sources})

if (UNIX)
    target_link_libraries(synlib arch client ipc net base platform mt server)
    if (NOT APPLE)
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(lib_glib REQUIRED IMPORTED_TARGET glib-2.0)
        pkg_search_module( PC_GDKPIXBUF gdk-pixbuf-2.0 )
        include_directories(${PC_GDKPIXBUF_INCLUDE_DIRS})
        pkg_check_modules(lib_gdkpixbuf REQUIRED IMPORTED_TARGET gdk-pixbuf-2.0 )
        pkg_check_modules(lib_notify REQUIRED IMPORTED_TARGET libnotify)
        target_link_libraries(synlib PkgConfig::lib_glib PkgConfig::lib_gdkpixbuf PkgConfig::lib_notify)
    endif()
endif()

if (SYSTEM_PUGIXML)
    target_link_libraries(synlib pugixml)
endif()
