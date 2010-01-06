#
# This convenience include finds if wxWindows is installed
# and set the appropriate libs, incdirs, flags etc.
# author Jan Woetzel <jw -at- mip.informatik.uni-kiel.de> (07/2003)
##
# -----------------------------------------------------
# USAGE:
#   just include Use_wxWindows.cmake
#   in your projects CMakeLists.txt
# INCLUDE( ${CMAKE_MODULE_PATH}/Use_wxWindows.cmake)
##
#   if you are sure you need GL then
# SET(WXWINDOWS_USE_GL 1)
#   *before* you include this file.

#=============================================================================
# Copyright 2003-2009 Kitware, Inc.
# Copyright 2003      Jan Woetzel
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

# -----------------------------------------------------
# 16.Feb.2004: changed INCLUDE to FIND_PACKAGE to read from users own non-system CMAKE_MODULE_PATH (Jan Woetzel JW)
# 07/2006: rewrite as FindwxWidgets.cmake, kept for backward compatibility JW

MESSAGE(STATUS "Use_wxWindows.cmake is DEPRECATED. \n"
"Please use FIND_PACKAGE(wxWidgets) and INCLUDE(${wxWidgets_USE_FILE}) instead. (JW)")


# ------------------------

FIND_PACKAGE( wxWindows )

IF(WXWINDOWS_FOUND)

#MESSAGE("DBG Use_wxWindows.cmake:  WXWINDOWS_INCLUDE_DIR=${WXWINDOWS_INCLUDE_DIR} WXWINDOWS_LINK_DIRECTORIES=${WXWINDOWS_LINK_DIRECTORIES}     WXWINDOWS_LIBRARIES=${WXWINDOWS_LIBRARIES}  CMAKE_WXWINDOWS_CXX_FLAGS=${CMAKE_WXWINDOWS_CXX_FLAGS} WXWINDOWS_DEFINITIONS=${WXWINDOWS_DEFINITIONS}")

 IF(WXWINDOWS_INCLUDE_DIR)
    INCLUDE_DIRECTORIES(${WXWINDOWS_INCLUDE_DIR})
  ENDIF(WXWINDOWS_INCLUDE_DIR)
 IF(WXWINDOWS_LINK_DIRECTORIES)
    LINK_DIRECTORIES(${WXWINDOWS_LINK_DIRECTORIES})
  ENDIF(WXWINDOWS_LINK_DIRECTORIES)
  IF(WXWINDOWS_LIBRARIES)
    LINK_LIBRARIES(${WXWINDOWS_LIBRARIES})
  ENDIF(WXWINDOWS_LIBRARIES)
  IF (CMAKE_WXWINDOWS_CXX_FLAGS)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_WXWINDOWS_CXX_FLAGS}")
  ENDIF(CMAKE_WXWINDOWS_CXX_FLAGS)
  IF(WXWINDOWS_DEFINITIONS)
    ADD_DEFINITIONS(${WXWINDOWS_DEFINITIONS})
  ENDIF(WXWINDOWS_DEFINITIONS)
ELSE(WXWINDOWS_FOUND)
  MESSAGE(SEND_ERROR "wxWindows not found by Use_wxWindows.cmake")
ENDIF(WXWINDOWS_FOUND)

