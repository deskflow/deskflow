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

set(target synergy)

find_package(
  Qt6
  COMPONENTS Core Widgets Network
  REQUIRED)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

file(
  GLOB
  GUI_SOURCE_FILES
  src/*.cpp
  src/*.h
  src/validators/*
  src/widgets/*)
file(GLOB GUI_UI_FILES src/*.ui)
file(GLOB ACTIVATION_FILES src/*Activation* src/*License*)

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  file(GLOB GUI_MAC_SOURCE_FILES src/*.mm)
  list(APPEND GUI_SOURCE_FILES ${GUI_MAC_SOURCE_FILES})
endif()

if(NOT ENABLE_LICENSING)
  list(REMOVE_ITEM GUI_SOURCE_FILES ${ACTIVATION_FILES})
  list(REMOVE_ITEM GUI_UI_FILES ${ACTIVATION_FILES})
endif()

if(WIN32)
  set(GUI_RC_FILES res/win/Synergy.rc ${CMAKE_BINARY_DIR}/src/version.rc)
endif()

add_executable(
  ${target} WIN32
  ${GUI_SOURCE_FILES}
  ${GUI_UI_FILES}
  ${GUI_RC_FILES}
  res/Synergy.qrc
  ${QM_FILES})

include_directories(./src)
target_link_libraries(${target} shared)

target_link_libraries(${target} Qt6::Core Qt6::Widgets Qt6::Network)
target_compile_definitions(
  ${target} PRIVATE -DSYNERGY_VERSION_STAGE="${SYNERGY_VERSION_STAGE}")
target_compile_definitions(${target}
                           PRIVATE -DSYNERGY_REVISION="${SYNERGY_REVISION}")

if(WIN32)
  set_target_properties(${target} PROPERTIES LINK_FLAGS "/NODEFAULTLIB:LIBCMT")
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

  find_program(MACDEPLOYQT_BIN macdeployqt6)
  message(STATUS "Found macdeployqt6: ${MACDEPLOYQT_BIN}")

  set(MACDEPLOYQT_CMD
      "${MACDEPLOYQT_BIN} ${SYNERGY_BUNDLE_APP_DIR} -always-overwrite")

  install(TARGETS ${target} DESTINATION ${SYNERGY_BUNDLE_BINARY_DIR})
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
              "$<TARGET_FILE_DIR:synergy>/$<TARGET_FILE_NAME:synergy>")
  endif()

endif()

# post_config()
