# Deskflow -- mouse and keyboard sharing utility
# Copyright (C) 2012-2024 Symless Ltd.
# Copyright (C) 2009-2012 Nick Bolton
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

set(lib_name app)

file(GLOB headers "*.h" "languages/*.h")
file(GLOB sources "*.cpp" "languages/*.cpp")

# arch
if(WIN32)
  file(GLOB arch_headers "win32/*.h")
  file(GLOB arch_sources "win32/*.cpp")

  if(HAVE_WINTOAST)
    list(APPEND arch_sources ${WINTOAST_DIR}/src/wintoastlib.cpp)
  endif()

elseif(UNIX)
  file(GLOB arch_headers "unix/*.h")
  file(GLOB arch_sources "unix/*.cpp")
endif()

list(APPEND sources ${arch_sources})
list(APPEND headers ${arch_headers})

if(ADD_HEADERS_TO_SOURCES)
  list(APPEND sources ${headers})
endif()

add_library(${lib_name} STATIC ${sources})

if(UNIX)
  target_link_libraries(
    ${lib_name}
    arch
    client
    ipc
    net
    base
    platform
    mt
    server)

  if(CLI11_FOUND)
    target_link_libraries(${lib_name} CLI11::CLI11)
  endif()

  if(tomlplusplus_FOUND)
    target_link_libraries(${lib_name} tomlplusplus::tomlplusplus)
  endif()

  if(NOT APPLE)
    target_link_libraries(${lib_name} pugixml)

    find_package(PkgConfig)
    if(PKG_CONFIG_FOUND)
      target_link_libraries(${lib_name} PkgConfig::lib_glib
                            PkgConfig::lib_gdkpixbuf PkgConfig::lib_notify)
    endif()
  endif()
endif()
