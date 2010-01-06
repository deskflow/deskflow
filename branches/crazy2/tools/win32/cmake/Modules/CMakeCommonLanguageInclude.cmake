
#=============================================================================
# Copyright 2004-2009 Kitware, Inc.
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

# this file has flags that are shared across languages and sets
# cache values that can be initialized in the platform-compiler.cmake file
# it may be included by more than one language.

SET (CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS_INIT} $ENV{LDFLAGS}
     CACHE STRING "Flags used by the linker.")


IF(NOT CMAKE_NOT_USING_CONFIG_FLAGS)
# default build type is none
  IF(NOT CMAKE_NO_BUILD_TYPE)
    SET (CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE_INIT} CACHE STRING 
      "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel.")
  ENDIF(NOT CMAKE_NO_BUILD_TYPE)
  
  SET (CMAKE_EXE_LINKER_FLAGS_DEBUG ${CMAKE_EXE_LINKER_FLAGS_DEBUG_INIT} CACHE STRING
     "Flags used by the linker during debug builds.")

  SET (CMAKE_EXE_LINKER_FLAGS_MINSIZEREL ${CMAKE_EXE_LINKER_FLAGS_MINSIZEREL_INIT} CACHE STRING
     "Flags used by the linker during release minsize builds.")

  SET (CMAKE_EXE_LINKER_FLAGS_RELEASE ${CMAKE_EXE_LINKER_FLAGS_RELEASE_INIT} CACHE STRING
     "Flags used by the linker during release builds.")

  SET (CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO 
     ${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO_INIT} CACHE STRING
     "Flags used by the linker during Release with Debug Info builds.")
  
  SET (CMAKE_SHARED_LINKER_FLAGS_DEBUG ${CMAKE_SHARED_LINKER_FLAGS_DEBUG_INIT} CACHE STRING
     "Flags used by the linker during debug builds.")

  SET (CMAKE_SHARED_LINKER_FLAGS_MINSIZEREL ${CMAKE_SHARED_LINKER_FLAGS_MINSIZEREL_INIT}
     CACHE STRING
     "Flags used by the linker during release minsize builds.")

  SET (CMAKE_SHARED_LINKER_FLAGS_RELEASE ${CMAKE_SHARED_LINKER_FLAGS_RELEASE_INIT} CACHE STRING
     "Flags used by the linker during release builds.")

  SET (CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO 
     ${CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO_INIT} CACHE STRING
     "Flags used by the linker during Release with Debug Info builds.")

  SET (CMAKE_MODULE_LINKER_FLAGS_DEBUG ${CMAKE_MODULE_LINKER_FLAGS_DEBUG_INIT} CACHE STRING
     "Flags used by the linker during debug builds.")

  SET (CMAKE_MODULE_LINKER_FLAGS_MINSIZEREL ${CMAKE_MODULE_LINKER_FLAGS_MINSIZEREL_INIT}
     CACHE STRING
     "Flags used by the linker during release minsize builds.")

  SET (CMAKE_MODULE_LINKER_FLAGS_RELEASE ${CMAKE_MODULE_LINKER_FLAGS_RELEASE_INIT} CACHE STRING
     "Flags used by the linker during release builds.")

  SET (CMAKE_MODULE_LINKER_FLAGS_RELWITHDEBINFO 
     ${CMAKE_MODULE_LINKER_FLAGS_RELWITHDEBINFO_INIT} CACHE STRING
     "Flags used by the linker during Release with Debug Info builds.")

ENDIF(NOT CMAKE_NOT_USING_CONFIG_FLAGS)
# shared linker flags
SET (CMAKE_SHARED_LINKER_FLAGS ${CMAKE_SHARED_LINKER_FLAGS_INIT} $ENV{LDFLAGS}
     CACHE STRING "Flags used by the linker during the creation of dll's.")

# module linker flags
SET (CMAKE_MODULE_LINKER_FLAGS ${CMAKE_MODULE_LINKER_FLAGS_INIT} $ENV{LDFLAGS}
     CACHE STRING "Flags used by the linker during the creation of modules.")

SET(CMAKE_BUILD_TOOL ${CMAKE_MAKE_PROGRAM} CACHE INTERNAL 
     "What is the target build tool cmake is generating for.")


MARK_AS_ADVANCED(
CMAKE_BUILD_TOOL
CMAKE_VERBOSE_MAKEFILE 

CMAKE_EXE_LINKER_FLAGS
CMAKE_EXE_LINKER_FLAGS_DEBUG
CMAKE_EXE_LINKER_FLAGS_MINSIZEREL
CMAKE_EXE_LINKER_FLAGS_RELEASE
CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO

CMAKE_SHARED_LINKER_FLAGS
CMAKE_SHARED_LINKER_FLAGS_DEBUG
CMAKE_SHARED_LINKER_FLAGS_MINSIZEREL
CMAKE_SHARED_LINKER_FLAGS_RELEASE
CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO

CMAKE_MODULE_LINKER_FLAGS
CMAKE_MODULE_LINKER_FLAGS_DEBUG
CMAKE_MODULE_LINKER_FLAGS_MINSIZEREL
CMAKE_MODULE_LINKER_FLAGS_RELEASE
CMAKE_MODULE_LINKER_FLAGS_RELWITHDEBINFO

)

