# - Configure a project for testing with CTest or old Dart Tcl Client
# This file is the backwards-compatibility version of the CTest module.
# It supports using the old Dart 1 Tcl client for driving dashboard
# submissions as well as testing with CTest.  This module should be included
# in the CMakeLists.txt file at the top of a project.  Typical usage:
#  INCLUDE(Dart)
#  IF(BUILD_TESTING)
#    # ... testing related CMake code ...
#  ENDIF(BUILD_TESTING)
# The BUILD_TESTING option is created by the Dart module to determine
# whether testing support should be enabled.  The default is ON.

# This file configures a project to use the Dart testing/dashboard process.
# It is broken into 3 sections.
#
#  Section #1: Locate programs on the client and determine site and build name
#  Section #2: Configure or copy Tcl scripts from the source tree to build tree
#  Section #3: Custom targets for performing dashboard builds.
#
#

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

OPTION(BUILD_TESTING "Build the testing tree." ON)

IF(BUILD_TESTING)
  FIND_PACKAGE(Dart QUIET)

  #
  # Section #1:
  #
  # CMake commands that will not vary from project to project. Locates programs
  # on the client and configure site name and build name.
  #

  SET(RUN_FROM_DART 1)
  INCLUDE(CTest)
  SET(RUN_FROM_DART)

  FIND_PROGRAM(COMPRESSIONCOMMAND NAMES gzip compress zip 
    DOC "Path to program used to compress files for transfer to the dart server")
  FIND_PROGRAM(GUNZIPCOMMAND gunzip DOC "Path to gunzip executable")
  FIND_PROGRAM(JAVACOMMAND java DOC "Path to java command, used by the Dart server to create html.")
  OPTION(DART_VERBOSE_BUILD "Show the actual output of the build, or if off show a . for each 1024 bytes." 
    OFF)
  OPTION(DART_BUILD_ERROR_REPORT_LIMIT "Limit of reported errors, -1 reports all." -1 )  
  OPTION(DART_BUILD_WARNING_REPORT_LIMIT "Limit of reported warnings, -1 reports all." -1 )  

  SET(VERBOSE_BUILD ${DART_VERBOSE_BUILD})
  SET(BUILD_ERROR_REPORT_LIMIT ${DART_BUILD_ERROR_REPORT_LIMIT})
  SET(BUILD_WARNING_REPORT_LIMIT ${DART_BUILD_WARNING_REPORT_LIMIT})
  SET (DELIVER_CONTINUOUS_EMAIL "Off" CACHE BOOL "Should Dart server send email when build errors are found in Continuous builds?")

  MARK_AS_ADVANCED(
    COMPRESSIONCOMMAND
    DART_BUILD_ERROR_REPORT_LIMIT     
    DART_BUILD_WARNING_REPORT_LIMIT 
    DART_TESTING_TIMEOUT
    DART_VERBOSE_BUILD
    DELIVER_CONTINUOUS_EMAIL
    GUNZIPCOMMAND
    JAVACOMMAND 
    )

  SET(HAVE_DART)
  IF(EXISTS "${DART_ROOT}/Source/Client/Dart.conf.in")
    SET(HAVE_DART 1)
  ENDIF(EXISTS "${DART_ROOT}/Source/Client/Dart.conf.in")

  #
  # Section #2:
  # 
  # Make necessary directories and configure testing scripts
  #
  # find a tcl shell command
  IF(HAVE_DART)
    FIND_PACKAGE(Tclsh)
  ENDIF(HAVE_DART)


  IF (HAVE_DART)
    # make directories in the binary tree
    FILE(MAKE_DIRECTORY "${PROJECT_BINARY_DIR}/Testing/HTML/TestingResults/Dashboard"
      "${PROJECT_BINARY_DIR}/Testing/HTML/TestingResults/Sites/${SITE}/${BUILDNAME}")

    # configure files
    CONFIGURE_FILE(
      "${DART_ROOT}/Source/Client/Dart.conf.in"
      "${PROJECT_BINARY_DIR}/DartConfiguration.tcl" )

    #
    # Section 3:
    #
    # Custom targets to perform dashboard builds and submissions.
    # These should NOT need to be modified from project to project.
    #

    # add testing targets
    SET(DART_EXPERIMENTAL_NAME Experimental)
    IF(DART_EXPERIMENTAL_USE_PROJECT_NAME)
      SET(DART_EXPERIMENTAL_NAME "${DART_EXPERIMENTAL_NAME}${PROJECT_NAME}")
    ENDIF(DART_EXPERIMENTAL_USE_PROJECT_NAME)
  ENDIF (HAVE_DART)
  
  SET(RUN_FROM_CTEST_OR_DART 1)
  INCLUDE(CTestTargets)
  SET(RUN_FROM_CTEST_OR_DART)
ENDIF(BUILD_TESTING)

#
# End of Dart.cmake
#

