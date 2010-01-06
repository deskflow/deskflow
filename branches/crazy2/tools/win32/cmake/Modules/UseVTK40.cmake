#

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

# This is an implementation detail for using VTK 4.0 with the
# FindVTK.cmake module.  Do not include directly by name.  This should
# be included only when FindVTK.cmake sets the VTK_USE_FILE variable
# to point here.

# Add compiler flags needed to use VTK.
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${VTK_REQUIRED_C_FLAGS}")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${VTK_REQUIRED_CXX_FLAGS}")

# Add include directories needed to use VTK.
INCLUDE_DIRECTORIES(${VTK_INCLUDE_DIRS})

# Add link directories needed to use VTK.
LINK_DIRECTORIES(${VTK_LIBRARY_DIRS})
