# Synergy -- mouse and keyboard sharing utility
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

set(target unittests)

file(GLOB_RECURSE headers "*.h" "languages/*.h")
file(GLOB_RECURSE sources "*.cpp" "languages/*.cpp")

file(GLOB_RECURSE remove_platform "platform/*")
list(REMOVE_ITEM headers ${remove_platform})
list(REMOVE_ITEM sources ${remove_platform})

file(GLOB_RECURSE global_headers "../../test/global/*.h")
file(GLOB_RECURSE global_sources "../../test/global/*.cpp")

list(APPEND headers ${global_headers})
list(APPEND sources ${global_sources})

file(GLOB_RECURSE mock_headers "../../test/mock/*.h")
file(GLOB_RECURSE mock_sources "../../test/mock/*.cpp")

list(APPEND headers ${mock_headers})
list(APPEND sources ${mock_sources})

set(gui_dir "../../gui/src")
file(GLOB_RECURSE gui_sources "${gui_dir}/*.cpp")

file(GLOB activation_sources "${gui_dir}/*Activation*" "${gui_dir}/*License*")
if(NOT ENABLE_LICENSING)
  list(REMOVE_ITEM gui_sources ${activation_sources})
endif()

# remove main gui as the test already has its own main.
file(GLOB gui_main "${gui_dir}/main.cpp")
list(REMOVE_ITEM gui_sources ${gui_main})

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  file(GLOB mac_gui_sources "${gui_dir}/*.mm")
  list(APPEND gui_sources ${mac_gui_sources})
endif()

list(APPEND sources ${gui_sources})

# platform
if(WIN32)
  file(GLOB platform_sources "platform/MSWindows*.cpp")
  file(GLOB platform_headers "platform/MSWindows*.h")
elseif(APPLE)
  file(GLOB platform_sources "platform/OSX*.cpp")
  file(GLOB platform_headers "platform/OSX*.h")
elseif(UNIX)
  file(GLOB platform_sources "platform/XWindows*.cpp")
  file(GLOB platform_headers "platform/XWindows*.h")
endif()

list(APPEND sources ${platform_sources})
list(APPEND headers ${platform_headers})

set(ext_dir "../../../ext")

include_directories("../.." "../../lib" ${ext_dir} ${gui_dir}
                    "${ext_dir}/gtest/include" "${ext_dir}/gmock/include")

if(SYNERGY_ADD_HEADERS)
  list(APPEND sources ${headers})
endif()

if(WIN32)
  list(APPEND sources ${CMAKE_BINARY_DIR}/src/version.rc)
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(
  Qt6
  COMPONENTS Core5Compat Core Widgets Network Test
  REQUIRED)

add_executable(${target} ${sources})

target_link_libraries(
  ${target}
  arch
  base
  client
  server
  common
  io
  net
  platform
  server
  synlib
  mt
  ipc
  gtest
  gmock
  shared
  Qt6::Core
  Qt6::Widgets
  Qt6::Network
  Qt6::Core5Compat
  Qt6::Test
  ${libs})
