
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

IF(CMAKE_BINARY_DIR)
  MESSAGE(FATAL_ERROR "CPackZIP.cmake may only be used by CPack internally.")
ENDIF(CMAKE_BINARY_DIR)

FIND_PROGRAM(ZIP_EXECUTABLE wzzip PATHS "$ENV{ProgramFiles}/WinZip")
IF(ZIP_EXECUTABLE)
  SET(CPACK_ZIP_COMMAND "\"${ZIP_EXECUTABLE}\" -P \"<ARCHIVE>\" @<FILELIST>")
  SET(CPACK_ZIP_NEED_QUOTES TRUE)
ENDIF(ZIP_EXECUTABLE)

IF(NOT ZIP_EXECUTABLE)
  FIND_PROGRAM(ZIP_EXECUTABLE 7z PATHS "$ENV{ProgramFiles}/7-Zip") 
  IF(ZIP_EXECUTABLE)
    SET(CPACK_ZIP_COMMAND "\"${ZIP_EXECUTABLE}\" a -tzip \"<ARCHIVE>\" @<FILELIST>")
  SET(CPACK_ZIP_NEED_QUOTES TRUE)
  ENDIF(ZIP_EXECUTABLE)
ENDIF(NOT ZIP_EXECUTABLE)

IF(NOT ZIP_EXECUTABLE)
  FIND_PACKAGE(Cygwin)
  FIND_PROGRAM(ZIP_EXECUTABLE zip PATHS "${CYGWIN_INSTALL_PATH}/bin")
  IF(ZIP_EXECUTABLE)
    SET(CPACK_ZIP_COMMAND "\"${ZIP_EXECUTABLE}\" -r \"<ARCHIVE>\" . -i@<FILELIST>")
    SET(CPACK_ZIP_NEED_QUOTES FALSE)
  ENDIF(ZIP_EXECUTABLE)
ENDIF(NOT ZIP_EXECUTABLE)

