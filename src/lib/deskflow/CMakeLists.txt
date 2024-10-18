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

find_package(CLI11 QUIET)
if(CLI11_FOUND)
  message(STATUS "CLI11 [System] Version: ${CLI11_VERSION}")
  set(cli11_lib CLI11::CLI11)
else()
  #Be sure to update the version and MD5 together
  set(CLI11_VERSION 2.4.2)
  set(CLI11_MD5 d7923d1ca06d03e2299e55cad532d126)
  file(
    DOWNLOAD "https://github.com/CLIUtils/CLI11/releases/download/v${CLI11_VERSION}/CLI11.hpp"
    "${CMAKE_BINARY_DIR}/include/CLI/CLI.hpp"
    EXPECTED_MD5 ${CLI11_MD5}
  )
  set(cli11_inc_dir "${CMAKE_BINARY_DIR}/include")
  message(STATUS "CLI11 [Downloaded] Version: ${CLI11_VERSION}")
  message(STATUS "CLI11 INC_DIR: ${cli11_inc_dir}")
endif()

find_package(tomlplusplus QUIET)
if(tomlplusplus_FOUND)
  message(STATUS "tomlplusplus [System] Version: ${tomlplusplus_VERSION}")
  set(tomlPP_lib tomlplusplus::tomlplusplus)
else()
  # Be sure to update the version and MD5 together
  # Save our copy with .h not .hpp for compatibliy with system version < 3.4
  set(TOMLPP_VERSION 3.4.0)
  set(TOMLPP_MD5 caefb3b60119731fb4b4d24b6339e7fb)
  file(
    DOWNLOAD "https://raw.githubusercontent.com/marzer/tomlplusplus/refs/tags/v${TOMLPP_VERSION}/toml.hpp"
    "${CMAKE_BINARY_DIR}/include/toml++/toml.h"
    SHOW_PROGRESS
    EXPECTED_MD5 ${TOMLPP_MD5}
  )
  set(tomlPP_inc_dir "${CMAKE_BINARY_DIR}/include")
  message(STATUS "tomlplusplus [Downloaded] Version: 3.4.0")
  message(STATUS "tomlplusplus INC DIR: ${tomlPP_inc_dir}")
endif()

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

target_include_directories(
  ${lib_name}
    PRIVATE
      ${cli11_inc_dir}
      ${tomlPP_inc_dir}
)

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
    server
    ${cli11_lib}
    ${tomlPP_lib}
    ${wintoast-lib}
  )

  if(NOT APPLE)
    target_link_libraries(${lib_name} pugixml)

    find_package(PkgConfig)
    if(PKG_CONFIG_FOUND)
      target_link_libraries(${lib_name} PkgConfig::lib_glib
                            PkgConfig::lib_gdkpixbuf PkgConfig::lib_notify)
    endif()
  endif()
endif()
