#
# This CMake file contains two macros to assist with searching for OSG
# libraries and nodekits.
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
# OSG_FIND_PATH
#
function(OSG_FIND_PATH module header)
   string(TOUPPER ${module} module_uc)

   # Try the user's environment request before anything else.
   find_path(${module_uc}_INCLUDE_DIR ${header}
       HINTS
            $ENV{${module_uc}_DIR}
            $ENV{OSG_DIR}
            $ENV{OSGDIR}
            $ENV{OSG_ROOT}
       PATH_SUFFIXES include
       PATHS
            /sw # Fink
            /opt/local # DarwinPorts
            /opt/csw # Blastwave
            /opt
            /usr/freeware
   )
endfunction(OSG_FIND_PATH module header)


#
# OSG_FIND_LIBRARY
#
function(OSG_FIND_LIBRARY module library)
   string(TOUPPER ${module} module_uc)

   find_library(${module_uc}_LIBRARY
       NAMES ${library}
       HINTS
            $ENV{${module_uc}_DIR}
            $ENV{OSG_DIR}
            $ENV{OSGDIR}
            $ENV{OSG_ROOT}
       PATH_SUFFIXES lib64 lib
       PATHS
            /sw # Fink
            /opt/local # DarwinPorts
            /opt/csw # Blastwave
            /opt
            /usr/freeware
   )

   find_library(${module_uc}_LIBRARY_DEBUG
       NAMES ${library}d
       HINTS
            $ENV{${module_uc}_DIR}
            $ENV{OSG_DIR}
            $ENV{OSGDIR}
            $ENV{OSG_ROOT}
       PATH_SUFFIXES lib64 lib
       PATHS
            /sw # Fink
            /opt/local # DarwinPorts
            /opt/csw # Blastwave
            /opt
            /usr/freeware
    )

   if(NOT ${module_uc}_LIBRARY_DEBUG)
      # They don't have a debug library
      set(${module_uc}_LIBRARY_DEBUG ${${module_uc}_LIBRARY} PARENT_SCOPE)
      set(${module_uc}_LIBRARIES ${${module_uc}_LIBRARY} PARENT_SCOPE)
   else()
      # They really have a FOO_LIBRARY_DEBUG
      set(${module_uc}_LIBRARIES 
          optimized ${${module_uc}_LIBRARY}
          debug ${${module_uc}_LIBRARY_DEBUG}
          PARENT_SCOPE
      )
   endif()
endfunction(OSG_FIND_LIBRARY module library)

#
# OSG_MARK_AS_ADVANCED
# Just a convenience function for calling MARK_AS_ADVANCED
#
function(OSG_MARK_AS_ADVANCED _module)
   string(TOUPPER ${_module} _module_UC)
   mark_as_advanced(${_module_UC}_INCLUDE_DIR)
   mark_as_advanced(${_module_UC}_LIBRARY)
   mark_as_advanced(${_module_UC}_LIBRARY_DEBUG)
endfunction()
