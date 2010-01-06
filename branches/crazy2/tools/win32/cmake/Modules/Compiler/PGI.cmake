
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
if(__COMPILER_PGI)
  return()
endif()
set(__COMPILER_PGI 1)

macro(__compiler_pgi lang)
  # Feature flags.
  set(CMAKE_${lang}_VERBOSE_FLAG "-v")

  # Initial configuration flags.
  set(CMAKE_${lang}_FLAGS_INIT "")
  set(CMAKE_${lang}_FLAGS_DEBUG_INIT "-g -O0")
  set(CMAKE_${lang}_FLAGS_MINSIZEREL_INIT "-O2 -s")
  set(CMAKE_${lang}_FLAGS_RELEASE_INIT "-fast -O3 -Mipa=fast")
  set(CMAKE_${lang}_FLAGS_RELWITHDEBINFO_INIT "-O2 -gopt")

  # Preprocessing and assembly rules.
  set(CMAKE_${lang}_CREATE_PREPROCESSED_SOURCE "<CMAKE_${lang}_COMPILER> <DEFINES> <FLAGS> -E <SOURCE> > <PREPROCESSED_SOURCE>")
  set(CMAKE_${lang}_CREATE_ASSEMBLY_SOURCE "<CMAKE_${lang}_COMPILER> <DEFINES> <FLAGS> -S <SOURCE> -o <ASSEMBLY_SOURCE>")
endmacro()
