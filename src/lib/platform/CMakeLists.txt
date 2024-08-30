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
  file(GLOB x_headers "XWindows*.h")
  file(GLOB x_sources "XWindows*.cpp")

  if(LIBEI_FOUND)
    file(GLOB ei_headers "Ei*.h")
    file(GLOB ei_sources "Ei*.cpp")

    # The Portal sources also require EI.
    if(LIBPORTAL_FOUND)
      file(GLOB portal_headers "Portal*.h")
      file(GLOB portal_sources "Portal*.cpp")
    endif()
  endif()

  list(
    APPEND
    headers
    ${x_headers}
    ${ei_headers}
    ${portal_headers})
  list(
    APPEND
    sources
    ${x_sources}
    ${ei_sources}
    ${portal_sources})
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

macro(link_wayland_libs)
  target_link_libraries(platform ${LIBXKBCOMMON_LINK_LIBRARIES}
                        ${GLIB2_LINK_LIBRARIES} ${LIBM_LIBRARIES})

  if(LIBEI_FOUND)
    target_link_libraries(platform ${LIBEI_LINK_LIBRARIES})
  endif()

  if(LIBPORTAL_FOUND)
    target_link_libraries(platform ${LIBPORTAL_LINK_LIBRARIES})
  endif()
endmacro()

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

    link_wayland_libs()
  endif()
endif()

if(APPLE)
  find_library(COCOA_LIBRARY Cocoa)
  target_link_libraries(platform ${COCOA_LIBRARY})
endif()
