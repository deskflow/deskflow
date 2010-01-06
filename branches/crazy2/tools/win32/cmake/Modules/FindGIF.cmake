# This module defines
# GIF_LIBRARIES - libraries to link to in order to use GIF
# GIF_FOUND, if false, do not try to link 
# GIF_INCLUDE_DIR, where to find the headers
#
# $GIF_DIR is an environment variable that would
# correspond to the ./configure --prefix=$GIF_DIR

#=============================================================================
# Copyright 2007-2009 Kitware, Inc.
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

# Created by Eric Wing. 
# Modifications by Alexander Neundorf

FIND_PATH(GIF_INCLUDE_DIR gif_lib.h
  HINTS
  $ENV{GIF_DIR}
  PATH_SUFFIXES include
  PATHS
  ~/Library/Frameworks
  /Library/Frameworks
  /sw/include # Fink
  [HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\Session\ Manager\\Environment;OSG_ROOT]/include
  /usr/freeware/include
)

# the gif library can have many names :-/
SET(POTENTIAL_GIF_LIBS gif libgif ungif libungif giflib)

FIND_LIBRARY(GIF_LIBRARY 
  NAMES ${POTENTIAL_GIF_LIBS}
  HINTS
  $ENV{GIF_DIR}
  PATH_SUFFIXES lib64 lib
  PATHS
  ~/Library/Frameworks
  /Library/Frameworks
  /usr/local
  /usr
  /sw
  /opt/local
  /opt/csw
  /opt
  [HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\Session\ Manager\\Environment;OSG_ROOT]
  /usr/freeware
)

# see readme.txt
SET(GIF_LIBRARIES ${GIF_LIBRARY})

# handle the QUIETLY and REQUIRED arguments and set GIF_FOUND to TRUE if 
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(GIF  DEFAULT_MSG  GIF_LIBRARY  GIF_INCLUDE_DIR)

MARK_AS_ADVANCED(GIF_INCLUDE_DIR GIF_LIBRARY)
