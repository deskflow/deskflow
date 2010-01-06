# - Find JNI java libraries.
# This module finds if Java is installed and determines where the
# include files and libraries are. It also determines what the name of
# the library is. This code sets the following variables:
#   
#  JNI_INCLUDE_DIRS      = the include dirs to use
#  JNI_LIBRARIES         = the libraries to use
#  JNI_FOUND             = TRUE if JNI headers and libraries were found.
#  JAVA_AWT_LIBRARY      = the path to the jawt library
#  JAVA_JVM_LIBRARY      = the path to the jvm library
#  JAVA_INCLUDE_PATH     = the include path to jni.h
#  JAVA_INCLUDE_PATH2    = the include path to jni_md.h
#  JAVA_AWT_INCLUDE_PATH = the include path to jawt.h
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

# Expand {libarch} occurences to java_libarch subdirectory(-ies) and set ${_var}
MACRO(java_append_library_directories _var)
    # Determine java arch-specific library subdir
    IF (CMAKE_SYSTEM_NAME MATCHES "Linux")
        # Based on openjdk/jdk/make/common/shared/Platform.gmk as of 6b16
        # and kaffe as of 1.1.8 which uses the first part of the
        # GNU config.guess platform triplet.
        IF(CMAKE_SYSTEM_PROCESSOR MATCHES "^i[3-9]86$")
            SET(_java_libarch "i386")
        ELSEIF(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
            SET(_java_libarch "amd64" "x86_64")
        ELSEIF(CMAKE_SYSTEM_PROCESSOR MATCHES "^ppc")
            SET(_java_libarch "ppc" "powerpc" "ppc64")
        ELSEIF(CMAKE_SYSTEM_PROCESSOR MATCHES "^sparc")
            SET(_java_libarch "sparc" "sparcv9")
        ELSE(CMAKE_SYSTEM_PROCESSOR MATCHES "^i[3-9]86$")
            SET(_java_libarch "${CMAKE_SYSTEM_PROCESSOR}")
        ENDIF(CMAKE_SYSTEM_PROCESSOR MATCHES "^i[3-9]86$")
    ELSE(CMAKE_SYSTEM_NAME MATCHES "Linux")
        SET(_java_libarch "i386" "amd64" "ppc") # previous default
    ENDIF(CMAKE_SYSTEM_NAME MATCHES "Linux")

    FOREACH(_path ${ARGN})
        IF(_path MATCHES "{libarch}")
            FOREACH(_libarch ${_java_libarch})
                STRING(REPLACE "{libarch}" "${_libarch}" _newpath "${_path}")
                LIST(APPEND ${_var} "${_newpath}")
            ENDFOREACH(_libarch)
        ELSE(_path MATCHES "{libarch}")
            LIST(APPEND ${_var} "${_path}")
        ENDIF(_path MATCHES "{libarch}")
    ENDFOREACH(_path)
ENDMACRO(java_append_library_directories)

GET_FILENAME_COMPONENT(java_install_version
  "[HKEY_LOCAL_MACHINE\\SOFTWARE\\JavaSoft\\Java Development Kit;CurrentVersion]" NAME)

SET(JAVA_AWT_LIBRARY_DIRECTORIES
  "[HKEY_LOCAL_MACHINE\\SOFTWARE\\JavaSoft\\Java Development Kit\\1.4;JavaHome]/lib"
  "[HKEY_LOCAL_MACHINE\\SOFTWARE\\JavaSoft\\Java Development Kit\\1.3;JavaHome]/lib"
  "[HKEY_LOCAL_MACHINE\\SOFTWARE\\JavaSoft\\Java Development Kit\\${java_install_version};JavaHome]/lib"
  )

FILE(TO_CMAKE_PATH "$ENV{JAVA_HOME}" _JAVA_HOME)

JAVA_APPEND_LIBRARY_DIRECTORIES(JAVA_AWT_LIBRARY_DIRECTORIES
  ${_JAVA_HOME}/jre/lib/{libarch}
  ${_JAVA_HOME}/jre/lib
  ${_JAVA_HOME}/lib
  ${_JAVA_HOME}
  /usr/lib
  /usr/local/lib
  /usr/lib/jvm/java/lib
  /usr/lib/java/jre/lib/{libarch}
  /usr/local/lib/java/jre/lib/{libarch}
  /usr/local/share/java/jre/lib/{libarch}
  /usr/lib/j2sdk1.4-sun/jre/lib/{libarch}
  /usr/lib/j2sdk1.5-sun/jre/lib/{libarch}
  /opt/sun-jdk-1.5.0.04/jre/lib/{libarch}
  /usr/lib/jvm/java-6-sun/jre/lib/{libarch}
  /usr/lib/jvm/java-1.5.0-sun/jre/lib/{libarch}
  /usr/lib/jvm/java-6-sun-1.6.0.00/jre/lib/{libarch}       # can this one be removed according to #8821 ? Alex
  /usr/lib/jvm/java-6-openjdk/jre/lib/{libarch}
  # Debian specific paths for default JVM
  /usr/lib/jvm/default-java/jre/lib/{libarch}
  /usr/lib/jvm/default-java/jre/lib
  /usr/lib/jvm/default-java/lib
  )

SET(JAVA_JVM_LIBRARY_DIRECTORIES)
FOREACH(dir ${JAVA_AWT_LIBRARY_DIRECTORIES})
  SET(JAVA_JVM_LIBRARY_DIRECTORIES
    ${JAVA_JVM_LIBRARY_DIRECTORIES}
    "${dir}"
    "${dir}/client"
    "${dir}/server"
    )
ENDFOREACH(dir)


SET(JAVA_AWT_INCLUDE_DIRECTORIES
  "[HKEY_LOCAL_MACHINE\\SOFTWARE\\JavaSoft\\Java Development Kit\\1.4;JavaHome]/include"
  "[HKEY_LOCAL_MACHINE\\SOFTWARE\\JavaSoft\\Java Development Kit\\1.3;JavaHome]/include"
  "[HKEY_LOCAL_MACHINE\\SOFTWARE\\JavaSoft\\Java Development Kit\\${java_install_version};JavaHome]/include"
  ${_JAVA_HOME}/include
  /usr/include 
  /usr/local/include
  /usr/lib/java/include
  /usr/local/lib/java/include
  /usr/lib/jvm/java/include
  /usr/lib/jvm/java-6-sun/include
  /usr/lib/jvm/java-1.5.0-sun/include
  /usr/lib/jvm/java-6-sun-1.6.0.00/include       # can this one be removed according to #8821 ? Alex
  /usr/lib/jvm/java-6-openjdk/include
  /usr/local/share/java/include
  /usr/lib/j2sdk1.4-sun/include
  /usr/lib/j2sdk1.5-sun/include
  /opt/sun-jdk-1.5.0.04/include
  # Debian specific path for default JVM
  /usr/lib/jvm/default-java/include
  )

FOREACH(JAVA_PROG "${JAVA_RUNTIME}" "${JAVA_COMPILE}" "${JAVA_ARCHIVE}")
  GET_FILENAME_COMPONENT(jpath "${JAVA_PROG}" PATH)
  FOREACH(JAVA_INC_PATH ../include ../java/include ../share/java/include)
    IF(EXISTS ${jpath}/${JAVA_INC_PATH})
      SET(JAVA_AWT_INCLUDE_DIRECTORIES ${JAVA_AWT_INCLUDE_DIRECTORIES} "${jpath}/${JAVA_INC_PATH}")
    ENDIF(EXISTS ${jpath}/${JAVA_INC_PATH})
  ENDFOREACH(JAVA_INC_PATH)
  FOREACH(JAVA_LIB_PATH 
    ../lib ../jre/lib ../jre/lib/i386 
    ../java/lib ../java/jre/lib ../java/jre/lib/i386 
    ../share/java/lib ../share/java/jre/lib ../share/java/jre/lib/i386)
    IF(EXISTS ${jpath}/${JAVA_LIB_PATH})
      SET(JAVA_AWT_LIBRARY_DIRECTORIES ${JAVA_AWT_LIBRARY_DIRECTORIES} "${jpath}/${JAVA_LIB_PATH}")
    ENDIF(EXISTS ${jpath}/${JAVA_LIB_PATH})
  ENDFOREACH(JAVA_LIB_PATH)
ENDFOREACH(JAVA_PROG)

IF(APPLE)
  IF(EXISTS ~/Library/Frameworks/JavaVM.framework)
    SET(JAVA_HAVE_FRAMEWORK 1)
  ENDIF(EXISTS ~/Library/Frameworks/JavaVM.framework)
  IF(EXISTS /Library/Frameworks/JavaVM.framework)
    SET(JAVA_HAVE_FRAMEWORK 1)
  ENDIF(EXISTS /Library/Frameworks/JavaVM.framework)
  IF(EXISTS /System/Library/Frameworks/JavaVM.framework)
    SET(JAVA_HAVE_FRAMEWORK 1)
  ENDIF(EXISTS /System/Library/Frameworks/JavaVM.framework)

  IF(JAVA_HAVE_FRAMEWORK)
    IF(NOT JAVA_AWT_LIBRARY)
      SET (JAVA_AWT_LIBRARY "-framework JavaVM" CACHE FILEPATH "Java Frameworks" FORCE)
    ENDIF(NOT JAVA_AWT_LIBRARY)

    IF(NOT JAVA_JVM_LIBRARY)
      SET (JAVA_JVM_LIBRARY "-framework JavaVM" CACHE FILEPATH "Java Frameworks" FORCE)
    ENDIF(NOT JAVA_JVM_LIBRARY)

    IF(NOT JAVA_AWT_INCLUDE_PATH)
      IF(EXISTS /System/Library/Frameworks/JavaVM.framework/Headers/jawt.h)
        SET (JAVA_AWT_INCLUDE_PATH "/System/Library/Frameworks/JavaVM.framework/Headers" CACHE FILEPATH "jawt.h location" FORCE)
      ENDIF(EXISTS /System/Library/Frameworks/JavaVM.framework/Headers/jawt.h)
    ENDIF(NOT JAVA_AWT_INCLUDE_PATH)

    # If using "-framework JavaVM", prefer its headers *before* the others in
    # JAVA_AWT_INCLUDE_DIRECTORIES... (*prepend* to the list here)
    #
    SET(JAVA_AWT_INCLUDE_DIRECTORIES
      ~/Library/Frameworks/JavaVM.framework/Headers
      /Library/Frameworks/JavaVM.framework/Headers
      /System/Library/Frameworks/JavaVM.framework/Headers
      ${JAVA_AWT_INCLUDE_DIRECTORIES}
      )
  ENDIF(JAVA_HAVE_FRAMEWORK)
ELSE(APPLE)
  FIND_LIBRARY(JAVA_AWT_LIBRARY jawt 
    PATHS ${JAVA_AWT_LIBRARY_DIRECTORIES}
  )
  FIND_LIBRARY(JAVA_JVM_LIBRARY NAMES jvm JavaVM
    PATHS ${JAVA_JVM_LIBRARY_DIRECTORIES}
  )
ENDIF(APPLE)

# add in the include path    
FIND_PATH(JAVA_INCLUDE_PATH jni.h 
  ${JAVA_AWT_INCLUDE_DIRECTORIES}
)

FIND_PATH(JAVA_INCLUDE_PATH2 jni_md.h 
  ${JAVA_INCLUDE_PATH}
  ${JAVA_INCLUDE_PATH}/win32
  ${JAVA_INCLUDE_PATH}/linux
  ${JAVA_INCLUDE_PATH}/freebsd
  ${JAVA_INCLUDE_PATH}/solaris
)

FIND_PATH(JAVA_AWT_INCLUDE_PATH jawt.h
  ${JAVA_INCLUDE_PATH}
)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(JNI  DEFAULT_MSG  JAVA_AWT_LIBRARY JAVA_JVM_LIBRARY
                                                    JAVA_INCLUDE_PATH  JAVA_INCLUDE_PATH2 JAVA_AWT_INCLUDE_PATH)

MARK_AS_ADVANCED(
  JAVA_AWT_LIBRARY
  JAVA_JVM_LIBRARY
  JAVA_AWT_INCLUDE_PATH
  JAVA_INCLUDE_PATH
  JAVA_INCLUDE_PATH2
)

SET(JNI_LIBRARIES
  ${JAVA_AWT_LIBRARY}
  ${JAVA_JVM_LIBRARY}
)

SET(JNI_INCLUDE_DIRS
  ${JAVA_INCLUDE_PATH}
  ${JAVA_INCLUDE_PATH2}
  ${JAVA_AWT_INCLUDE_PATH}
)

