# - Convenience include for using wxWidgets library
# Finds if wxWidgets is installed
# and set the appropriate libs, incdirs, flags etc.
# INCLUDE_DIRECTORIES, LINK_DIRECTORIES and ADD_DEFINITIONS
# are called.
#
# USAGE
#  SET( wxWidgets_USE_LIBS  gl xml xrc ) # optionally: more than wx std libs
#  FIND_PACKAGE(wxWidgets REQUIRED)
#  INCLUDE( ${xWidgets_USE_FILE} )
#  ... add your targets here, e.g. ADD_EXECUTABLE/ ADD_LIBRARY ...
#  TARGET_LINK_LIBRARIERS( <yourWxDependantTarget>  ${wxWidgets_LIBRARIES})
#
# DEPRECATED
#  LINK_LIBRARIES is not called in favor of adding dependencies per target.
#
# AUTHOR
#  Jan Woetzel <jw -at- mip.informatik.uni-kiel.de>

#=============================================================================
# Copyright 2004-2009 Kitware, Inc.
# Copyright 2006      Jan Woetzel
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

# debug message and logging.
# comment these out for distribution
IF    (NOT LOGFILE )
  #  SET(LOGFILE "${PROJECT_BINARY_DIR}/CMakeOutput.log")
ENDIF (NOT LOGFILE )
MACRO(MSG _MSG)
  #  FILE(APPEND ${LOGFILE} "${CMAKE_CURRENT_LIST_FILE}(${CMAKE_CURRENT_LIST_LINE}):   ${_MSG}\n")
  #  MESSAGE(STATUS "${CMAKE_CURRENT_LIST_FILE}(${CMAKE_CURRENT_LIST_LINE}): ${_MSG}")
ENDMACRO(MSG)


MSG("wxWidgets_FOUND=${wxWidgets_FOUND}")
IF   (wxWidgets_FOUND)
  IF   (wxWidgets_INCLUDE_DIRS)
    IF(wxWidgets_INCLUDE_DIRS_NO_SYSTEM)
      INCLUDE_DIRECTORIES(${wxWidgets_INCLUDE_DIRS})
    ELSE(wxWidgets_INCLUDE_DIRS_NO_SYSTEM)
      INCLUDE_DIRECTORIES(SYSTEM ${wxWidgets_INCLUDE_DIRS})
    ENDIF(wxWidgets_INCLUDE_DIRS_NO_SYSTEM)
    MSG("wxWidgets_INCLUDE_DIRS=${wxWidgets_INCLUDE_DIRS}")
  ENDIF(wxWidgets_INCLUDE_DIRS)

  IF   (wxWidgets_LIBRARY_DIRS)
    LINK_DIRECTORIES(${wxWidgets_LIBRARY_DIRS})
    MSG("wxWidgets_LIBRARY_DIRS=${wxWidgets_LIBRARY_DIRS}")
  ENDIF(wxWidgets_LIBRARY_DIRS)

  IF   (wxWidgets_DEFINITIONS)
    SET_PROPERTY(DIRECTORY APPEND
      PROPERTY COMPILE_DEFINITIONS ${wxWidgets_DEFINITIONS})
    MSG("wxWidgets_DEFINITIONS=${wxWidgets_DEFINITIONS}")
  ENDIF(wxWidgets_DEFINITIONS)

  IF   (wxWidgets_DEFINITIONS_DEBUG)
    SET_PROPERTY(DIRECTORY APPEND
      PROPERTY COMPILE_DEFINITIONS_DEBUG ${wxWidgets_DEFINITIONS_DEBUG})
    MSG("wxWidgets_DEFINITIONS_DEBUG=${wxWidgets_DEFINITIONS_DEBUG}")
  ENDIF(wxWidgets_DEFINITIONS_DEBUG)

  IF   (wxWidgets_CXX_FLAGS)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${wxWidgets_CXX_FLAGS}")
    MSG("wxWidgets_CXX_FLAGS=${wxWidgets_CXX_FLAGS}")
  ENDIF(wxWidgets_CXX_FLAGS)

  # DEPRECATED JW
  # just for backward compatibility: add deps to all targets
  # library projects better use advanced FIND_PACKAGE(wxWidgets) directly.
  #IF(wxWidgets_LIBRARIES)
  #  LINK_LIBRARIES(${wxWidgets_LIBRARIES})
  #  # BUG: str too long:  MSG("wxWidgets_LIBRARIES=${wxWidgets_LIBRARIES}")
  #  IF(LOGFILE)
  #    FILE(APPEND ${LOGFILE} "${CMAKE_CURRENT_LIST_FILE}(${CMAKE_CURRENT_LIST_LINE}):   ${wxWidgets_LIBRARIES}\n")
  #  ENDIF(LOGFILE)
  #ENDIF(wxWidgets_LIBRARIES)

ELSE (wxWidgets_FOUND)
  MESSAGE("wxWidgets requested but not found.")
ENDIF(wxWidgets_FOUND)
