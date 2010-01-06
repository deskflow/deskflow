# - Locate Qt include paths and libraries
# This module defines:
#  QT_INCLUDE_DIR - where to find qt.h, etc.
#  QT_LIBRARIES   - the libraries to link against to use Qt.
#  QT_DEFINITIONS - definitions to use when
#                   compiling code that uses Qt.
#  QT_FOUND       - If false, don't try to use Qt.
#
# If you need the multithreaded version of Qt, set QT_MT_REQUIRED to TRUE
#
# Also defined, but not for general use are:
#  QT_MOC_EXECUTABLE, where to find the moc tool.
#  QT_UIC_EXECUTABLE, where to find the uic tool.
#  QT_QT_LIBRARY, where to find the Qt library.
#  QT_QTMAIN_LIBRARY, where to find the qtmain
#   library. This is only required by Qt3 on Windows.

# These are around for backwards compatibility
# they will be set
#  QT_WRAP_CPP, set true if QT_MOC_EXECUTABLE is found
#  QT_WRAP_UI set true if QT_UIC_EXECUTABLE is found

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

# If Qt4 has already been found, fail.
IF(QT4_FOUND)
  IF(Qt3_FIND_REQUIRED)
    MESSAGE( FATAL_ERROR "Qt3 and Qt4 cannot be used together in one project.")
  ELSE(Qt3_FIND_REQUIRED)
    IF(NOT Qt3_FIND_QUIETLY)
      MESSAGE( STATUS    "Qt3 and Qt4 cannot be used together in one project.")
    ENDIF(NOT Qt3_FIND_QUIETLY)
    RETURN()
  ENDIF(Qt3_FIND_REQUIRED)
ENDIF(QT4_FOUND)


FILE(GLOB GLOB_PATHS_BIN /usr/lib/qt-3*/bin/)
FIND_PATH(QT_INCLUDE_DIR qt.h
  "[HKEY_CURRENT_USER\\Software\\Trolltech\\Qt3Versions\\3.2.1;InstallDir]/include/Qt"
  "[HKEY_CURRENT_USER\\Software\\Trolltech\\Qt3Versions\\3.2.0;InstallDir]/include/Qt"
  "[HKEY_CURRENT_USER\\Software\\Trolltech\\Qt3Versions\\3.1.0;InstallDir]/include/Qt"
  $ENV{QTDIR}/include
  ${GLOB_PATHS_BIN}
  /usr/local/qt/include
  /usr/lib/qt/include
  /usr/lib/qt3/include
  /usr/include/qt
  /usr/share/qt3/include
  C:/Progra~1/qt/include
  /usr/include/qt3
  )

# if qglobal.h is not in the qt_include_dir then set
# QT_INCLUDE_DIR to NOTFOUND
IF(NOT EXISTS ${QT_INCLUDE_DIR}/qglobal.h)
  SET(QT_INCLUDE_DIR QT_INCLUDE_DIR-NOTFOUND CACHE PATH "path to Qt3 include directory" FORCE)
ENDIF(NOT EXISTS ${QT_INCLUDE_DIR}/qglobal.h)

IF(QT_INCLUDE_DIR)
  #extract the version string from qglobal.h
  FILE(READ ${QT_INCLUDE_DIR}/qglobal.h QGLOBAL_H)
  STRING(REGEX MATCH "#define[\t ]+QT_VERSION_STR[\t ]+\"([0-9]+.[0-9]+.[0-9]+)\"" QGLOBAL_H "${QGLOBAL_H}")
  STRING(REGEX REPLACE ".*\"([0-9]+.[0-9]+.[0-9]+)\".*" "\\1" qt_version_str "${QGLOBAL_H}")

  # Under windows the qt library (MSVC) has the format qt-mtXYZ where XYZ is the
  # version X.Y.Z, so we need to remove the dots from version
  STRING(REGEX REPLACE "\\." "" qt_version_str_lib "${qt_version_str}")
ENDIF(QT_INCLUDE_DIR)

FILE(GLOB GLOB_PATHS_LIB /usr/lib/qt-3*/lib/)
IF (QT_MT_REQUIRED)
  FIND_LIBRARY(QT_QT_LIBRARY
    NAMES
    qt-mt qt-mt${qt_version_str_lib} qt-mtnc${qt_version_str_lib}
    qt-mtedu${qt_version_str_lib} qt-mt230nc qt-mtnc321 qt-mt3
    PATHS
  "[HKEY_CURRENT_USER\\Software\\Trolltech\\Qt3Versions\\3.2.1;InstallDir]/lib"
  "[HKEY_CURRENT_USER\\Software\\Trolltech\\Qt3Versions\\3.2.0;InstallDir]/lib"
  "[HKEY_CURRENT_USER\\Software\\Trolltech\\Qt3Versions\\3.1.0;InstallDir]/lib"
    $ENV{QTDIR}/lib
    ${GLOB_PATHS_LIB}
    /usr/local/qt/lib
    /usr/lib/qt/lib
    /usr/lib/qt3/lib
    /usr/lib/qt3/lib64
    /usr/share/qt3/lib
    C:/Progra~1/qt/lib
    )

ELSE (QT_MT_REQUIRED)
  FIND_LIBRARY(QT_QT_LIBRARY
    NAMES
    qt qt-${qt_version_str_lib} qt-edu${qt_version_str_lib}
    qt-mt qt-mt${qt_version_str_lib} qt-mtnc${qt_version_str_lib}
    qt-mtedu${qt_version_str_lib} qt-mt230nc qt-mtnc321 qt-mt3
    PATHS
    "[HKEY_CURRENT_USER\\Software\\Trolltech\\Qt3Versions\\3.2.1;InstallDir]/lib"
    "[HKEY_CURRENT_USER\\Software\\Trolltech\\Qt3Versions\\3.2.0;InstallDir]/lib"
    "[HKEY_CURRENT_USER\\Software\\Trolltech\\Qt3Versions\\3.1.0;InstallDir]/lib"
    $ENV{QTDIR}/lib
    ${GLOB_PATHS_LIB}
    /usr/local/qt/lib
    /usr/lib/qt/lib
    /usr/lib/qt3/lib
    /usr/lib/qt3/lib64
    /usr/share/qt3/lib
    C:/Progra~1/qt/lib
    )
ENDIF (QT_MT_REQUIRED)


FIND_LIBRARY(QT_QASSISTANTCLIENT_LIBRARY
  NAMES qassistantclient
  PATHS
  "[HKEY_CURRENT_USER\\Software\\Trolltech\\Qt3Versions\\3.2.1;InstallDir]/lib"
  "[HKEY_CURRENT_USER\\Software\\Trolltech\\Qt3Versions\\3.2.0;InstallDir]/lib"
  "[HKEY_CURRENT_USER\\Software\\Trolltech\\Qt3Versions\\3.1.0;InstallDir]/lib"
  $ENV{QTDIR}/lib
  ${GLOB_PATHS_LIB}
  /usr/local/qt/lib
  /usr/lib/qt3/lib
  /usr/lib/qt3/lib64
  /usr/share/qt3/lib
  C:/Progra~1/qt/lib
  )

# qt 3 should prefer QTDIR over the PATH
FIND_PROGRAM(QT_MOC_EXECUTABLE
  NAMES moc-qt3 moc
  HINTS
  $ENV{QTDIR}/bin
  PATHS
  "[HKEY_CURRENT_USER\\Software\\Trolltech\\Qt3Versions\\3.2.1;InstallDir]/include/Qt"
  "[HKEY_CURRENT_USER\\Software\\Trolltech\\Qt3Versions\\3.2.0;InstallDir]/include/Qt"
  "[HKEY_CURRENT_USER\\Software\\Trolltech\\Qt3Versions\\3.1.0;InstallDir]/include/Qt"
  $ENV{QTDIR}/bin
  ${GLOB_PATHS_BIN}
  /usr/local/qt/bin
  /usr/lib/qt/bin
  /usr/lib/qt3/bin
  /usr/share/qt3/bin
  C:/Progra~1/qt/bin
  /usr/X11R6/bin
  )

IF(QT_MOC_EXECUTABLE)
  SET ( QT_WRAP_CPP "YES")
ENDIF(QT_MOC_EXECUTABLE)

# qt 3 should prefer QTDIR over the PATH
FIND_PROGRAM(QT_UIC_EXECUTABLE
  NAMES uic-qt3 uic
  HINTS
  $ENV{QTDIR}/bin
  PATHS
  "[HKEY_CURRENT_USER\\Software\\Trolltech\\Qt3Versions\\3.2.1;InstallDir]/include/Qt"
  "[HKEY_CURRENT_USER\\Software\\Trolltech\\Qt3Versions\\3.2.0;InstallDir]/include/Qt"
  "[HKEY_CURRENT_USER\\Software\\Trolltech\\Qt3Versions\\3.1.0;InstallDir]/include/Qt"
  ${GLOB_PATHS_BIN}
  /usr/local/qt/bin
  /usr/lib/qt/bin
  /usr/lib/qt3/bin
  /usr/share/qt3/bin
  C:/Progra~1/qt/bin
  /usr/X11R6/bin
  )

IF(QT_UIC_EXECUTABLE)
  SET ( QT_WRAP_UI "YES")
ENDIF(QT_UIC_EXECUTABLE)

IF (WIN32)
  FIND_LIBRARY(QT_QTMAIN_LIBRARY qtmain
    "[HKEY_CURRENT_USER\\Software\\Trolltech\\Qt3Versions\\3.2.1;InstallDir]/lib"
    "[HKEY_CURRENT_USER\\Software\\Trolltech\\Qt3Versions\\3.2.0;InstallDir]/lib"
    "[HKEY_CURRENT_USER\\Software\\Trolltech\\Qt3Versions\\3.1.0;InstallDir]/lib"
    "$ENV{ProgramFiles}/qt/lib"
    $ENV{QTDIR}/lib "C:/Program Files/qt/lib"
    DOC "This Library is only needed by and included with Qt3 on MSWindows. It should be NOTFOUND, undefined or IGNORE otherwise."
    )
ENDIF (WIN32)


IF (QT_MIN_VERSION)

  STRING(REGEX REPLACE "([0-9]+)\\.[0-9]+\\.[0-9]+" "\\1" qt_major_vers "${qt_version_str}")
  STRING(REGEX REPLACE "[0-9]+\\.([0-9]+)\\.[0-9]+" "\\1" qt_minor_vers "${qt_version_str}")
  STRING(REGEX REPLACE "[0-9]+\\.[0-9]+\\.([0-9]+)" "\\1" qt_patch_vers "${qt_version_str}")

  #now parse the parts of the user given version string into variables
  STRING(REGEX MATCH "^[0-9]+\\.[0-9]+\\.[0-9]+$" req_qt_major_vers "${QT_MIN_VERSION}")
  IF (NOT req_qt_major_vers)
    error_message(  "Invalid Qt version string given: \"${QT_MIN_VERSION}\", expected e.g. \"3.1.5\"")
  ENDIF (NOT req_qt_major_vers)

  STRING(REGEX REPLACE "([0-9]+)\\.[0-9]+\\.[0-9]+" "\\1" req_qt_major_vers "${QT_MIN_VERSION}")
  STRING(REGEX REPLACE "[0-9]+\\.([0-9])+\\.[0-9]+" "\\1" req_qt_minor_vers "${QT_MIN_VERSION}")
  STRING(REGEX REPLACE "[0-9]+\\.[0-9]+\\.([0-9]+)" "\\1" req_qt_patch_vers "${QT_MIN_VERSION}")

  # req = "6.5.4", qt = "3.2.1"
  MACRO(error_message msg)
    IF(QT3_REQUIRED)
      MESSAGE( FATAL_ERROR ${msg})
    ELSE(QT3_REQUIRED)
      MESSAGE( STATUS ${msg})
    ENDIF(QT3_REQUIRED)
  ENDMACRO(error_message)

  IF (req_qt_major_vers GREATER qt_major_vers)                  # (6 > 3) ?
    ERROR_MESSAGE(  "Qt major version not matched (required: ${QT_MIN_VERSION}, found: ${qt_version_str})")            # yes
  ELSE  (req_qt_major_vers GREATER qt_major_vers)               # no
    IF (req_qt_major_vers LESS qt_major_vers)                  # (6 < 3) ?
      SET( QT_VERSION_BIG_ENOUGH "YES" )                      # yes
    ELSE (req_qt_major_vers LESS qt_major_vers)                # ( 6==3) ?
      IF (req_qt_minor_vers GREATER qt_minor_vers)            # (5>2) ?
        ERROR_MESSAGE(  "Qt minor version not matched (required: ${QT_MIN_VERSION}, found: ${qt_version_str})")      # yes
      ELSE (req_qt_minor_vers GREATER qt_minor_vers)          # no
        IF (req_qt_minor_vers LESS qt_minor_vers)            # (5<2) ?
          SET( QT_VERSION_BIG_ENOUGH "YES" )                # yes
        ELSE (req_qt_minor_vers LESS qt_minor_vers)          # (5==2)
          IF (req_qt_patch_vers GREATER qt_patch_vers)      # (4>1) ?
            ERROR_MESSAGE(  "Qt patch level not matched (required: ${QT_MIN_VERSION}, found: ${qt_version_str})")  # yes
          ELSE (req_qt_patch_vers GREATER qt_patch_vers)    # (4>1) ?
            SET( QT_VERSION_BIG_ENOUGH "YES" )             # yes
          ENDIF (req_qt_patch_vers GREATER qt_patch_vers)   # (4>1) ?
        ENDIF (req_qt_minor_vers LESS qt_minor_vers)
      ENDIF (req_qt_minor_vers GREATER qt_minor_vers)
    ENDIF (req_qt_major_vers LESS qt_major_vers)
  ENDIF (req_qt_major_vers GREATER qt_major_vers)
ENDIF (QT_MIN_VERSION)

# if the include a library are found then we have it
IF(QT_INCLUDE_DIR AND QT_QT_LIBRARY)
  SET( QT_FOUND "YES" )
ENDIF(QT_INCLUDE_DIR AND QT_QT_LIBRARY)

IF(QT_FOUND)
  SET( QT_LIBRARIES ${QT_LIBRARIES} ${QT_QT_LIBRARY} )
  SET( QT_DEFINITIONS "")

  IF (WIN32 AND NOT CYGWIN)
    IF (QT_QTMAIN_LIBRARY)
      # for version 3
      SET (QT_DEFINITIONS -DQT_DLL -DQT_THREAD_SUPPORT -DNO_DEBUG)
      SET (QT_LIBRARIES imm32.lib ${QT_QT_LIBRARY} ${QT_QTMAIN_LIBRARY} )
      SET (QT_LIBRARIES ${QT_LIBRARIES} winmm wsock32)
    ELSE (QT_QTMAIN_LIBRARY)
      # for version 2
      SET (QT_LIBRARIES imm32.lib ws2_32.lib ${QT_QT_LIBRARY} )
    ENDIF (QT_QTMAIN_LIBRARY)
  ELSE (WIN32 AND NOT CYGWIN)
    SET (QT_LIBRARIES ${QT_QT_LIBRARY} )

    SET (QT_DEFINITIONS -DQT_SHARED -DQT_NO_DEBUG)
    IF(QT_QT_LIBRARY MATCHES "qt-mt")
      SET (QT_DEFINITIONS ${QT_DEFINITIONS} -DQT_THREAD_SUPPORT -D_REENTRANT)
    ENDIF(QT_QT_LIBRARY MATCHES "qt-mt")

  ENDIF (WIN32 AND NOT CYGWIN)

  IF (QT_QASSISTANTCLIENT_LIBRARY)
    SET (QT_LIBRARIES ${QT_QASSISTANTCLIENT_LIBRARY} ${QT_LIBRARIES})
  ENDIF (QT_QASSISTANTCLIENT_LIBRARY)

  # Backwards compatibility for CMake1.4 and 1.2
  SET (QT_MOC_EXE ${QT_MOC_EXECUTABLE} )
  SET (QT_UIC_EXE ${QT_UIC_EXECUTABLE} )
  # for unix add X11 stuff
  IF(UNIX)
    FIND_PACKAGE(X11)
    IF (X11_FOUND)
      SET (QT_LIBRARIES ${QT_LIBRARIES} ${X11_LIBRARIES})
    ENDIF (X11_FOUND)
    IF (CMAKE_DL_LIBS)
      SET (QT_LIBRARIES ${QT_LIBRARIES} ${CMAKE_DL_LIBS})
    ENDIF (CMAKE_DL_LIBS)
  ENDIF(UNIX)
  IF(QT_QT_LIBRARY MATCHES "qt-mt")
    FIND_PACKAGE(Threads)
    SET(QT_LIBRARIES ${QT_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
  ENDIF(QT_QT_LIBRARY MATCHES "qt-mt")
ENDIF(QT_FOUND)

EXEC_PROGRAM(${QT_MOC_EXECUTABLE} ARGS "-v" OUTPUT_VARIABLE QTVERSION_MOC)
EXEC_PROGRAM(${QT_UIC_EXECUTABLE} ARGS "-version" OUTPUT_VARIABLE QTVERSION_UIC)

SET(_QT_UIC_VERSION_3 FALSE)
IF("${QTVERSION_UIC}" MATCHES ".* 3..*")
  SET(_QT_UIC_VERSION_3 TRUE)
ENDIF("${QTVERSION_UIC}" MATCHES ".* 3..*")

SET(_QT_MOC_VERSION_3 FALSE)
IF("${QTVERSION_MOC}" MATCHES ".* 3..*")
  SET(_QT_MOC_VERSION_3 TRUE)
ENDIF("${QTVERSION_MOC}" MATCHES ".* 3..*")

SET(QT_WRAP_CPP FALSE)
IF (QT_MOC_EXECUTABLE AND _QT_MOC_VERSION_3)
  SET ( QT_WRAP_CPP TRUE)
ENDIF (QT_MOC_EXECUTABLE AND _QT_MOC_VERSION_3)

SET(QT_WRAP_UI FALSE)
IF (QT_UIC_EXECUTABLE AND _QT_UIC_VERSION_3)
  SET ( QT_WRAP_UI TRUE)
ENDIF (QT_UIC_EXECUTABLE AND _QT_UIC_VERSION_3)

MARK_AS_ADVANCED(
  QT_INCLUDE_DIR
  QT_QT_LIBRARY
  QT_QTMAIN_LIBRARY
  QT_QASSISTANTCLIENT_LIBRARY
  QT_UIC_EXECUTABLE
  QT_MOC_EXECUTABLE
  QT_WRAP_CPP
  QT_WRAP_UI
  )
