# - Searches for all installed versions of QT.
# This should only be used if your project can work with multiple
# versions of QT.  If not, you should just directly use FindQt4 or FindQt3.
# If multiple versions of QT are found on the machine, then
# The user must set the option DESIRED_QT_VERSION to the version
# they want to use.  If only one version of qt is found on the machine,
# then the DESIRED_QT_VERSION is set to that version and the 
# matching FindQt3 or FindQt4 module is included.
# Once the user sets DESIRED_QT_VERSION, then the FindQt3 or FindQt4 module
# is included.
#
#  QT_REQUIRED if this is set to TRUE then if CMake can 
#              not find QT4 or QT3 an error is raised 
#              and a message is sent to the user.
#
#  DESIRED_QT_VERSION OPTION is created
#  QT4_INSTALLED is set to TRUE if qt4 is found.
#  QT3_INSTALLED is set to TRUE if qt3 is found.

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

# look for signs of qt3 installations
FILE(GLOB GLOB_TEMP_VAR /usr/lib/qt-3*/bin/qmake)
IF(GLOB_TEMP_VAR)
  SET(QT3_INSTALLED TRUE)
ENDIF(GLOB_TEMP_VAR)
SET(GLOB_TEMP_VAR)

FILE(GLOB GLOB_TEMP_VAR /usr/local/qt-x11-commercial-3*/bin/qmake)
IF(GLOB_TEMP_VAR)
  SET(QT3_INSTALLED TRUE)
ENDIF(GLOB_TEMP_VAR)
SET(GLOB_TEMP_VAR)

# look for qt4 installations
FILE(GLOB GLOB_TEMP_VAR /usr/local/qt-x11-commercial-4*/bin/qmake)
IF(GLOB_TEMP_VAR)
  SET(QT4_INSTALLED TRUE)
ENDIF(GLOB_TEMP_VAR)
SET(GLOB_TEMP_VAR)

FILE(GLOB GLOB_TEMP_VAR /usr/local/Trolltech/Qt-4*/bin/qmake)
IF(GLOB_TEMP_VAR)
  SET(QT4_INSTALLED TRUE)
ENDIF(GLOB_TEMP_VAR)
SET(GLOB_TEMP_VAR)

# now find qmake
FIND_PROGRAM(QT_QMAKE_EXECUTABLE_FINDQT NAMES qmake PATHS "${QT_SEARCH_PATH}/bin" "$ENV{QTDIR}/bin")
IF(QT_QMAKE_EXECUTABLE_FINDQT)
  EXEC_PROGRAM(${QT_QMAKE_EXECUTABLE_FINDQT} ARGS "-query QT_VERSION"
    OUTPUT_VARIABLE QTVERSION)
  IF(QTVERSION MATCHES "4.*")
    SET(QT_QMAKE_EXECUTABLE ${QT_QMAKE_EXECUTABLE_FINDQT} CACHE PATH "QT4 qmake program.")
    SET(QT4_INSTALLED TRUE)
  ENDIF(QTVERSION MATCHES "4.*")
  IF(QTVERSION MATCHES "Unknown")
    SET(QT3_INSTALLED TRUE)
  ENDIF(QTVERSION MATCHES "Unknown")
ENDIF(QT_QMAKE_EXECUTABLE_FINDQT)

IF(QT_QMAKE_EXECUTABLE_FINDQT)
  EXEC_PROGRAM( ${QT_QMAKE_EXECUTABLE_FINDQT}
    ARGS "-query QT_INSTALL_HEADERS" 
    OUTPUT_VARIABLE qt_headers )
ENDIF(QT_QMAKE_EXECUTABLE_FINDQT)

FIND_FILE( QT4_QGLOBAL_H_FILE qglobal.h
  "${QT_SEARCH_PATH}/Qt/include"
  "[HKEY_CURRENT_USER\\Software\\Trolltech\\Qt3Versions\\4.0.0;InstallDir]/include/Qt"
  "[HKEY_CURRENT_USER\\Software\\Trolltech\\Versions\\4.0.0;InstallDir]/include/Qt"
  ${qt_headers}/Qt
  $ENV{QTDIR}/include/Qt
  /usr/local/qt/include/Qt
  /usr/local/include/Qt
  /usr/lib/qt/include/Qt
  /usr/include/Qt
  /usr/share/qt4/include/Qt
  C:/Progra~1/qt/include/Qt )

IF(QT4_QGLOBAL_H_FILE)
  SET(QT4_INSTALLED TRUE)
ENDIF(QT4_QGLOBAL_H_FILE)

FIND_FILE( QT3_QGLOBAL_H_FILE qglobal.h
  "${QT_SEARCH_PATH}/Qt/include" 
 "[HKEY_CURRENT_USER\\Software\\Trolltech\\Qt3Versions\\3.2.1;InstallDir]/include/Qt"
  "[HKEY_CURRENT_USER\\Software\\Trolltech\\Qt3Versions\\3.2.0;InstallDir]/include/Qt"
  "[HKEY_CURRENT_USER\\Software\\Trolltech\\Qt3Versions\\3.1.0;InstallDir]/include/Qt"
  C:/Qt/3.3.3Educational/include
  $ENV{QTDIR}/include
  /usr/include/qt3/Qt
  /usr/local/qt/include
  /usr/local/include
  /usr/lib/qt/include
  /usr/include
  /usr/share/qt3/include
  C:/Progra~1/qt/include
  /usr/include/qt3 )

IF(QT3_QGLOBAL_H_FILE)
  SET(QT3_INSTALLED TRUE)
ENDIF(QT3_QGLOBAL_H_FILE)

IF(QT3_INSTALLED AND QT4_INSTALLED )
  # force user to pick if we have both
  SET(DESIRED_QT_VERSION 0 CACHE STRING "Pick a version of QT to use: 3 or 4")
ELSE(QT3_INSTALLED AND QT4_INSTALLED )
  # if only one found then pick that one
  IF(QT3_INSTALLED)
    SET(DESIRED_QT_VERSION 3 CACHE STRING "Pick a version of QT to use: 3 or 4")
  ENDIF(QT3_INSTALLED)
  IF(QT4_INSTALLED)
    SET(DESIRED_QT_VERSION 4 CACHE STRING "Pick a version of QT to use: 3 or 4")
  ENDIF(QT4_INSTALLED)
ENDIF(QT3_INSTALLED AND QT4_INSTALLED )

IF(DESIRED_QT_VERSION MATCHES 3)
  SET(Qt3_FIND_REQUIRED ${Qt_FIND_REQUIRED})
  SET(Qt3_FIND_QUIETLY  ${Qt_FIND_QUIETLY})
  INCLUDE(FindQt3)
ENDIF(DESIRED_QT_VERSION MATCHES 3)
IF(DESIRED_QT_VERSION MATCHES 4)
  SET(Qt4_FIND_REQUIRED ${Qt_FIND_REQUIRED})
  SET(Qt4_FIND_QUIETLY  ${Qt_FIND_QUIETLY})
  INCLUDE(FindQt4)
ENDIF(DESIRED_QT_VERSION MATCHES 4)

IF(NOT QT3_INSTALLED AND NOT QT4_INSTALLED)
  IF(QT_REQUIRED)
    MESSAGE(SEND_ERROR "CMake was unable to find any QT versions, put qmake in your path, or set QT_QMAKE_EXECUTABLE.")
  ENDIF(QT_REQUIRED)
ELSE(NOT QT3_INSTALLED AND NOT QT4_INSTALLED)
  IF(NOT QT_FOUND AND NOT DESIRED_QT_VERSION)
    IF(QT_REQUIRED)
      MESSAGE(SEND_ERROR "Multiple versions of QT found please set DESIRED_QT_VERSION")
    ELSE(QT_REQUIRED)
      MESSAGE("Multiple versions of QT found please set DESIRED_QT_VERSION")
    ENDIF(QT_REQUIRED)
  ENDIF(NOT QT_FOUND AND NOT DESIRED_QT_VERSION)
  IF(NOT QT_FOUND AND DESIRED_QT_VERSION)
    IF(QT_REQUIRED)
      MESSAGE(FATAL_ERROR "CMake was unable to find QT version: ${DESIRED_QT_VERSION}. Set advanced values QT_QMAKE_EXECUTABLE and QT${DESIRED_QT_VERSION}_QGLOBAL_FILE, if those are set then QT_QT_LIBRARY or QT_LIBRARY_DIR.")
    ELSE(QT_REQUIRED)
      MESSAGE( "CMake was unable to find desired QT version: ${DESIRED_QT_VERSION}. Set advanced values QT_QMAKE_EXECUTABLE and QT${DESIRED_QT_VERSION}_QGLOBAL_FILE.")
    ENDIF(QT_REQUIRED)
  ENDIF(NOT QT_FOUND AND DESIRED_QT_VERSION)
ENDIF(NOT QT3_INSTALLED AND NOT QT4_INSTALLED)
MARK_AS_ADVANCED(QT3_QGLOBAL_H_FILE QT4_QGLOBAL_H_FILE QT_QMAKE_EXECUTABLE_FINDQT)
