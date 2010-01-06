# - Find OpenSceneGraph
# This module searches for the OpenSceneGraph core "osg" library as well as
# OpenThreads, and whatever additional COMPONENTS (nodekits) that you specify.
#    See http://www.openscenegraph.org
#
# NOTE: To use this module effectively you must either require CMake >= 2.6.3
# with cmake_minimum_required(VERSION 2.6.3) or download and place
# FindOpenThreads.cmake, Findosg_functions.cmake, Findosg.cmake,
# and Find<etc>.cmake files into your CMAKE_MODULE_PATH.
#
#==================================
#
# This module accepts the following variables (note mixed case)
#
#    OpenSceneGraph_DEBUG - Enable debugging output
#
#    OpenSceneGraph_MARK_AS_ADVANCED - Mark cache variables as advanced 
#                                      automatically
#
# The following environment variables are also respected for finding the OSG
# and it's various components.  CMAKE_PREFIX_PATH can also be used for this
# (see find_library() CMake documentation).
#
#    <MODULE>_DIR (where MODULE is of the form "OSGVOLUME" and there is a FindosgVolume.cmake file)
#    OSG_DIR
#    OSGDIR
#    OSG_ROOT
#
# This module defines the following output variables:
#
#    OPENSCENEGRAPH_FOUND - Was the OSG and all of the specified components found?
#
#    OPENSCENEGRAPH_VERSION - The version of the OSG which was found
#
#    OPENSCENEGRAPH_INCLUDE_DIRS - Where to find the headers
#
#    OPENSCENEGRAPH_LIBRARIES - The OSG libraries
#
#==================================
# Example Usage:
#
#  find_package(OpenSceneGraph 2.0.0 REQUIRED osgDB osgUtil)
#      # libOpenThreads & libosg automatically searched
#  include_directories(${OPENSCENEGRAPH_INCLUDE_DIRS})
#
#  add_executable(foo foo.cc)
#  target_link_libraries(foo ${OPENSCENEGRAPH_LIBRARIES})
#

#=============================================================================
# Copyright 2009 Kitware, Inc.
# Copyright 2009 Philip Lowman <philip@yhbt.com>
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

#
# Naming convention:
#  Local variables of the form _osg_foo
#  Input variables of the form OpenSceneGraph_FOO
#  Output variables of the form OPENSCENEGRAPH_FOO
#

include(Findosg_functions)

set(_osg_modules_to_process)
foreach(_osg_component ${OpenSceneGraph_FIND_COMPONENTS})
    list(APPEND _osg_modules_to_process ${_osg_component})
endforeach()
list(APPEND _osg_modules_to_process "osg" "OpenThreads")
list(REMOVE_DUPLICATES _osg_modules_to_process)

if(OpenSceneGraph_DEBUG)
    message("[ FindOpenSceneGraph.cmake:${CMAKE_CURRENT_LIST_LINE} ] "
        "Components = ${_osg_modules_to_process}")
endif()

#
# First we need to find and parse osg/Version
#
OSG_FIND_PATH(OSG osg/Version)
if(OpenSceneGraph_MARK_AS_ADVANCED)
    OSG_MARK_AS_ADVANCED(OSG)
endif()

# Try to ascertain the version...
if(OSG_INCLUDE_DIR)
    if(OpenSceneGraph_DEBUG)
        message("[ FindOpenSceneGraph.cmake:${CMAKE_CURRENT_LIST_LINE} ] "
            "Detected OSG_INCLUDE_DIR = ${OSG_INCLUDE_DIR}")
    endif()
    
    set(_osg_Version_file "${OSG_INCLUDE_DIR}/osg/Version")
    if("${OSG_INCLUDE_DIR}" MATCHES "\\.framework$" AND NOT EXISTS "${_osg_Version_file}")
        set(_osg_Version_file "${OSG_INCLUDE_DIR}/Headers/Version")
    endif()
    
    if(EXISTS "${_osg_Version_file}")
      file(READ "${_osg_Version_file}" _osg_Version_contents)
    else()
      set(_osg_Version_contents "unknown")
    endif()

    string(REGEX MATCH ".*#define OSG_VERSION_MAJOR[ \t]+[0-9]+.*"
        _osg_old_defines "${_osg_Version_contents}")
    string(REGEX MATCH ".*#define OPENSCENEGRAPH_MAJOR_VERSION[ \t]+[0-9]+.*"
        _osg_new_defines "${_osg_Version_contents}")
    if(_osg_old_defines)
        string(REGEX REPLACE ".*#define OSG_VERSION_MAJOR[ \t]+([0-9]+).*"
            "\\1" _osg_VERSION_MAJOR ${_osg_Version_contents})
        string(REGEX REPLACE ".*#define OSG_VERSION_MINOR[ \t]+([0-9]+).*"
            "\\1" _osg_VERSION_MINOR ${_osg_Version_contents})
        string(REGEX REPLACE ".*#define OSG_VERSION_PATCH[ \t]+([0-9]+).*"
            "\\1" _osg_VERSION_PATCH ${_osg_Version_contents})
    elseif(_osg_new_defines)
        string(REGEX REPLACE ".*#define OPENSCENEGRAPH_MAJOR_VERSION[ \t]+([0-9]+).*"
            "\\1" _osg_VERSION_MAJOR ${_osg_Version_contents})
        string(REGEX REPLACE ".*#define OPENSCENEGRAPH_MINOR_VERSION[ \t]+([0-9]+).*"
            "\\1" _osg_VERSION_MINOR ${_osg_Version_contents})
        string(REGEX REPLACE ".*#define OPENSCENEGRAPH_PATCH_VERSION[ \t]+([0-9]+).*"
            "\\1" _osg_VERSION_PATCH ${_osg_Version_contents})
    else()
        message("[ FindOpenSceneGraph.cmake:${CMAKE_CURRENT_LIST_LINE} ] "
            "Failed to parse version number, please report this as a bug")
    endif()

    set(OPENSCENEGRAPH_VERSION "${_osg_VERSION_MAJOR}.${_osg_VERSION_MINOR}.${_osg_VERSION_PATCH}"
                                CACHE INTERNAL "The version of OSG which was detected")
    if(OpenSceneGraph_DEBUG)
        message("[ FindOpenSceneGraph.cmake:${CMAKE_CURRENT_LIST_LINE} ] "
            "Detected version ${OPENSCENEGRAPH_VERSION}")
    endif()
endif()

#
# Version checking
#
if(OpenSceneGraph_FIND_VERSION AND OPENSCENEGRAPH_VERSION)
    if(OpenSceneGraph_FIND_VERSION_EXACT)
        if(NOT OPENSCENEGRAPH_VERSION VERSION_EQUAL ${OpenSceneGraph_FIND_VERSION})
            set(_osg_version_not_exact TRUE)
        endif()
    else()
        # version is too low
        if(NOT OPENSCENEGRAPH_VERSION VERSION_EQUAL ${OpenSceneGraph_FIND_VERSION} AND 
                NOT OPENSCENEGRAPH_VERSION VERSION_GREATER ${OpenSceneGraph_FIND_VERSION})
            set(_osg_version_not_high_enough TRUE)
        endif()
    endif()
endif()

set(_osg_quiet)
if(OpenSceneGraph_FIND_QUIETLY)
    set(_osg_quiet "QUIET")
endif()
#
# Here we call FIND_PACKAGE() on all of the components
#
foreach(_osg_module ${_osg_modules_to_process})
    if(OpenSceneGraph_DEBUG)
        message("[ FindOpenSceneGraph.cmake:${CMAKE_CURRENT_LIST_LINE} ] "
            "Calling find_package(${_osg_module} ${_osg_required} ${_osg_quiet})")
    endif()
    find_package(${_osg_module} ${_osg_quiet})

    string(TOUPPER ${_osg_module} _osg_module_UC)
    list(APPEND OPENSCENEGRAPH_INCLUDE_DIR ${${_osg_module_UC}_INCLUDE_DIR})
    list(APPEND OPENSCENEGRAPH_LIBRARIES ${${_osg_module_UC}_LIBRARIES})

    if(OpenSceneGraph_MARK_AS_ADVANCED)
        OSG_MARK_AS_ADVANCED(${_osg_module})
    endif()
endforeach()

if(OPENSCENEGRAPH_INCLUDE_DIR)
    list(REMOVE_DUPLICATES OPENSCENEGRAPH_INCLUDE_DIR)
endif()
        
#
# Inform the users with an error message based on
# what version they have vs. what version was
# required.
#
if(OpenSceneGraph_FIND_REQUIRED)
    set(_osg_version_output_type FATAL_ERROR)
else()
    set(_osg_version_output_type STATUS)
endif()
if(_osg_version_not_high_enough)
    set(_osg_EPIC_FAIL TRUE)
    if(NOT OpenSceneGraph_FIND_QUIETLY)
        message(${_osg_version_output_type}
            "ERROR: Version ${OpenSceneGraph_FIND_VERSION} or higher of the OSG "
            "is required.  Version ${OPENSCENEGRAPH_VERSION} was found.")
    endif()
elseif(_osg_version_not_exact)
    set(_osg_EPIC_FAIL TRUE)
    if(NOT OpenSceneGraph_FIND_QUIETLY)
        message(${_osg_version_output_type}
            "ERROR: Version ${OpenSceneGraph_FIND_VERSION} of the OSG is required "
            "(exactly), version ${OPENSCENEGRAPH_VERSION} was found.")
    endif()
else()

    #
    # Check each module to see if it's found
    #
    if(OpenSceneGraph_FIND_REQUIRED)
        set(_osg_missing_message)
        foreach(_osg_module ${_osg_modules_to_process})
            string(TOUPPER ${_osg_module} _osg_module_UC)
            if(NOT ${_osg_module_UC}_FOUND)
                set(_osg_missing_nodekit_fail true)
                set(_osg_missing_message "${_osg_missing_message} ${_osg_module}")
            endif()
        endforeach()
    
        if(_osg_missing_nodekit_fail)
            message(FATAL_ERROR "ERROR: Missing the following osg "
                "libraries: ${_osg_missing_message}.\n"
                "Consider using CMAKE_PREFIX_PATH or the OSG_DIR "
                "environment variable.  See the "
                "${CMAKE_CURRENT_LIST_FILE} for more details.")
        endif()
    endif()

    include(FindPackageHandleStandardArgs)
    FIND_PACKAGE_HANDLE_STANDARD_ARGS(OpenSceneGraph DEFAULT_MSG OPENSCENEGRAPH_LIBRARIES OPENSCENEGRAPH_INCLUDE_DIR)
endif()

if(_osg_EPIC_FAIL)
    # Zero out everything, we didn't meet version requirements
    set(OPENSCENEGRAPH_FOUND FALSE)
    set(OPENSCENEGRAPH_LIBRARIES)
    set(OPENSCENEGRAPH_INCLUDE_DIR)
endif()

set(OPENSCENEGRAPH_INCLUDE_DIRS ${OPENSCENEGRAPH_INCLUDE_DIR})

