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

if(WIN32)
  file(GLOB headers "MSWindows*.h")
  file(GLOB sources "MSWindows*.cpp")
elseif(APPLE)
  file(GLOB headers "OSX*.h" "IOSX*.h")
  file(
    GLOB
    sources
    "OSX*.cpp"
    "IOSX*.cpp"
    "OSX*.m"
    "OSX*.mm")
elseif(UNIX)
  file(GLOB headers "XWindows*.h")
  file(GLOB sources "XWindows*.cpp")
  # file(GLOB headers "Ei*.h")
  # file(GLOB sources "Ei*.cpp")
  # file(GLOB headers "Portal*.h")
  # file(GLOB sources "Portal*.cpp")
endif()

if(ADD_HEADERS_TO_SOURCES)
  list(APPEND sources ${headers})
endif()

if(APPLE)
  list(APPEND inc /System/Library/Frameworks)
endif()

include_directories(${inc})
add_library(platform STATIC ${sources})
target_link_libraries(platform client ${libs})

if(UNIX)
  target_link_libraries(
    platform
    io
    net
    ipc
    synlib
    client
    ${libs})

  if(NOT APPLE)
    find_package(Qt6 COMPONENTS DBus)
    target_link_libraries(platform Qt6::DBus)

    # Libraries for Wayland support
    target_link_libraries(platform ei portal)
  endif()
endif()

if(APPLE)
  find_library(COCOA_LIBRARY Cocoa)
  target_link_libraries(platform ${COCOA_LIBRARY})
endif()
