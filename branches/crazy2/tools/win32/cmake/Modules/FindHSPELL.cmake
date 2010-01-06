# - Try to find HSPELL
# Once done this will define
#
#  HSPELL_FOUND - system has HSPELL
#  HSPELL_INCLUDE_DIR - the HSPELL include directory
#  HSPELL_LIBRARIES - The libraries needed to use HSPELL
#  HSPELL_DEFINITIONS - Compiler switches required for using HSPELL

#=============================================================================
# Copyright 2006-2009 Kitware, Inc.
# Copyright 2006 Alexander Neundorf <neundorf@kde.org>
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

IF (HSPELL_INCLUDE_DIR AND HSPELL_LIBRARIES)
  # Already in cache, be silent
  SET(HSPELL_FIND_QUIETLY TRUE)
ENDIF (HSPELL_INCLUDE_DIR AND HSPELL_LIBRARIES)


FIND_PATH(HSPELL_INCLUDE_DIR hspell.h )

FIND_LIBRARY(HSPELL_LIBRARIES NAMES hspell )

# handle the QUIETLY and REQUIRED arguments and set HSPELL_FOUND to TRUE if 
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(HSPELL DEFAULT_MSG HSPELL_LIBRARIES HSPELL_INCLUDE_DIR)


MARK_AS_ADVANCED(HSPELL_INCLUDE_DIR HSPELL_LIBRARIES)

