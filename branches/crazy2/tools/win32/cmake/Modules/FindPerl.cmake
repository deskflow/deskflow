# - Find perl
# this module looks for Perl
#
#  PERL_EXECUTABLE - the full path to perl
#  PERL_FOUND      - If false, don't attempt to use perl.

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

SET(PERL_POSSIBLE_BIN_PATHS
  ${CYGWIN_INSTALL_PATH}/bin
  )

IF(WIN32)
  GET_FILENAME_COMPONENT(
    ActivePerl_CurrentVersion 
    "[HKEY_LOCAL_MACHINE\\SOFTWARE\\ActiveState\\ActivePerl;CurrentVersion]" 
    NAME)
  SET(PERL_POSSIBLE_BIN_PATHS ${PERL_POSSIBLE_BIN_PATHS}
    "C:/Perl/bin" 
    [HKEY_LOCAL_MACHINE\\SOFTWARE\\ActiveState\\ActivePerl\\${ActivePerl_CurrentVersion}]/bin
    )
ENDIF(WIN32)

FIND_PROGRAM(PERL_EXECUTABLE
  NAMES perl
  PATHS ${PERL_POSSIBLE_BIN_PATHS}
  )

# Deprecated settings for compatibility with CMake1.4
SET(PERL ${PERL_EXECUTABLE})

# handle the QUIETLY and REQUIRED arguments and set PERL_FOUND to TRUE if 
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Perl DEFAULT_MSG PERL_EXECUTABLE)

MARK_AS_ADVANCED(PERL_EXECUTABLE)
