# - Try to find the GNU Transport Layer Security library (gnutls)
#
# Once done this will define
#
#  GNUTLS_FOUND - System has gnutls
#  GNUTLS_INCLUDE_DIR - The gnutls include directory
#  GNUTLS_LIBRARIES - The libraries needed to use gnutls
#  GNUTLS_DEFINITIONS - Compiler switches required for using gnutls

#=============================================================================
# Copyright 2009 Kitware, Inc.
# Copyright 2009 Philip Lowman <philip@yhbt.com>
# Copyright 2009 Brad Hards <bradh@kde.org>
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

# Note that this doesn't try to find the gnutls-extra package.


IF (GNUTLS_INCLUDE_DIR AND GNUTLS_LIBRARY)
   # in cache already
   SET(gnutls_FIND_QUIETLY TRUE)
ENDIF (GNUTLS_INCLUDE_DIR AND GNUTLS_LIBRARY)

IF (NOT WIN32)
   # try using pkg-config to get the directories and then use these values
   # in the FIND_PATH() and FIND_LIBRARY() calls
   # also fills in GNUTLS_DEFINITIONS, although that isn't normally useful
   FIND_PACKAGE(PkgConfig)
   PKG_CHECK_MODULES(PC_GNUTLS gnutls)
   SET(GNUTLS_DEFINITIONS ${PC_GNUTLS_CFLAGS_OTHER})
ENDIF (NOT WIN32)

FIND_PATH(GNUTLS_INCLUDE_DIR gnutls/gnutls.h
   HINTS
   ${PC_GNUTLS_INCLUDEDIR}
   ${PC_GNUTLS_INCLUDE_DIRS}
   )

FIND_LIBRARY(GNUTLS_LIBRARY NAMES gnutls libgnutls
   HINTS
   ${PC_GNUTLS_LIBDIR}
   ${PC_GNUTLS_LIBRARY_DIRS}
   )

MARK_AS_ADVANCED(GNUTLS_INCLUDE_DIR GNUTLS_LIBRARY)

# handle the QUIETLY and REQUIRED arguments and set GNUTLS_FOUND to TRUE if 
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(GnuTLS DEFAULT_MSG GNUTLS_LIBRARY GNUTLS_INCLUDE_DIR)

IF(GNUTLS_FOUND)
    SET(GNUTLS_LIBRARIES    ${GNUTLS_LIBRARY})
    SET(GNUTLS_INCLUDE_DIRS ${GNUTLS_INCLUDE_DIR})
ENDIF()

