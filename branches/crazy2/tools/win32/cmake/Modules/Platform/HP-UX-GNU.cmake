
#=============================================================================
# Copyright 2002-2009 Kitware, Inc.
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

# This module is shared by multiple languages; use include blocker.
if(__HPUX_COMPILER_GNU)
  return()
endif()
set(__HPUX_COMPILER_GNU 1)

macro(__hpux_compiler_gnu lang)
  set(CMAKE_SHARED_LIBRARY_CREATE_${lang}_FLAGS "${CMAKE_SHARED_LIBRARY_CREATE_${lang}_FLAGS} -Wl,-E,-b,+nodefaultrpath")
  set(CMAKE_SHARED_LIBRARY_LINK_${lang}_FLAGS "-Wl,+s,-E,+nodefaultrpath")
  set(CMAKE_SHARED_LIBRARY_RUNTIME_${lang}_FLAG "-Wl,+b")
  set(CMAKE_SHARED_LIBRARY_RUNTIME_${lang}_FLAG_SEP ":")
  set(CMAKE_SHARED_LIBRARY_SONAME_${lang}_FLAG "-Wl,+h")
endmacro()
