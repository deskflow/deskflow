
#=============================================================================
# Copyright 2005-2009 Kitware, Inc.
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

IF(NOT RUN_FROM_CTEST_OR_DART)
  MESSAGE(FATAL_ERROR "Do not incldue CTestTargets.cmake directly")
ENDIF(NOT RUN_FROM_CTEST_OR_DART)

# make directories in the binary tree
FILE(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/Testing/Temporary)
GET_FILENAME_COMPONENT(CMAKE_HOST_PATH ${CMAKE_COMMAND} PATH)
SET(CMAKE_TARGET_PATH ${EXECUTABLE_OUTPUT_PATH})
FIND_PROGRAM(CMAKE_CTEST_COMMAND ctest ${CMAKE_HOST_PATH} ${CMAKE_TARGET_PATH})
MARK_AS_ADVANCED(CMAKE_CTEST_COMMAND)

# Use CTest
# configure files

IF(CTEST_NEW_FORMAT)
  CONFIGURE_FILE(
    ${CMAKE_ROOT}/Modules/DartConfiguration.tcl.in
    ${PROJECT_BINARY_DIR}/CTestConfiguration.ini )
ELSE(CTEST_NEW_FORMAT)
  CONFIGURE_FILE(
    ${CMAKE_ROOT}/Modules/DartConfiguration.tcl.in
    ${PROJECT_BINARY_DIR}/DartConfiguration.tcl )
ENDIF(CTEST_NEW_FORMAT)

#
# Section 3:
#
# Custom targets to perform dashboard builds and submissions.
# These should NOT need to be modified from project to project.
#

SET(__conf_types "")
IF(CMAKE_CONFIGURATION_TYPES)
  # We need to pass the configuration type on the test command line.
  SET(__conf_types -C "${CMAKE_CFG_INTDIR}")
ENDIF(CMAKE_CONFIGURATION_TYPES)

# Add convenience targets.  Do this at most once in case of nested
# projects.
DEFINE_PROPERTY(GLOBAL PROPERTY CTEST_TARGETS_ADDED
  BRIEF_DOCS "Internal property used by CTestTargets module."
  FULL_DOCS "Set by the CTestTargets module to track addition of testing targets."
  )
GET_PROPERTY(_CTEST_TARGETS_ADDED GLOBAL PROPERTY CTEST_TARGETS_ADDED)
IF(NOT _CTEST_TARGETS_ADDED)
  SET_PROPERTY(GLOBAL PROPERTY CTEST_TARGETS_ADDED 1)

  # For all generators add basic testing targets.
  FOREACH(mode Experimental Nightly Continuous NightlyMemoryCheck)
    ADD_CUSTOM_TARGET(${mode}
      ${CMAKE_CTEST_COMMAND} ${__conf_types} -D ${mode}
      )
    SET_PROPERTY(TARGET ${mode} PROPERTY RULE_LAUNCH_CUSTOM "")
  ENDFOREACH(mode)

  # For Makefile generators add more granular targets.
  IF("${CMAKE_GENERATOR}" MATCHES Make)
    # Make targets for Experimental builds
    FOREACH(mode Nightly Experimental Continuous)
      FOREACH(testtype
          Start Update Configure Build Test Coverage MemCheck Submit
          # missing purify
          )
        ADD_CUSTOM_TARGET(${mode}${testtype}
          ${CMAKE_CTEST_COMMAND} ${__conf_types} -D ${mode}${testtype}
          )
        SET_PROPERTY(TARGET ${mode}${testtype} PROPERTY RULE_LAUNCH_CUSTOM "")
      ENDFOREACH(testtype)
    ENDFOREACH(mode)
  ENDIF("${CMAKE_GENERATOR}" MATCHES Make)
ENDIF(NOT _CTEST_TARGETS_ADDED)
