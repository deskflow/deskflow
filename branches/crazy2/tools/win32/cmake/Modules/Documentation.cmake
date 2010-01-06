# - DocumentationVTK.cmake
# This file provides support for the VTK documentation framework.
# It relies on several tools (Doxygen, Perl, etc).

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

#
# Build the documentation ?
#
OPTION(BUILD_DOCUMENTATION "Build the documentation (Doxygen)." OFF)
MARK_AS_ADVANCED(BUILD_DOCUMENTATION)

IF (BUILD_DOCUMENTATION)

  #
  # Check for the tools
  #
  FIND_PACKAGE(UnixCommands)
  FIND_PACKAGE(Doxygen)
  FIND_PACKAGE(Gnuplot)
  FIND_PACKAGE(HTMLHelp)
  FIND_PACKAGE(Perl)
  FIND_PACKAGE(Wget)

  OPTION(DOCUMENTATION_HTML_HELP 
    "Build the HTML Help file (CHM)." OFF)

  OPTION(DOCUMENTATION_HTML_TARZ 
    "Build a compressed tar archive of the HTML doc." OFF)

  MARK_AS_ADVANCED(
    DOCUMENTATION_HTML_HELP
    DOCUMENTATION_HTML_TARZ
    )

  # 
  # The documentation process is controled by a batch file.
  # We will probably need bash to create the custom target
  #

ENDIF (BUILD_DOCUMENTATION)
