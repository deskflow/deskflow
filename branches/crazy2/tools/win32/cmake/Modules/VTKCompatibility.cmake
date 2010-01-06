
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

IF(APPLE)
  SET(CMAKE_CXX_CREATE_SHARED_LIBRARY "${CMAKE_C_CREATE_SHARED_LIBRARY}")
  SET(CMAKE_CXX_CREATE_SHARED_MODULE "${CMAKE_C_CREATE_SHARED_MODULE}")
  STRING( REGEX REPLACE "CMAKE_C_COMPILER"
    CMAKE_CXX_COMPILER CMAKE_CXX_CREATE_SHARED_MODULE
    "${CMAKE_CXX_CREATE_SHARED_MODULE}")
  STRING( REGEX REPLACE "CMAKE_C_COMPILER"
    CMAKE_CXX_COMPILER CMAKE_CXX_CREATE_SHARED_LIBRARY
    "${CMAKE_CXX_CREATE_SHARED_LIBRARY}")
ENDIF(APPLE)

SET(VTKFTGL_BINARY_DIR "${VTK_BINARY_DIR}/Utilities/ftgl"
  CACHE INTERNAL "")
SET(VTKFREETYPE_BINARY_DIR "${VTK_BINARY_DIR}/Utilities/freetype"
  CACHE INTERNAL "")
SET(VTKFTGL_SOURCE_DIR "${VTK_SOURCE_DIR}/Utilities/ftgl"
  CACHE INTERNAL "")
SET(VTKFREETYPE_SOURCE_DIR "${VTK_SOURCE_DIR}/Utilities/freetype"
  CACHE INTERNAL "")

SET(VTK_GLEXT_FILE "${VTK_SOURCE_DIR}/Utilities/ParseOGLExt/headers/glext.h"
  CACHE FILEPATH
  "Location of the OpenGL extensions header file (glext.h).")
SET(VTK_GLXEXT_FILE
  "${VTK_SOURCE_DIR}/Utilities/ParseOGLExt/headers/glxext.h" CACHE FILEPATH
  "Location of the GLX extensions header file (glxext.h).")
SET(VTK_WGLEXT_FILE "${VTK_SOURCE_DIR}/Utilities/ParseOGLExt/headers/wglext.h"
  CACHE FILEPATH
  "Location of the WGL extensions header file (wglext.h).")

# work around an old bug in VTK
SET(TIFF_RIGHT_VERSION 1)

# for very old VTK (versions prior to 4.2)
MACRO(SOURCE_FILES)
  message (FATAL_ERROR "You are trying to build a very old version of VTK (prior to VTK 4.2). To do this you need to use CMake 2.0 as it was the last version of CMake to support VTK 4.0.")
ENDMACRO(SOURCE_FILES)

