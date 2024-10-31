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

set(target ${GUI_BINARY_NAME})

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(res_dir ${GUI_RES_DIR})
set(qrc_file ${GUI_QRC_FILE})

file(
  GLOB
  sources
  src/*.cpp
  src/*.h
  src/dialogs/*.h
  src/dialogs/*.cpp
  src/validators/*
  src/widgets/*)
file(GLOB ui_files src/*.ui src/dialogs/*.ui)

if(WIN32)
  set(rc_files ${res_dir}/win/app.rc ${PROJECT_BINARY_DIR}/src/version.rc)
endif()

# regular exe headers
include_directories(./src)

# gui library autogen headers:
# qt doesn't seem to auto include the autogen headers for libraries.
include_directories(${PROJECT_BINARY_DIR}/src/lib/gui/gui_autogen/include)

# generated includes
include_directories(${PROJECT_BINARY_DIR}/config)

add_executable(${target} WIN32 ${sources} ${ui_files} ${rc_files} ${qrc_file})

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
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

  find_program(MACDEPLOYQT_BIN macdeployqt6)
  message(STATUS "Found macdeployqt6: ${MACDEPLOYQT_BIN}")

  set(MACDEPLOYQT_CMD
      "${MACDEPLOYQT_BIN} ${DESKFLOW_BUNDLE_DIR} -always-overwrite")

  install(TARGETS ${target} DESTINATION ${DESKFLOW_BUNDLE_BINARY_DIR})
  install(CODE "MESSAGE (\"Running: ${MACDEPLOYQT_CMD}\")")
  install(CODE "execute_process(COMMAND ${MACDEPLOYQT_CMD})")

elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

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
