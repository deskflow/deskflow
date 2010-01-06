# Locate PhysFS library
# This module defines
# PHYSFS_LIBRARY, the name of the library to link against
# PHYSFS_FOUND, if false, do not try to link to PHYSFS
# PHYSFS_INCLUDE_DIR, where to find physfs.h
#
# $PHYSFSDIR is an environment variable that would
# correspond to the ./configure --prefix=$PHYSFSDIR
# used in building PHYSFS.
#
# Created by Eric Wing. 

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

FIND_PATH(PHYSFS_INCLUDE_DIR physfs.h
  HINTS
  $ENV{PHYSFSDIR}
  PATH_SUFFIXES include/physfs include
  PATHS
  ~/Library/Frameworks
  /Library/Frameworks
  /usr/local
  /usr
  /sw # Fink
  /opt/local # DarwinPorts
  /opt/csw # Blastwave
  /opt
)

FIND_LIBRARY(PHYSFS_LIBRARY 
  NAMES physfs
  HINTS
  $ENV{PHYSFSDIR}
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
)

SET(PHYSFS_FOUND "NO")
IF(PHYSFS_LIBRARY AND PHYSFS_INCLUDE_DIR)
  SET(PHYSFS_FOUND "YES")
ENDIF(PHYSFS_LIBRARY AND PHYSFS_INCLUDE_DIR)

