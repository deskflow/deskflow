
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

FIND_PROGRAM(CMAKE_MAKE_PROGRAM
  NAMES devenv
  PATHS
  [HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\VisualStudio\\7.0\\Setup\\VS;EnvironmentDirectory]
  "c:/Program Files/Microsoft Visual Studio .NET/Common7/IDE"
  "c:/Program Files/Microsoft Visual Studio.NET/Common7/IDE"
  "/Program Files/Microsoft Visual Studio .NET/Common7/IDE/"
  )
MARK_AS_ADVANCED(CMAKE_MAKE_PROGRAM)
SET(MSVC70 1)
