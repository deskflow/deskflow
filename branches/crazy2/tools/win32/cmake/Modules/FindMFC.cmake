# - Find MFC on Windows
# Find the native MFC - i.e. decide if an application can link to the MFC
# libraries.
#  MFC_FOUND - Was MFC support found
# You don't need to include anything or link anything to use it.

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

# Assume no MFC support
SET(MFC_FOUND "NO")

# Only attempt the try_compile call if it has a chance to succeed:
SET(MFC_ATTEMPT_TRY_COMPILE 0)
IF(WIN32 AND NOT UNIX AND NOT BORLAND AND NOT MINGW)
  SET(MFC_ATTEMPT_TRY_COMPILE 1)
ENDIF(WIN32 AND NOT UNIX AND NOT BORLAND AND NOT MINGW)

IF(MFC_ATTEMPT_TRY_COMPILE)
  IF("MFC_HAVE_MFC" MATCHES "^MFC_HAVE_MFC$")
    SET(CHECK_INCLUDE_FILE_VAR "afxwin.h")
    CONFIGURE_FILE(${CMAKE_ROOT}/Modules/CheckIncludeFile.cxx.in
      ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/CheckIncludeFile.cxx)
    MESSAGE(STATUS "Looking for MFC")
    TRY_COMPILE(MFC_HAVE_MFC
      ${CMAKE_BINARY_DIR}
      ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/CheckIncludeFile.cxx
      CMAKE_FLAGS
      -DCMAKE_MFC_FLAG:STRING=2
      -DCOMPILE_DEFINITIONS:STRING=-D_AFXDLL
      OUTPUT_VARIABLE OUTPUT)
    IF(MFC_HAVE_MFC)
      MESSAGE(STATUS "Looking for MFC - found")
      SET(MFC_HAVE_MFC 1 CACHE INTERNAL "Have MFC?")
      FILE(APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeOutput.log
        "Determining if MFC exists passed with the following output:\n"
        "${OUTPUT}\n\n")
    ELSE(MFC_HAVE_MFC)
      MESSAGE(STATUS "Looking for MFC - not found")
      SET(MFC_HAVE_MFC 0 CACHE INTERNAL "Have MFC?")
      FILE(APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeError.log
        "Determining if MFC exists failed with the following output:\n"
        "${OUTPUT}\n\n")
    ENDIF(MFC_HAVE_MFC)
  ENDIF("MFC_HAVE_MFC" MATCHES "^MFC_HAVE_MFC$")

  IF(MFC_HAVE_MFC)
    SET(MFC_FOUND "YES")
  ENDIF(MFC_HAVE_MFC)
ENDIF(MFC_ATTEMPT_TRY_COMPILE)
