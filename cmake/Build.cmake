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

macro(configure_build)

  set(CMAKE_CXX_STANDARD 20)
  set(CMAKE_CXX_EXTENSIONS OFF)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")

  if(APPLE)
    message(STATUS "Configuring for Apple")
    set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0")
  endif()

  set_build_date()
  configure_file_shared()

endmacro()

macro(set_build_date)
  # Since CMake 3.8.0, `string(TIMESTAMP ...)` respects `SOURCE_DATE_EPOCH` env var if set,
  # which allows package maintainers to create reproducible builds.
  # We require CMake 3.8.0 in the root `CMakeLists.txt` for this reason.
  string(TIMESTAMP BUILD_DATE "%Y-%m-%d" UTC)
  message(STATUS "Build date: ${BUILD_DATE}")
  add_definitions(-DBUILD_DATE="${BUILD_DATE}")
endmacro()

macro(configure_file_shared)
  configure_file(${PROJECT_SOURCE_DIR}/src/lib/gui/gui_config.h.in
                 ${PROJECT_BINARY_DIR}/config/gui_config.h)
endmacro()

macro(post_config)

  # Build to a temp bin dir on Windows and then copy to the final bin dir
  # (ignore copy fail). It is neccesary to do this. Since the binary may already
  # be running and you can't write to a running binary (on Windows). It's common
  # to use Deskflow to develop Deskflow (i.e. eating your own dog food immediately
  # making it).
  if(WIN32)

    if(NOT target)
      message(FATAL_ERROR "target not set")
    endif()

    set_target_properties(${target} PROPERTIES RUNTIME_OUTPUT_DIRECTORY
                                               ${BIN_TEMP_DIR})
  endif()

endmacro()

macro(post_config_all)

  if(WIN32)
    # Always try to copy the files to the bin directory after every build and deliberatly ignore
    # copy errors, usually the error is because a running process has locked the file.
    #
    # It is useful to copy every time because the copy may have failed last time due to the file
    # being in use, and we'll usually want to try again (after killing the process).
    #
    # Yes, this looks like a ridiculous thing to do, but it really is necessary to
    # use a Python script to copy files on Windows. Why? Two reasons:
    #
    #   1. Windows file locks (on running processes) creates a very painful development
    #      experience; you can't overwrite the binary you're running it.
    #      Why not just stop the process? Windows services are an abject PITA to manage,
    #      and we don't always care about overwriting all binaries.
    #
    #   2. The Windows copy command is limited and gives vague/misleading errors.
    #      The CMake copy command also has similar shortfalls.
    #
    # Patches welcome! :)
    add_custom_target(
      run_post_build ALL
      COMMAND ${Python_EXECUTABLE} ${PROJECT_SOURCE_DIR}/scripts/fancy_copy.py
              ${BIN_TEMP_DIR} ${PROJECT_BINARY_DIR}/bin --ignore-errors
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      VERBATIM
      COMMENT "Copying files to bin dir")

    add_dependencies(
      run_post_build
      deskflow
      deskflow-client
      deskflow-server
      deskflow-daemon)
  endif()

endmacro()
