
#=============================================================================
# Copyright 2001-2009 Kitware, Inc.
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

# Use of this file is deprecated, and is here for backwards compatibility with CMake 1.4
# GLU library is now found by FindOpenGL.cmake
#
#

INCLUDE(FindOpenGL)

IF (OPENGL_GLU_FOUND)
  SET (GLU_LIBRARY ${OPENGL_LIBRARIES})
  SET (GLU_INCLUDE_PATH ${OPENGL_INCLUDE_DIR})
ENDIF (OPENGL_GLU_FOUND)

