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

if(WIN32)
  message(STATUS "Enabling warnings as errors (MSVC)")
  add_compile_options(/WX)
elseif(UNIX)
  message(STATUS "Enabling warnings as errors (GNU/Clang)")
  add_compile_options(-Werror)
endif()

macro(post_config)

  # Build to a temp bin dir on Windows and then copy to the final bin dir
  # (ignore copy fail). It is neccesary to do this. Since the binary may already
  # be running and you can't write to a running binary (on Windows). It's common
  # to use Synergy to develop Synergy (i.e. eating your own dog food immediately
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

  # Always try to copy the files to the bin directory after every build, even if
  # there was nothing to do. This is because the copy may have failed last time
  # due to the file being in use, and we'll usually want to try again.
  if(WIN32)

    if(NOT EXISTS ${PYTHON_BIN})
      message(FATAL_ERROR "Python not found at: ${PYTHON_BIN}")
    endif()

    add_custom_target(
      run_post_build ALL
      COMMAND ${PYTHON_BIN} ${CMAKE_SOURCE_DIR}/scripts/fancy_copy.py
              ${BIN_TEMP_DIR} ${CMAKE_BINARY_DIR}/bin --ignore-errors
      VERBATIM
      COMMENT "Copying files to bin dir")

    if(BUILD_UNIFIED)
      add_dependencies(run_post_build synergy synergy-core synergyd)
    else()
      add_dependencies(
        run_post_build
        synergy
        synergyc
        synergys
        synergyd)
    endif()

  endif()

endmacro()
