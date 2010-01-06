# - helper module to find OSX frameworks

#=============================================================================
# Copyright 2003-2009 Kitware, Inc.
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

IF(NOT CMAKE_FIND_FRAMEWORKS_INCLUDED)
  SET(CMAKE_FIND_FRAMEWORKS_INCLUDED 1)
  MACRO(CMAKE_FIND_FRAMEWORKS fwk)
    SET(${fwk}_FRAMEWORKS)
    IF(APPLE)
      FOREACH(dir
          ~/Library/Frameworks/${fwk}.framework
          /Library/Frameworks/${fwk}.framework
          /System/Library/Frameworks/${fwk}.framework
          /Network/Library/Frameworks/${fwk}.framework)
        IF(EXISTS ${dir})
          SET(${fwk}_FRAMEWORKS ${${fwk}_FRAMEWORKS} ${dir})
        ENDIF(EXISTS ${dir})
      ENDFOREACH(dir)
    ENDIF(APPLE)
  ENDMACRO(CMAKE_FIND_FRAMEWORKS)
ENDIF(NOT CMAKE_FIND_FRAMEWORKS_INCLUDED)
