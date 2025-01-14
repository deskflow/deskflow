# Deskflow -- mouse and keyboard sharing utility
# Copyright (C) 2024 - 2025 Chris Rizzitello <sithlord48@gmail.com>
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

#setup our bundle plist file
set(BUNDLE_EXECUTABLE_NAME "${target}")
set(BUNDLE_BUNDLE_NAME "${target}")
set(BUNDLE_DISPLAY_NAME "${target}")
set(BUNDLE_GUI_IDENTIFIER "org.deskflow.deskflow")
set(BUNDLE_ICON_FILE ${target}.icns)
set(BUNDLE_INFO_STRING "${CMAKE_PROJECT_DESCRIPTION}")
set(BUNDLE_COPYRIGHT "${CMAKE_PROJECT_COPYRIGHT}")

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/../res/deskflow.plist.in
  ${CMAKE_CURRENT_BINARY_DIR}/deskflow.plist
  @ONLY
)

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
  set(EXE_DESCRIPTION "${CMAKE_PROJECT_DESCRIPTION}\\n Control and configure deskflow")
  set(EXE_ICON "IDI_ICON1 ICON DISCARDABLE \"${CMAKE_SOURCE_DIR}/src/apps/res/deskflow.ico\" ")
  configure_file(${CMAKE_SOURCE_DIR}/src/apps/res/rc_template.rc.in deskflow.rc)
  set(platform_extra deskflow.rc)
elseif(APPLE)
  set(platform_extra ../res/Deskflow.icns ${PROJECT_SOURCE_DIR}/LICENSE ${PROJECT_SOURCE_DIR}/LICENSE_EXCEPTION)
  set_source_files_properties(${platform_extra} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
endif()

add_executable(${target} WIN32 MACOSX_BUNDLE ${sources} ${ui_files} ${platform_extra})

target_link_libraries(
  ${target}
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

  add_custom_command(
    TARGET ${target} POST_BUILD
      COMMAND ${DEPLOYQT} --no-compiler-runtime --no-system-d3d-compiler --no-quick-import -network $<TARGET_FILE:${target}>
  )
elseif(APPLE)
  set_target_properties(${target} PROPERTIES
    INSTALL_RPATH "@loader_path/../Libraries;@loader_path/../Frameworks"
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_BINARY_DIR}/deskflow.plist"
  )
  install(TARGETS ${target} BUNDLE DESTINATION .)
else()
  install(TARGETS ${target} DESTINATION bin)
endif()
