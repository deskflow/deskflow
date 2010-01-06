# - Configure a project for testing with CTest/CDash
# Include this module in the top CMakeLists.txt file of a project to
# enable testing with CTest and dashboard submissions to CDash:
#   project(MyProject)
#   ...
#   include(CTest)
# The module automatically creates a BUILD_TESTING option that selects
# whether to enable testing support (ON by default).  After including
# the module, use code like
#   if(BUILD_TESTING)
#     # ... CMake code to create tests ...
#   endif()
# to creating tests when testing is enabled.
#
# To enable submissions to a CDash server, create a CTestConfig.cmake
# file at the top of the project with content such as
#   set(CTEST_PROJECT_NAME "MyProject")
#   set(CTEST_NIGHTLY_START_TIME "01:00:00 UTC")
#   set(CTEST_DROP_METHOD "http")
#   set(CTEST_DROP_SITE "my.cdash.org")
#   set(CTEST_DROP_LOCATION "/submit.php?project=MyProject")
#   set(CTEST_DROP_SITE_CDASH TRUE)
# (the CDash server can provide the file to a project administrator
# who configures 'MyProject').
# Settings in the config file are shared by both this CTest module and
# the CTest command-line tool's dashboard script mode (ctest -S).
#
# While building a project for submission to CDash, CTest scans the
# build output for errors and warnings and reports them with
# surrounding context from the build log.  This generic approach works
# for all build tools, but does not give details about the command
# invocation that produced a given problem.  One may get more detailed
# reports by adding
#   set(CTEST_USE_LAUNCHERS 1)
# to the CTestConfig.cmake file.  When this option is enabled, the
# CTest module tells CMake's Makefile generators to invoke every
# command in the generated build system through a CTest launcher
# program.  (Currently the CTEST_USE_LAUNCHERS option is ignored on
# non-Makefile generators.)  During a manual build each launcher
# transparently runs the command it wraps.  During a CTest-driven
# build for submission to CDash each launcher reports detailed
# information when its command fails or warns.
# (Setting CTEST_USE_LAUNCHERS in CTestConfig.cmake is convenient, but
# also adds the launcher overhead even for manual builds.  One may
# instead set it in a CTest dashboard script and add it to the CMake
# cache for the build tree.)

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

OPTION(BUILD_TESTING "Build the testing tree." ON)

# function to turn generator name into a version string
# like vs7 vs71 vs8 vs9 
FUNCTION(GET_VS_VERSION_STRING generator var)
  STRING(REGEX REPLACE "Visual Studio ([0-9][0-9]?)($|.*)" "\\1" NUMBER "${generator}") 
  IF("${generator}" MATCHES "Visual Studio 7 .NET 2003")
    SET(ver_string "vs71")
  ELSE("${generator}" MATCHES "Visual Studio 7 .NET 2003")
    SET(ver_string "vs${NUMBER}")
  ENDIF("${generator}" MATCHES "Visual Studio 7 .NET 2003")
  SET(${var} ${ver_string} PARENT_SCOPE)
ENDFUNCTION(GET_VS_VERSION_STRING)

IF(BUILD_TESTING)
  # Setup some auxilary macros
  MACRO(SET_IF_NOT_SET var val)
    IF(NOT DEFINED "${var}")
      SET("${var}" "${val}")
    ENDIF(NOT DEFINED "${var}")
  ENDMACRO(SET_IF_NOT_SET)

  MACRO(SET_IF_SET var val)
    IF(NOT "${val}" MATCHES "^$")
      SET("${var}" "${val}")
    ENDIF(NOT "${val}" MATCHES "^$")
  ENDMACRO(SET_IF_SET)

  MACRO(SET_IF_SET_AND_NOT_SET var val)
    IF(NOT "${val}" MATCHES "^$")
      SET_IF_NOT_SET("${var}" "${val}")
    ENDIF(NOT "${val}" MATCHES "^$")
  ENDMACRO(SET_IF_SET_AND_NOT_SET)

  # Make sure testing is enabled
  ENABLE_TESTING()

  IF(EXISTS "${PROJECT_SOURCE_DIR}/CTestConfig.cmake")
    INCLUDE("${PROJECT_SOURCE_DIR}/CTestConfig.cmake")
    SET_IF_SET_AND_NOT_SET(NIGHTLY_START_TIME "${CTEST_NIGHTLY_START_TIME}")
    SET_IF_SET_AND_NOT_SET(DROP_METHOD "${CTEST_DROP_METHOD}")
    SET_IF_SET_AND_NOT_SET(DROP_SITE "${CTEST_DROP_SITE}")
    SET_IF_SET_AND_NOT_SET(DROP_SITE_USER "${CTEST_DROP_SITE_USER}")
    SET_IF_SET_AND_NOT_SET(DROP_SITE_PASSWORD "${CTEST_DROP_SITE_PASWORD}")
    SET_IF_SET_AND_NOT_SET(DROP_SITE_MODE "${CTEST_DROP_SITE_MODE}")
    SET_IF_SET_AND_NOT_SET(DROP_LOCATION "${CTEST_DROP_LOCATION}")
    SET_IF_SET_AND_NOT_SET(TRIGGER_SITE "${CTEST_TRIGGER_SITE}")
    SET_IF_SET_AND_NOT_SET(UPDATE_TYPE "${CTEST_UPDATE_TYPE}")
  ENDIF(EXISTS "${PROJECT_SOURCE_DIR}/CTestConfig.cmake")

  # the project can have a DartConfig.cmake file
  IF(EXISTS "${PROJECT_SOURCE_DIR}/DartConfig.cmake")
    INCLUDE("${PROJECT_SOURCE_DIR}/DartConfig.cmake")
  ELSE(EXISTS "${PROJECT_SOURCE_DIR}/DartConfig.cmake")
    # Dashboard is opened for submissions for a 24 hour period starting at
    # the specified NIGHTLY_START_TIME. Time is specified in 24 hour format.
    SET_IF_NOT_SET (NIGHTLY_START_TIME "00:00:00 EDT")
    SET_IF_NOT_SET(DROP_METHOD "http")
    SET_IF_NOT_SET (COMPRESS_SUBMISSION ON)
  ENDIF(EXISTS "${PROJECT_SOURCE_DIR}/DartConfig.cmake")
  SET_IF_NOT_SET (NIGHTLY_START_TIME "00:00:00 EDT")

  FIND_PROGRAM(CVSCOMMAND cvs )
  SET(CVS_UPDATE_OPTIONS "-d -A -P" CACHE STRING 
    "Options passed to the cvs update command.")
  FIND_PROGRAM(SVNCOMMAND svn)
  FIND_PROGRAM(BZRCOMMAND bzr)
  FIND_PROGRAM(HGCOMMAND hg)

  IF(NOT UPDATE_TYPE)
    IF(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/CVS")
      SET(UPDATE_TYPE cvs)
    ELSE(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/CVS")
      IF(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.svn")
        SET(UPDATE_TYPE svn)
      ELSE(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.svn")
        IF(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.bzr")
          SET(UPDATE_TYPE bzr)
        ELSE(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.bzr")
          IF(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.hg")
            SET(UPDATE_TYPE hg)
          ENDIF(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.hg")
        ENDIF(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.bzr")
      ENDIF(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.svn")
    ENDIF(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/CVS")
  ENDIF(NOT UPDATE_TYPE)

  IF(NOT UPDATE_TYPE)
    IF(NOT __CTEST_UPDATE_TYPE_COMPLAINED)
      SET(__CTEST_UPDATE_TYPE_COMPLAINED 1 CACHE INTERNAL "Already complained about update type.")
      MESSAGE(STATUS "CTest cannot determine repository type. Please set UPDATE_TYPE to 'cvs' or 'svn'. CTest update will not work.")
    ENDIF(NOT __CTEST_UPDATE_TYPE_COMPLAINED)
  ENDIF(NOT UPDATE_TYPE)

  STRING(TOLOWER "${UPDATE_TYPE}" _update_type)
  IF("${_update_type}" STREQUAL "cvs")
    SET(UPDATE_COMMAND "${CVSCOMMAND}")
    SET(UPDATE_OPTIONS "${CVS_UPDATE_OPTIONS}")
  ELSE("${_update_type}" STREQUAL "cvs")
    IF("${_update_type}" STREQUAL "svn")
      SET(UPDATE_COMMAND "${SVNCOMMAND}")
      SET(UPDATE_OPTIONS "${SVN_UPDATE_OPTIONS}")
    ELSE("${_update_type}" STREQUAL "svn")
      IF("${_update_type}" STREQUAL "bzr")
        SET(UPDATE_COMMAND "${BZRCOMMAND}")
        SET(UPDATE_OPTIONS "${BZR_UPDATE_OPTIONS}")
      ELSE("${_update_type}" STREQUAL "bzr")
        IF("${_update_type}" STREQUAL "hg")
          SET(UPDATE_COMMAND "${HGCOMMAND}")
          SET(UPDATE_OPTIONS "${HG_UPDATE_OPTIONS}")
        ENDIF("${_update_type}" STREQUAL "hg")
      ENDIF("${_update_type}" STREQUAL "bzr")
    ENDIF("${_update_type}" STREQUAL "svn")
  ENDIF("${_update_type}" STREQUAL "cvs")

  SET(DART_TESTING_TIMEOUT 1500 CACHE STRING 
    "Maximum time allowed before CTest will kill the test.")

  FIND_PROGRAM(MEMORYCHECK_COMMAND
    NAMES purify valgrind boundscheck
    PATHS
    "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Rational Software\\Purify\\Setup;InstallFolder]"
    DOC "Path to the memory checking command, used for memory error detection."
    )
  FIND_PROGRAM(SLURM_SBATCH_COMMAND sbatch DOC
    "Path to the SLURM sbatch executable"
    )
  FIND_PROGRAM(SLURM_SRUN_COMMAND srun DOC
    "Path to the SLURM srun executable"
    )
  SET(MEMORYCHECK_SUPPRESSIONS_FILE "" CACHE FILEPATH 
    "File that contains suppressions for the memory checker")
  FIND_PROGRAM(SCPCOMMAND scp DOC 
    "Path to scp command, used by CTest for submitting results to a Dart server"
    )
  FIND_PROGRAM(COVERAGE_COMMAND gcov DOC 
    "Path to the coverage program that CTest uses for performing coverage inspection"
    )

  # set the site name
  SITE_NAME(SITE)
  # set the build name
  IF(NOT BUILDNAME)
    SET(DART_COMPILER "${CMAKE_CXX_COMPILER}")
    IF(NOT DART_COMPILER)
      SET(DART_COMPILER "${CMAKE_C_COMPILER}")
    ENDIF(NOT DART_COMPILER)
    IF(NOT DART_COMPILER)
      SET(DART_COMPILER "unknown")
    ENDIF(NOT DART_COMPILER)
    IF(WIN32)
      SET(DART_NAME_COMPONENT "NAME_WE")
    ELSE(WIN32)
      SET(DART_NAME_COMPONENT "NAME")
    ENDIF(WIN32)
    IF(NOT BUILD_NAME_SYSTEM_NAME)
      SET(BUILD_NAME_SYSTEM_NAME "${CMAKE_SYSTEM_NAME}")
    ENDIF(NOT BUILD_NAME_SYSTEM_NAME)
    IF(WIN32)
      SET(BUILD_NAME_SYSTEM_NAME "Win32")
    ENDIF(WIN32)
    IF(UNIX OR BORLAND)
      GET_FILENAME_COMPONENT(DART_CXX_NAME 
        "${CMAKE_CXX_COMPILER}" ${DART_NAME_COMPONENT})
    ELSE(UNIX OR BORLAND)
      GET_FILENAME_COMPONENT(DART_CXX_NAME 
        "${CMAKE_BUILD_TOOL}" ${DART_NAME_COMPONENT})
    ENDIF(UNIX OR BORLAND)
    IF(DART_CXX_NAME MATCHES "msdev")
      SET(DART_CXX_NAME "vs60")
    ENDIF(DART_CXX_NAME MATCHES "msdev")
    IF(DART_CXX_NAME MATCHES "devenv")
      GET_VS_VERSION_STRING("${CMAKE_GENERATOR}" DART_CXX_NAME)
    ENDIF(DART_CXX_NAME MATCHES "devenv")
    SET(BUILDNAME "${BUILD_NAME_SYSTEM_NAME}-${DART_CXX_NAME}")
  ENDIF(NOT BUILDNAME)

  # the build command
  BUILD_COMMAND(MAKECOMMAND CONFIGURATION "\${CTEST_CONFIGURATION_TYPE}")
  SET(MAKECOMMAND ${MAKECOMMAND} CACHE STRING "Command to build the project")

  # the default build configuration the ctest build handler will use
  # if there is no -C arg given to ctest:
  SET(DEFAULT_CTEST_CONFIGURATION_TYPE "$ENV{CMAKE_CONFIG_TYPE}")
  IF(DEFAULT_CTEST_CONFIGURATION_TYPE STREQUAL "")
    SET(DEFAULT_CTEST_CONFIGURATION_TYPE "Release")
  ENDIF(DEFAULT_CTEST_CONFIGURATION_TYPE STREQUAL "")

  IF(NOT "${CMAKE_GENERATOR}" MATCHES "Make")
    SET(CTEST_USE_LAUNCHERS 0)
  ENDIF(NOT "${CMAKE_GENERATOR}" MATCHES "Make")
  IF(CTEST_USE_LAUNCHERS)
    SET(CTEST_LAUNCH_COMPILE "\"${CMAKE_CTEST_COMMAND}\" --launch --target-name <TARGET_NAME> --build-dir <CMAKE_CURRENT_BINARY_DIR> --output <OBJECT> --source <SOURCE> --language <LANGUAGE> --")
    SET(CTEST_LAUNCH_LINK    "\"${CMAKE_CTEST_COMMAND}\" --launch --target-name <TARGET_NAME> --build-dir <CMAKE_CURRENT_BINARY_DIR> --output <TARGET> --target-type <TARGET_TYPE> --language <LANGUAGE> --")
    SET(CTEST_LAUNCH_CUSTOM  "\"${CMAKE_CTEST_COMMAND}\" --launch --target-name <TARGET_NAME> --build-dir <CMAKE_CURRENT_BINARY_DIR> --output <OUTPUT> --")
    SET_PROPERTY(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CTEST_LAUNCH_COMPILE}")
    SET_PROPERTY(GLOBAL PROPERTY RULE_LAUNCH_LINK "${CTEST_LAUNCH_LINK}")
    SET_PROPERTY(GLOBAL PROPERTY RULE_LAUNCH_CUSTOM "${CTEST_LAUNCH_CUSTOM}")
  ENDIF(CTEST_USE_LAUNCHERS)

  MARK_AS_ADVANCED(
    COVERAGE_COMMAND
    CVSCOMMAND
    SVNCOMMAND
    BZRCOMMAND
    HGCOMMAND
    CVS_UPDATE_OPTIONS
    SVN_UPDATE_OPTIONS
    BZR_UPDATE_OPTIONS
    MAKECOMMAND 
    MEMORYCHECK_COMMAND
    MEMORYCHECK_SUPPRESSIONS_FILE
    PURIFYCOMMAND
    SCPCOMMAND
    SLURM_SBATCH_COMMAND
    SLURM_SRUN_COMMAND
    SITE 
    )
  #  BUILDNAME 
  IF(NOT RUN_FROM_DART)
    SET(RUN_FROM_CTEST_OR_DART 1)
    INCLUDE(CTestTargets)
    SET(RUN_FROM_CTEST_OR_DART)
  ENDIF(NOT RUN_FROM_DART)
ENDIF(BUILD_TESTING)
