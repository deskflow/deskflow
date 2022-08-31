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
find_package(Qt5 COMPONENTS Core Widgets Network)
find_package(httplib REQUIRED)
set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

file(GLOB headers "*.h")
file(GLOB sources "*.cpp")

if(SYNERGY_ADD_HEADERS)
  list(APPEND sources ${headers})
endif()

add_library(server STATIC ${sources})

target_link_libraries(server shared httplib::httplib Qt5::Core Qt5::Network Qt5::Widgets)

if(UNIX)
  target_link_libraries(server synlib)
endif()
