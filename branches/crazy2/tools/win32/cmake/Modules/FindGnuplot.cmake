# - this module looks for gnuplot
#
# Once done this will define
#
#  GNUPLOT_FOUND - system has Gnuplot
#  GNUPLOT_EXECUTABLE - the Gnuplot executable

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

INCLUDE(FindCygwin)

FIND_PROGRAM(GNUPLOT_EXECUTABLE
  NAMES 
  gnuplot
  pgnuplot
  wgnupl32
  PATHS
  ${CYGWIN_INSTALL_PATH}/bin
)

# for compatibility
SET(GNUPLOT ${GNUPLOT_EXECUTABLE})

# handle the QUIETLY and REQUIRED arguments and set GNUPLOT_FOUND to TRUE if 
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Gnuplot DEFAULT_MSG GNUPLOT_EXECUTABLE)

MARK_AS_ADVANCED( GNUPLOT_EXECUTABLE )

