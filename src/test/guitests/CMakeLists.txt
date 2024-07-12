# Synergy -- mouse and keyboard sharing utility
# Copyright (C) 2024 Symless Ltd.
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

set(target guitests)
set(gui_dir "../../../src/gui/src")

file(GLOB_RECURSE test_sources "*.cpp")
file(GLOB_RECURSE gui_sources "${gui_dir}/*.cpp")

file(GLOB activation_sources "${gui_dir}/*Activation*" "${gui_dir}/*License*")
if(NOT ENABLE_LICENSING)
  list(REMOVE_ITEM gui_sources ${activation_sources})
endif()

# Remove main dir as the test already has its own main.
file(GLOB gui_main "${gui_dir}/main.cpp")
list(REMOVE_ITEM gui_sources ${gui_main})

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  file(GLOB mac_gui_sources "${gui_dir}/*.mm")
  list(APPEND gui_sources ${mac_gui_sources})
endif()

include_directories(${gui_dir} ../../)

if(WIN32)
  list(APPEND sources ${CMAKE_BINARY_DIR}/src/version.rc)
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_executable(${target} ${test_sources} ${gui_sources})

find_package(
  Qt6
  COMPONENTS Core5Compat Core Widgets Network Test
  REQUIRED)

target_link_libraries(
  ${target}
  Qt6::Core
  Qt6::Widgets
  Qt6::Network
  Qt6::Core5Compat
  Qt6::Test
  gtest
  gmock
  shared
  ${libs})
