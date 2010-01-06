
#=============================================================================
# Copyright 2009 Kitware, Inc.
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

# VCExpress does not support cross compiling, which is necessary for Win CE
SET( _CMAKE_MAKE_PROGRAM_NAMES devenv)
IF(NOT CMAKE_CROSSCOMPILING)
  SET( _CMAKE_MAKE_PROGRAM_NAMES ${_CMAKE_MAKE_PROGRAM_NAMES} VCExpress)
ENDIF(NOT CMAKE_CROSSCOMPILING)

FIND_PROGRAM(CMAKE_MAKE_PROGRAM
  NAMES MSBuild
  HINTS 
  "$ENV{SYSTEMROOT}/Microsoft.NET/Framework/[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\VisualStudio\\10.0;CLR Version]/"
  "c:/WINDOWS/Microsoft.NET/Framework/[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\VisualStudio\\10.0;CLR Version]/"
  )

FIND_PROGRAM(CMAKE_MAKE_PROGRAM
  NAMES ${_CMAKE_MAKE_PROGRAM_NAMES}
  HINTS
  [HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\VisualStudio\\10.0\\Setup\\VS;EnvironmentDirectory]
  [HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\VisualStudio\\10.0\\Setup;Dbghelp_path]
  "$ENV{ProgramFiles}/Microsoft Visual Studio 10.0/Common7/IDE"
  "$ENV{ProgramFiles}/Microsoft Visual Studio10.0/Common7/IDE"
  "$ENV{ProgramFiles}/Microsoft Visual Studio 10/Common7/IDE"
  "$ENV{ProgramFiles}/Microsoft Visual Studio10/Common7/IDE"
  "$ENV{ProgramFiles} (x86)/Microsoft Visual Studio 10.0/Common7/IDE"
  "$ENV{ProgramFiles} (x86)/Microsoft Visual Studio10.0/Common7/IDE"
  "$ENV{ProgramFiles} (x86)/Microsoft Visual Studio 10/Common7/IDE"
  "$ENV{ProgramFiles} (x86)/Microsoft Visual Studio10/Common7/IDE"
  "/Program Files/Microsoft Visual Studio 10.0/Common7/IDE/"
  "/Program Files/Microsoft Visual Studio 10/Common7/IDE/"
  PATHS
  "$ENV{ProgramFiles} (x86)/Microsoft Visual Studio .NET/Common7/IDE"
  "$ENV{ProgramFiles}/Microsoft Visual Studio .NET/Common7/IDE"
  
  )

MARK_AS_ADVANCED(CMAKE_MAKE_PROGRAM)
SET(MSVC10 1)
SET(MSVC_VERSION 1600)

