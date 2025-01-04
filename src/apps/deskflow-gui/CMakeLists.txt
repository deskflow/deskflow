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

if(APPLE)
  set(target Deskflow)
else()
  set(target deskflow)
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

file(
  GLOB
  sources
  ../res/deskflow.qrc
  *.cpp
  *.h
  dialogs/*.h
  dialogs/*.cpp
  validators/*
  widgets/*)
file(GLOB ui_files *.ui dialogs/*.ui)

if(WIN32)
  set(platform_extra deskflow.rc ${PROJECT_BINARY_DIR}/src/version.rc)
elseif(APPLE)
  set(platform_extra Deskflow.icns ${PROJECT_SOURCE_DIR}/LICENSE ${PROJECT_SOURCE_DIR}/LICENSE_EXCEPTION)
  set_source_files_properties(${platform_extra} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
endif()

# gui library autogen headers:
# qt doesn't seem to auto include the autogen headers for libraries.
include_directories(${PROJECT_BINARY_DIR}/src/lib/gui/gui_autogen/include)

# generated includes
include_directories(${PROJECT_BINARY_DIR}/config)

add_executable(${target} WIN32 MACOSX_BUNDLE ${sources} ${ui_files} ${platform_extra})

target_link_libraries(
  ${target}
  ${DESKFLOW_GUI_HOOK_LIB}
  gui
  Qt6::Core
  Qt6::Widgets
  Qt6::Network)

if(WIN32)
  set_target_properties(${target} PROPERTIES LINK_FLAGS "/NODEFAULTLIB:LIBCMT")
  install(
    TARGETS ${target}
    RUNTIME_DEPENDENCY_SET guiDeps
    DESTINATION .
  )
  install(RUNTIME_DEPENDENCY_SET guiDeps
    PRE_EXCLUDE_REGEXES
      "api-ms-win-.*"
      "ext-ms-.*"
      "^hvsifiletrust\\.dll$"
    POST_EXCLUDE_REGEXES
      ".*system32.*"
    RUNTIME DESTINATION .
  )
  find_program(DEPLOYQT windeployqt6)
  add_custom_command(
    TARGET ${target} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/qtDeploy
      COMMAND ${DEPLOYQT} --no-compiler-runtime --no-system-d3d-compiler --no-quick-import -network --dir ${CMAKE_BINARY_DIR}/qtDeploy $<TARGET_FILE:${target}>
  )
  install(
    DIRECTORY ${CMAKE_BINARY_DIR}/qtDeploy/
    DESTINATION .
    FILES_MATCHING PATTERN "*.*"
  )
elseif(APPLE)
  set_target_properties(${target} PROPERTIES
    INSTALL_RPATH "@loader_path/../Libraries;@loader_path/../Frameworks"
    MACOSX_BUNDLE_BUNDLE_NAME "Deskflow"
    MACOSX_BUNDLE_DISPLAY_NAME "Deskflow"
    MACOSX_BUNDLE_GUI_IDENTIFIER "org.deskflow.deskflow"
    MACOSX_BUNDLE_ICON_FILE Deskflow.icns
    MACOSX_BUNDLE_INFO_STRING "${CMAKE_PROJECT_DESCRIPTION}"
    MACOSX_BUNDLE_COPYRIGHT "© 2024 Deskflow Developers"
    MACOSX_BUNDLE_BUNDLE_VERSION ${CMAKE_PROJECT_VERSION}
    MACOSX_BUNDLE_LONG_VERSION_STRING ${CMAKE_PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${CMAKE_PROJECT_VERSION}
  )
  install(TARGETS ${target} BUNDLE DESTINATION .)
else()
  install(TARGETS ${target} DESTINATION bin)
endif()
