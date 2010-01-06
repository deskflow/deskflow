# - Find CABLE
# This module finds if CABLE is installed and determines where the
# include files and libraries are.  This code sets the following variables:
#
#  CABLE             the path to the cable executable
#  CABLE_TCL_LIBRARY the path to the Tcl wrapper library
#  CABLE_INCLUDE_DIR the path to the include directory
#
# To build Tcl wrappers, you should add shared library and link it to
# ${CABLE_TCL_LIBRARY}.  You should also add ${CABLE_INCLUDE_DIR} as
# an include directory.

#=============================================================================
# Copyright 2001-2009 Kitware, Inc.
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distributed this file outside of CMake, substitute the full
#  License text for the above reference.)

IF(NOT CABLE)
  FIND_PATH(CABLE_BUILD_DIR cableVersion.h)
ENDIF(NOT CABLE)

IF(CABLE_BUILD_DIR)
  LOAD_CACHE(${CABLE_BUILD_DIR}
             EXCLUDE
               BUILD_SHARED_LIBS
               LIBRARY_OUTPUT_PATH
               EXECUTABLE_OUTPUT_PATH
               MAKECOMMAND
               CMAKE_INSTALL_PREFIX
             INCLUDE_INTERNALS
               CABLE_LIBRARY_PATH
               CABLE_EXECUTABLE_PATH)

  IF(CABLE_LIBRARY_PATH)
    FIND_LIBRARY(CABLE_TCL_LIBRARY NAMES CableTclFacility PATHS
                 ${CABLE_LIBRARY_PATH}
                 ${CABLE_LIBRARY_PATH}/*)
  ELSE(CABLE_LIBRARY_PATH)
    FIND_LIBRARY(CABLE_TCL_LIBRARY NAMES CableTclFacility PATHS
                 ${CABLE_BINARY_DIR}/CableTclFacility
                 ${CABLE_BINARY_DIR}/CableTclFacility/*)
  ENDIF(CABLE_LIBRARY_PATH)

  IF(CABLE_EXECUTABLE_PATH)
    FIND_PROGRAM(CABLE NAMES cable PATHS
                 ${CABLE_EXECUTABLE_PATH}
                 ${CABLE_EXECUTABLE_PATH}/*)
  ELSE(CABLE_EXECUTABLE_PATH)
    FIND_PROGRAM(CABLE NAMES cable PATHS
                 ${CABLE_BINARY_DIR}/Executables
                 ${CABLE_BINARY_DIR}/Executables/*)
  ENDIF(CABLE_EXECUTABLE_PATH)

  FIND_PATH(CABLE_INCLUDE_DIR CableTclFacility/ctCalls.h
            ${CABLE_SOURCE_DIR})
ELSE(CABLE_BUILD_DIR)
  # Find the cable executable in the path.
  FIND_PROGRAM(CABLE NAMES cable)

  # Get the path where the executable sits, but without the executable
  # name on it.
  GET_FILENAME_COMPONENT(CABLE_ROOT_BIN ${CABLE} PATH)

  # Find the cable include directory in a path relative to the cable
  # executable.
  FIND_PATH(CABLE_INCLUDE_DIR CableTclFacility/ctCalls.h
            ${CABLE_ROOT_BIN}/../include/Cable)

  # Find the WrapTclFacility library in a path relative to the cable
  # executable.
  FIND_LIBRARY(CABLE_TCL_LIBRARY NAMES CableTclFacility PATHS
               ${CABLE_ROOT_BIN}/../lib/Cable)
ENDIF(CABLE_BUILD_DIR)
