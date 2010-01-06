# - Find unix commands from cygwin
# This module looks for some usual Unix commands.
#

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

INCLUDE(FindCygwin)

FIND_PROGRAM(BASH
  bash
  ${CYGWIN_INSTALL_PATH}/bin
  /bin
  /usr/bin 
  /usr/local/bin
  /sbin
)
MARK_AS_ADVANCED(
  BASH
)

FIND_PROGRAM(CP
  cp
  ${CYGWIN_INSTALL_PATH}/bin
  /bin
  /usr/bin 
  /usr/local/bin
  /sbin
)
MARK_AS_ADVANCED(
  CP
)

FIND_PROGRAM(GZIP
  gzip
  ${CYGWIN_INSTALL_PATH}/bin
  /bin
  /usr/bin 
  /usr/local/bin
  /sbin
)
MARK_AS_ADVANCED(
  GZIP
)

FIND_PROGRAM(MV
  mv
  ${CYGWIN_INSTALL_PATH}/bin
  /bin
  /usr/bin 
  /usr/local/bin
  /sbin
)
MARK_AS_ADVANCED(
  MV
)

FIND_PROGRAM(RM
  rm
  ${CYGWIN_INSTALL_PATH}/bin
  /bin
  /usr/bin 
  /usr/local/bin
  /sbin
)
MARK_AS_ADVANCED(
  RM
)

FIND_PROGRAM(TAR
  NAMES 
  tar 
  gtar
  PATH
  ${CYGWIN_INSTALL_PATH}/bin
  /bin
  /usr/bin 
  /usr/local/bin
  /sbin
)
MARK_AS_ADVANCED(
  TAR
)
