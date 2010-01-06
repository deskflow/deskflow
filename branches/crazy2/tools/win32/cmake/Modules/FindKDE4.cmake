# Find KDE4 and provide all necessary variables and macros to compile software for it.
# It looks for KDE 4 in the following directories in the given order:
#  CMAKE_INSTALL_PREFIX
#  KDEDIRS
#  /opt/kde4
#
# Please look in FindKDE4Internal.cmake and KDE4Macros.cmake for more information.
# They are installed with the KDE 4 libraries in $KDEDIRS/share/apps/cmake/modules/.
#
# Author: Alexander Neundorf <neundorf@kde.org>

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

# If Qt3 has already been found, fail.
IF(QT_QT_LIBRARY)
  IF(KDE4_FIND_REQUIRED)
    MESSAGE( FATAL_ERROR "KDE4/Qt4 and Qt3 cannot be used together in one project.")
  ELSE(KDE4_FIND_REQUIRED)
    IF(NOT KDE4_FIND_QUIETLY)
      MESSAGE( STATUS    "KDE4/Qt4 and Qt3 cannot be used together in one project.")
    ENDIF(NOT KDE4_FIND_QUIETLY)
    RETURN()
  ENDIF(KDE4_FIND_REQUIRED)
ENDIF(QT_QT_LIBRARY)

FILE(TO_CMAKE_PATH "$ENV{KDEDIRS}" _KDEDIRS)

# when cross compiling, searching kde4-config in order to run it later on
# doesn't make a lot of sense. We'll have to do something about this. 
# Searching always in the target environment ? Then we get at least the correct one,
# still it can't be used to run it. Alex

# For KDE4 kde-config has been renamed to kde4-config
FIND_PROGRAM(KDE4_KDECONFIG_EXECUTABLE NAMES kde4-config
   # the suffix must be used since KDEDIRS can be a list of directories which don't have bin/ appended
   PATH_SUFFIXES bin               
   HINTS
   ${CMAKE_INSTALL_PREFIX}
   ${_KDEDIRS}
   /opt/kde4
   ONLY_CMAKE_FIND_ROOT_PATH
   )

IF (NOT KDE4_KDECONFIG_EXECUTABLE)
   IF (KDE4_FIND_REQUIRED)
      MESSAGE(FATAL_ERROR "ERROR: Could not find KDE4 kde4-config")
   ENDIF (KDE4_FIND_REQUIRED)
ENDIF (NOT KDE4_KDECONFIG_EXECUTABLE)


# when cross compiling, KDE4_DATA_DIR may be already preset
IF(NOT KDE4_DATA_DIR)
   IF(CMAKE_CROSSCOMPILING)
      # when cross compiling, don't run kde4-config but use its location as install dir
      GET_FILENAME_COMPONENT(KDE4_DATA_DIR "${KDE4_KDECONFIG_EXECUTABLE}" PATH)
      GET_FILENAME_COMPONENT(KDE4_DATA_DIR "${KDE4_DATA_DIR}" PATH)
   ELSE(CMAKE_CROSSCOMPILING)
      # then ask kde4-config for the kde data dirs

      IF(KDE4_KDECONFIG_EXECUTABLE)
        EXECUTE_PROCESS(COMMAND "${KDE4_KDECONFIG_EXECUTABLE}" --path data OUTPUT_VARIABLE _data_DIR ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
        FILE(TO_CMAKE_PATH "${_data_DIR}" _data_DIR)
        # then check the data dirs for FindKDE4Internal.cmake
        FIND_PATH(KDE4_DATA_DIR cmake/modules/FindKDE4Internal.cmake ${_data_DIR})
      ENDIF(KDE4_KDECONFIG_EXECUTABLE)
   ENDIF(CMAKE_CROSSCOMPILING)
ENDIF(NOT KDE4_DATA_DIR)

# if it has been found...
IF (KDE4_DATA_DIR)

   SET(CMAKE_MODULE_PATH  ${CMAKE_MODULE_PATH} ${KDE4_DATA_DIR}/cmake/modules)

   IF (KDE4_FIND_QUIETLY)
      SET(_quiet QUIET)
   ENDIF (KDE4_FIND_QUIETLY)

   IF (KDE4_FIND_REQUIRED)
      SET(_req REQUIRED)
   ENDIF (KDE4_FIND_REQUIRED)

   # use FindKDE4Internal.cmake to do the rest
   FIND_PACKAGE(KDE4Internal ${_req} ${_quiet})
ELSE (KDE4_DATA_DIR)
   IF (KDE4_FIND_REQUIRED)
      MESSAGE(FATAL_ERROR "ERROR: cmake/modules/FindKDE4Internal.cmake not found in ${_data_DIR}")
   ENDIF (KDE4_FIND_REQUIRED)
ENDIF (KDE4_DATA_DIR)
