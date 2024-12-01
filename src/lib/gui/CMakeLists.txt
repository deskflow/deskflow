# Deskflow -- mouse and keyboard sharing utility
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

set(target gui)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

file(GLOB_RECURSE sources *.cpp)
file(GLOB_RECURSE headers *.h)

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  file(GLOB mac_sources *.mm)
  list(APPEND sources ${mac_sources})
endif()

# generated includes
include_directories(${PROJECT_BINARY_DIR}/config)

add_library(${target} STATIC ${sources} ${headers})

target_link_libraries(
  ${target}
  ${DESKFLOW_GUI_HOOK_LIB}
  platform
  Qt6::Core
  Qt6::Widgets
  Qt6::Network)

if(WIN32)
  target_link_libraries(
    ${target}
    base
    arch
  )
endif()
