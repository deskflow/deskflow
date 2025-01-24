# SOURCE https://github.com/bilke/cmake-modules/blob/master/CodeCoverage.cmake
# SPDX-FileCopyrightText: 2020 Bob Apthrope
# SPDX-FileCopyrightText: 2019 Anatolii Kurotych
# SPDX-FileCopyrightText: 2019 - 2020 Frank Dana
# SPDX-FileCopyrightText: 2013 Joakim Söderberg
# SPDX-FileCopyrightText: 2012 - 2017 Lars Bilke
# SPDX-License-Identifier: MIT

# USAGE:
#
# 1. Copy this file into your cmake modules path.
#
# 2. Add the following line to your CMakeLists.txt (best inside an if-condition
#    using a CMake option() to enable it just optionally):
#      include(CodeCoverage)
#
# 3. Append necessary compiler flags:
#      append_coverage_compiler_flags()
#
# 3.a (OPTIONAL) Set appropriate optimization flags, e.g. -O0, -O1 or -Og
#
# 4. If you need to exclude additional directories from the report, specify them
#    using full paths in the COVERAGE_EXCLUDES variable before calling
#    setup_target_for_coverage_*().
#    Example:
#      set(COVERAGE_EXCLUDES
#          '${PROJECT_SOURCE_DIR}/src/dir1/*'
#          '/path/to/my/src/dir2/*')
#    Or, use the EXCLUDE argument to setup_target_for_coverage_*().
#    Example:
#      setup_target_for_coverage_lcov(
#          NAME coverage
#          EXECUTABLE testrunner
#          EXCLUDE "${PROJECT_SOURCE_DIR}/src/dir1/*" "/path/to/my/src/dir2/*")
#
# 4.a NOTE: With CMake 3.4+, COVERAGE_EXCLUDES or EXCLUDE can also be set
#     relative to the BASE_DIRECTORY (default: PROJECT_SOURCE_DIR)
#     Example:
#       set(COVERAGE_EXCLUDES "dir1/*")
#       setup_target_for_coverage_gcovr_html(
#           NAME coverage
#           EXECUTABLE testrunner
#           BASE_DIRECTORY "${PROJECT_SOURCE_DIR}/src"
#           EXCLUDE "dir2/*")
#
# 5. Use the functions described below to create a custom make target which
#    runs your test executable and produces a code coverage report.
#
# 6. Build a Debug build:
#      cmake -DCMAKE_BUILD_TYPE=Debug ..
#      make
#      make my_coverage_target
#

include(CMakeParseArguments)

# Check prereqs
find_program(GCOV_PATH gcov)
find_program(LCOV_PATH NAMES lcov lcov.bat lcov.exe lcov.perl)
find_program(GENHTML_PATH NAMES genhtml genhtml.perl genhtml.bat)
find_program(GCOVR_PATH gcovr PATHS ${CMAKE_SOURCE_DIR}/scripts/test)
find_program(CPPFILT_PATH NAMES c++filt)

if(NOT GCOV_PATH)
  message(FATAL_ERROR "gcov not found! Aborting...")
endif() # NOT GCOV_PATH

if("${CMAKE_CXX_COMPILER_ID}" MATCHES "(Apple)?[Cc]lang")
  if("${CMAKE_CXX_COMPILER_VERSION}" VERSION_LESS 3)
    message(FATAL_ERROR "Clang version must be 3.0.0 or greater! Aborting...")
  endif()
elseif(NOT CMAKE_COMPILER_IS_GNUCXX)
  if("${CMAKE_Fortran_COMPILER_ID}" MATCHES "[Ff]lang")
    # Do nothing; exit conditional without error if true
  elseif("${CMAKE_Fortran_COMPILER_ID}" MATCHES "GNU")
    # Do nothing; exit conditional without error if true
  else()
    message(FATAL_ERROR "Compiler is not GNU gcc! Aborting...")
  endif()
endif()

set(COVERAGE_COMPILER_FLAGS
    "-g -fprofile-arcs -ftest-coverage"
    CACHE INTERNAL "")

set(CMAKE_Fortran_FLAGS_COVERAGE
    ${COVERAGE_COMPILER_FLAGS}
    CACHE STRING "Flags used by the Fortran compiler during coverage builds."
          FORCE)
set(CMAKE_CXX_FLAGS_COVERAGE
    ${COVERAGE_COMPILER_FLAGS}
    CACHE STRING "Flags used by the C++ compiler during coverage builds." FORCE)
set(CMAKE_C_FLAGS_COVERAGE
    ${COVERAGE_COMPILER_FLAGS}
    CACHE STRING "Flags used by the C compiler during coverage builds." FORCE)
set(CMAKE_EXE_LINKER_FLAGS_COVERAGE
    ""
    CACHE STRING "Flags used for linking binaries during coverage builds."
          FORCE)
set(CMAKE_SHARED_LINKER_FLAGS_COVERAGE
    ""
    CACHE STRING
          "Flags used by the shared libraries linker during coverage builds."
          FORCE)
mark_as_advanced(
  CMAKE_Fortran_FLAGS_COVERAGE
  CMAKE_CXX_FLAGS_COVERAGE
  CMAKE_C_FLAGS_COVERAGE
  CMAKE_EXE_LINKER_FLAGS_COVERAGE
  CMAKE_SHARED_LINKER_FLAGS_COVERAGE)

if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
  message(
    WARNING
      "Code coverage results with an optimised (non-Debug) build may be misleading"
  )
endif() # NOT CMAKE_BUILD_TYPE STREQUAL "Debug"

if(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_Fortran_COMPILER_ID STREQUAL
                                         "GNU")
  link_libraries(gcov)
endif()

# Defines a target for running and collection code coverage information Builds
# dependencies, runs the given executable and outputs reports. NOTE! The
# executable should always have a ZERO as exit code otherwise the coverage
# generation will not complete.
#
# setup_target_for_coverage_lcov( NAME testrunner_coverage                    #
# New target name EXECUTABLE testrunner -j ${PROCESSOR_COUNT} # Executable in
# PROJECT_BINARY_DIR DEPENDENCIES testrunner                     # Dependencies
# to build first BASE_DIRECTORY "../"                        # Base directory
# for report #  (defaults to PROJECT_SOURCE_DIR) EXCLUDE "src/dir1/*"
# "src/dir2/*"           # Patterns to exclude (can be relative #  to
# BASE_DIRECTORY, with CMake 3.4+) NO_DEMANGLE                                 #
# Don't demangle C++ symbols #  even if c++filt is found )
function(setup_target_for_coverage_lcov)

  set(options NO_DEMANGLE)
  set(oneValueArgs BASE_DIRECTORY NAME)
  set(multiValueArgs
      EXCLUDE
      EXECUTABLE
      EXECUTABLE_ARGS
      DEPENDENCIES
      LCOV_ARGS
      GENHTML_ARGS)
  cmake_parse_arguments(
    Coverage
    "${options}"
    "${oneValueArgs}"
    "${multiValueArgs}"
    ${ARGN})

  if(NOT LCOV_PATH)
    message(FATAL_ERROR "lcov not found! Aborting...")
  endif() # NOT LCOV_PATH

  if(NOT GENHTML_PATH)
    message(FATAL_ERROR "genhtml not found! Aborting...")
  endif() # NOT GENHTML_PATH

  # Set base directory (as absolute path), or default to PROJECT_SOURCE_DIR
  if(${Coverage_BASE_DIRECTORY})
    get_filename_component(BASEDIR ${Coverage_BASE_DIRECTORY} ABSOLUTE)
  else()
    set(BASEDIR ${PROJECT_SOURCE_DIR})
  endif()

  # Collect excludes (CMake 3.4+: Also compute absolute paths)
  set(LCOV_EXCLUDES "")
  foreach(EXCLUDE ${Coverage_EXCLUDE} ${COVERAGE_EXCLUDES}
                  ${COVERAGE_LCOV_EXCLUDES})
    if(CMAKE_VERSION VERSION_GREATER 3.4)
      get_filename_component(
        EXCLUDE
        ${EXCLUDE}
        ABSOLUTE
        BASE_DIR
        ${BASEDIR})
    endif()
    list(APPEND LCOV_EXCLUDES "${EXCLUDE}")
  endforeach()
  list(REMOVE_DUPLICATES LCOV_EXCLUDES)

  # Conditional arguments
  if(CPPFILT_PATH AND NOT ${Coverage_NO_DEMANGLE})
    set(GENHTML_EXTRA_ARGS "--demangle-cpp")
  endif()

  # Setup target
  add_custom_target(
    ${Coverage_NAME}
    # Cleanup lcov
    COMMAND ${LCOV_PATH} ${Coverage_LCOV_ARGS} --gcov-tool ${GCOV_PATH}
            -directory . -b ${BASEDIR} --zerocounters
    # Create baseline to make sure untouched files show up in the report
    COMMAND ${LCOV_PATH} ${Coverage_LCOV_ARGS} --gcov-tool ${GCOV_PATH} -c -i -d
            . -b ${BASEDIR} -o ${Coverage_NAME}.base
    # Run tests
    COMMAND ${Coverage_EXECUTABLE} ${Coverage_EXECUTABLE_ARGS}
    # Capturing lcov counters and generating report
    COMMAND
      ${LCOV_PATH} ${Coverage_LCOV_ARGS} --gcov-tool ${GCOV_PATH} --directory .
      -b ${BASEDIR} --capture --output-file ${Coverage_NAME}.capture
    # add baseline counters
    COMMAND
      ${LCOV_PATH} ${Coverage_LCOV_ARGS} --gcov-tool ${GCOV_PATH} -a
      ${Coverage_NAME}.base -a ${Coverage_NAME}.capture --output-file
      ${Coverage_NAME}.total
    # filter collected data to final coverage report
    COMMAND
      ${LCOV_PATH} ${Coverage_LCOV_ARGS} --gcov-tool ${GCOV_PATH} --remove
      ${Coverage_NAME}.total ${LCOV_EXCLUDES} --output-file
      ${Coverage_NAME}.info
    # Generate HTML output
    COMMAND ${GENHTML_PATH} ${GENHTML_EXTRA_ARGS} ${Coverage_GENHTML_ARGS} -o
            ${Coverage_NAME} ${Coverage_NAME}.info
    # Set output files as GENERATED (will be removed on 'make clean')
    BYPRODUCTS ${Coverage_NAME}.base
               ${Coverage_NAME}.capture
               ${Coverage_NAME}.total
               ${Coverage_NAME}.info
               ${Coverage_NAME} # report directory
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    DEPENDS ${Coverage_DEPENDENCIES}
    VERBATIM # Protect arguments to commands
    COMMENT
      "Resetting code coverage counters to zero.\nProcessing code coverage counters and generating report."
  )

  # Show where to find the lcov info report
  add_custom_command(
    TARGET ${Coverage_NAME}
    POST_BUILD
    COMMAND ;
    COMMENT "Lcov code coverage info report saved in ${Coverage_NAME}.info.")

  # Show info where to find the report
  add_custom_command(
    TARGET ${Coverage_NAME}
    POST_BUILD
    COMMAND ;
    COMMENT
      "Open ./${Coverage_NAME}/index.html in your browser to view the coverage report."
  )

endfunction() # setup_target_for_coverage_lcov

# Defines a target for running and collection code coverage information Builds
# dependencies, runs the given executable and outputs reports. NOTE! The
# executable should always have a ZERO as exit code otherwise the coverage
# generation will not complete.
#
# setup_target_for_coverage_gcovr_xml( NAME ctest_coverage                    #
# New target name EXECUTABLE ctest -j ${PROCESSOR_COUNT} # Executable in
# PROJECT_BINARY_DIR DEPENDENCIES executable_target         # Dependencies to
# build first BASE_DIRECTORY "../"                   # Base directory for report
# #  (defaults to PROJECT_SOURCE_DIR) EXCLUDE "src/dir1/*" "src/dir2/*"      #
# Patterns to exclude (can be relative #  to BASE_DIRECTORY, with CMake 3.4+) )
function(setup_target_for_coverage_gcovr_xml)

  set(options NONE)
  set(oneValueArgs BASE_DIRECTORY NAME)
  set(multiValueArgs EXCLUDE EXECUTABLE EXECUTABLE_ARGS DEPENDENCIES)
  cmake_parse_arguments(
    Coverage
    "${options}"
    "${oneValueArgs}"
    "${multiValueArgs}"
    ${ARGN})

  if(NOT GCOVR_PATH)
    message(FATAL_ERROR "gcovr not found! Aborting...")
  endif() # NOT GCOVR_PATH

  # Set base directory (as absolute path), or default to PROJECT_SOURCE_DIR
  if(${Coverage_BASE_DIRECTORY})
    get_filename_component(BASEDIR ${Coverage_BASE_DIRECTORY} ABSOLUTE)
  else()
    set(BASEDIR ${PROJECT_SOURCE_DIR})
  endif()

  # Collect excludes (CMake 3.4+: Also compute absolute paths)
  set(GCOVR_EXCLUDES "")
  foreach(EXCLUDE ${Coverage_EXCLUDE} ${COVERAGE_EXCLUDES}
                  ${COVERAGE_GCOVR_EXCLUDES})
    if(CMAKE_VERSION VERSION_GREATER 3.4)
      get_filename_component(
        EXCLUDE
        ${EXCLUDE}
        ABSOLUTE
        BASE_DIR
        ${BASEDIR})
    endif()
    list(APPEND GCOVR_EXCLUDES "${EXCLUDE}")
  endforeach()
  list(REMOVE_DUPLICATES GCOVR_EXCLUDES)

  # Combine excludes to several -e arguments
  set(GCOVR_EXCLUDE_ARGS "")
  foreach(EXCLUDE ${GCOVR_EXCLUDES})
    list(APPEND GCOVR_EXCLUDE_ARGS "-e")
    list(APPEND GCOVR_EXCLUDE_ARGS "${EXCLUDE}")
  endforeach()

  add_custom_target(
    ${Coverage_NAME}
    # Run tests
    ${Coverage_EXECUTABLE} ${Coverage_EXECUTABLE_ARGS}
    # Running gcovr
    COMMAND ${GCOVR_PATH} --sonarqube -r ${BASEDIR} ${GCOVR_EXCLUDE_ARGS}
            --object-directory=${PROJECT_BINARY_DIR} -o ${Coverage_NAME}.xml
    BYPRODUCTS ${Coverage_NAME}.xml
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    DEPENDS ${Coverage_DEPENDENCIES}
    VERBATIM # Protect arguments to commands
    COMMENT "Running gcovr to produce Cobertura code coverage report.")

  # Show info where to find the report
  add_custom_command(
    TARGET ${Coverage_NAME}
    POST_BUILD
    COMMAND ;
    COMMENT "Cobertura code coverage report saved in ${Coverage_NAME}.xml.")
endfunction() # setup_target_for_coverage_gcovr_xml

# Defines a target for running and collection code coverage information Builds
# dependencies, runs the given executable and outputs reports. NOTE! The
# executable should always have a ZERO as exit code otherwise the coverage
# generation will not complete.
#
# setup_target_for_coverage_gcovr_html( NAME ctest_coverage                    #
# New target name EXECUTABLE ctest -j ${PROCESSOR_COUNT} # Executable in
# PROJECT_BINARY_DIR DEPENDENCIES executable_target         # Dependencies to
# build first BASE_DIRECTORY "../"                   # Base directory for report
# #  (defaults to PROJECT_SOURCE_DIR) EXCLUDE "src/dir1/*" "src/dir2/*"      #
# Patterns to exclude (can be relative #  to BASE_DIRECTORY, with CMake 3.4+) )
function(setup_target_for_coverage_gcovr_html)

  set(options NONE)
  set(oneValueArgs BASE_DIRECTORY NAME)
  set(multiValueArgs EXCLUDE EXECUTABLE EXECUTABLE_ARGS DEPENDENCIES)
  cmake_parse_arguments(
    Coverage
    "${options}"
    "${oneValueArgs}"
    "${multiValueArgs}"
    ${ARGN})

  if(NOT GCOVR_PATH)
    message(FATAL_ERROR "gcovr not found! Aborting...")
  endif() # NOT GCOVR_PATH

  # Set base directory (as absolute path), or default to PROJECT_SOURCE_DIR
  if(${Coverage_BASE_DIRECTORY})
    get_filename_component(BASEDIR ${Coverage_BASE_DIRECTORY} ABSOLUTE)
  else()
    set(BASEDIR ${PROJECT_SOURCE_DIR})
  endif()

  # Collect excludes (CMake 3.4+: Also compute absolute paths)
  set(GCOVR_EXCLUDES "")
  foreach(EXCLUDE ${Coverage_EXCLUDE} ${COVERAGE_EXCLUDES}
                  ${COVERAGE_GCOVR_EXCLUDES})
    if(CMAKE_VERSION VERSION_GREATER 3.4)
      get_filename_component(
        EXCLUDE
        ${EXCLUDE}
        ABSOLUTE
        BASE_DIR
        ${BASEDIR})
    endif()
    list(APPEND GCOVR_EXCLUDES "${EXCLUDE}")
  endforeach()
  list(REMOVE_DUPLICATES GCOVR_EXCLUDES)

  # Combine excludes to several -e arguments
  set(GCOVR_EXCLUDE_ARGS "")
  foreach(EXCLUDE ${GCOVR_EXCLUDES})
    list(APPEND GCOVR_EXCLUDE_ARGS "-e")
    list(APPEND GCOVR_EXCLUDE_ARGS "${EXCLUDE}")
  endforeach()

  add_custom_target(
    ${Coverage_NAME}
    # Run tests
    ${Coverage_EXECUTABLE} ${Coverage_EXECUTABLE_ARGS}
    # Create folder
    COMMAND ${CMAKE_COMMAND} -E make_directory
            ${PROJECT_BINARY_DIR}/${Coverage_NAME}
    # Running gcovr
    COMMAND
      ${GCOVR_PATH} --html --html-details -r ${BASEDIR} ${GCOVR_EXCLUDE_ARGS}
      --object-directory=${PROJECT_BINARY_DIR} -o ${Coverage_NAME}/index.html
    BYPRODUCTS ${PROJECT_BINARY_DIR}/${Coverage_NAME} # report directory
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    DEPENDS ${Coverage_DEPENDENCIES}
    VERBATIM # Protect arguments to commands
    COMMENT "Running gcovr to produce HTML code coverage report.")

  # Show info where to find the report
  add_custom_command(
    TARGET ${Coverage_NAME}
    POST_BUILD
    COMMAND ;
    COMMENT
      "Open ./${Coverage_NAME}/index.html in your browser to view the coverage report."
  )

endfunction() # setup_target_for_coverage_gcovr_html

function(append_coverage_compiler_flags)
  set(CMAKE_C_FLAGS
      "${CMAKE_C_FLAGS} ${COVERAGE_COMPILER_FLAGS}"
      PARENT_SCOPE)
  set(CMAKE_CXX_FLAGS
      "${CMAKE_CXX_FLAGS} ${COVERAGE_COMPILER_FLAGS}"
      PARENT_SCOPE)
  set(CMAKE_Fortran_FLAGS
      "${CMAKE_Fortran_FLAGS} ${COVERAGE_COMPILER_FLAGS}"
      PARENT_SCOPE)
  message(
    STATUS "Appending code coverage compiler flags: ${COVERAGE_COMPILER_FLAGS}")
endfunction() # append_coverage_compiler_flags
