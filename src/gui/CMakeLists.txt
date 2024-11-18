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
  res/deskflow.qrc
  src/*.cpp
  src/*.h
  src/dialogs/*.h
  src/dialogs/*.cpp
  src/validators/*
  src/widgets/*)
file(GLOB ui_files src/*.ui src/dialogs/*.ui)

if(WIN32)
  set(platform_extra src/deskflow.rc ${PROJECT_BINARY_DIR}/src/version.rc)
elseif(APPLE)
  set(platform_extra src/Deskflow.icns)
  set_source_files_properties(${platform_extra} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
endif()

# regular exe headers
include_directories(./src)

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

target_compile_definitions(
  ${target} PRIVATE -DDESKFLOW_VERSION_STAGE="${DESKFLOW_VERSION_STAGE}")
target_compile_definitions(${target}
                           PRIVATE -DDESKFLOW_REVISION="${DESKFLOW_REVISION}")

if(WIN32)
  set_target_properties(${target} PROPERTIES LINK_FLAGS "/NODEFAULTLIB:LIBCMT")
elseif(APPLE)
  set_target_properties(${target} PROPERTIES
    MACOSX_BUNDLE_BUNDLE_NAME "Deskflow"
    MACOSX_BUNDLE_DISPLAY_NAME "Deskflow"
    MACOSX_BUNDLE_GUI_IDENTIFIER "org.deskflow.deskflow"
    MACOSX_BUNDLE_ICON_FILE Deskflow.icns
    MACOSX_BUNDLE_INFO_STRING "${CMAKE_PROJECT_DESCRIPTION}"
    MACOSX_BUNDLE_COPYRIGHT "© 2024 Deskflow Developers"
    MACOSX_BUNDLE_BUNDLE_VERSION ${DESKFLOW_VERSION}
    MACOSX_BUNDLE_LONG_VERSION_STRING ${DESKFLOW_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${DESKFLOW_VERSION}
  )
  find_program(MACDEPLOYQT_BIN macdeployqt6)
  add_custom_command(
    TARGET ${target} POST_BUILD
    COMMAND ${MACDEPLOYQT_BIN} "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${target}.app"
  )
  install(TARGETS ${target} BUNDLE DESTINATION .)
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

  install(TARGETS ${target} DESTINATION bin)

elseif(WIN32)

  if(Qt6_FOUND
     AND WIN32
     AND TARGET Qt6::qmake
     AND NOT TARGET Qt6::windeployqt)
    get_target_property(_qt6_qmake_location Qt6::qmake IMPORTED_LOCATION)

    execute_process(
      COMMAND "${_qt6_qmake_location}" -query QT_INSTALL_PREFIX
      RESULT_VARIABLE return_code
      OUTPUT_VARIABLE qt6_install_prefix
      OUTPUT_STRIP_TRAILING_WHITESPACE)

    set(imported_location "${qt6_install_prefix}/bin/windeployqt.exe")

    if(EXISTS ${imported_location})
      add_executable(Qt6::windeployqt IMPORTED)

      set_target_properties(Qt6::windeployqt PROPERTIES IMPORTED_LOCATION
                                                        ${imported_location})
    endif()
  endif()

  if(TARGET Qt6::windeployqt)
    # execute windeployqt in a tmp directory after build
    add_custom_command(
      TARGET ${target}
      POST_BUILD
      COMMAND set PATH=%PATH%$<SEMICOLON>${qt6_install_prefix}/bin
      COMMAND Qt6::windeployqt
              "$<TARGET_FILE_DIR:${target}>/$<TARGET_FILE_NAME:${target}>")
  endif()

endif()

post_config()
