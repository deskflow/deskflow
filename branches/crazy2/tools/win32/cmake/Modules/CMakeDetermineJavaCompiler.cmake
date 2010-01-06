
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

# determine the compiler to use for Java programs
# NOTE, a generator may set CMAKE_Java_COMPILER before
# loading this file to force a compiler.

IF(NOT CMAKE_Java_COMPILER)
  # prefer the environment variable CC
  IF($ENV{JAVA_COMPILER} MATCHES ".+")
    GET_FILENAME_COMPONENT(CMAKE_Java_COMPILER_INIT $ENV{JAVA_COMPILER} PROGRAM PROGRAM_ARGS CMAKE_Java_FLAGS_ENV_INIT)
    IF(CMAKE_Java_FLAGS_ENV_INIT)
      SET(CMAKE_Java_COMPILER_ARG1 "${CMAKE_Java_FLAGS_ENV_INIT}" CACHE STRING "First argument to Java compiler")
    ENDIF(CMAKE_Java_FLAGS_ENV_INIT)
    IF(NOT EXISTS ${CMAKE_Java_COMPILER_INIT})
      MESSAGE(SEND_ERROR "Could not find compiler set in environment variable JAVA_COMPILER:\n$ENV{JAVA_COMPILER}.") 
    ENDIF(NOT EXISTS ${CMAKE_Java_COMPILER_INIT})
  ENDIF($ENV{JAVA_COMPILER} MATCHES ".+")

  IF($ENV{JAVA_RUNTIME} MATCHES ".+")
    GET_FILENAME_COMPONENT(CMAKE_Java_RUNTIME_INIT $ENV{JAVA_RUNTIME} PROGRAM PROGRAM_ARGS CMAKE_Java_FLAGS_ENV_INIT)
    IF(NOT EXISTS ${CMAKE_Java_RUNTIME_INIT})
      MESSAGE(SEND_ERROR "Could not find compiler set in environment variable JAVA_RUNTIME:\n$ENV{JAVA_RUNTIME}.") 
    ENDIF(NOT EXISTS ${CMAKE_Java_RUNTIME_INIT})
  ENDIF($ENV{JAVA_RUNTIME} MATCHES ".+")

  IF($ENV{JAVA_ARCHIVE} MATCHES ".+")
    GET_FILENAME_COMPONENT(CMAKE_Java_ARCHIVE_INIT $ENV{JAVA_ARCHIVE} PROGRAM PROGRAM_ARGS CMAKE_Java_FLAGS_ENV_INIT)
    IF(NOT EXISTS ${CMAKE_Java_ARCHIVE_INIT})
      MESSAGE(SEND_ERROR "Could not find compiler set in environment variable JAVA_ARCHIVE:\n$ENV{JAVA_ARCHIVE}.") 
    ENDIF(NOT EXISTS ${CMAKE_Java_ARCHIVE_INIT})
  ENDIF($ENV{JAVA_ARCHIVE} MATCHES ".+")

  SET(Java_BIN_PATH
    "[HKEY_LOCAL_MACHINE\\SOFTWARE\\JavaSoft\\Java Development Kit\\2.0;JavaHome]/bin"
    "[HKEY_LOCAL_MACHINE\\SOFTWARE\\JavaSoft\\Java Development Kit\\1.9;JavaHome]/bin"
    "[HKEY_LOCAL_MACHINE\\SOFTWARE\\JavaSoft\\Java Development Kit\\1.8;JavaHome]/bin"
    "[HKEY_LOCAL_MACHINE\\SOFTWARE\\JavaSoft\\Java Development Kit\\1.7;JavaHome]/bin"
    "[HKEY_LOCAL_MACHINE\\SOFTWARE\\JavaSoft\\Java Development Kit\\1.6;JavaHome]/bin"
    "[HKEY_LOCAL_MACHINE\\SOFTWARE\\JavaSoft\\Java Development Kit\\1.5;JavaHome]/bin"
    "[HKEY_LOCAL_MACHINE\\SOFTWARE\\JavaSoft\\Java Development Kit\\1.4;JavaHome]/bin"
    "[HKEY_LOCAL_MACHINE\\SOFTWARE\\JavaSoft\\Java Development Kit\\1.3;JavaHome]/bin"
    $ENV{JAVA_HOME}/bin
    /usr/bin
    /usr/lib/java/bin
    /usr/share/java/bin
    /usr/local/bin
    /usr/local/java/bin
    /usr/local/java/share/bin
    /usr/java/j2sdk1.4.2_04
    /usr/lib/j2sdk1.4-sun/bin
    /usr/java/j2sdk1.4.2_09/bin
    /usr/lib/j2sdk1.5-sun/bin
    /opt/sun-jdk-1.5.0.04/bin
    )
  # if no compiler has been specified yet, then look for one
  IF(CMAKE_Java_COMPILER_INIT)
    SET(CMAKE_Java_COMPILER ${CMAKE_Java_COMPILER_INIT} CACHE PATH "Java Compiler")
  ELSE(CMAKE_Java_COMPILER_INIT)
    FIND_PROGRAM(CMAKE_Java_COMPILER
      NAMES javac
      PATHS ${Java_BIN_PATH}
    )    
  ENDIF(CMAKE_Java_COMPILER_INIT)

  # if no runtime has been specified yet, then look for one
  IF(CMAKE_Java_RUNTIME_INIT)
    SET(CMAKE_Java_RUNTIME ${CMAKE_Java_RUNTIME_INIT} CACHE PATH "Java Compiler")
  ELSE(CMAKE_Java_RUNTIME_INIT)
    FIND_PROGRAM(CMAKE_Java_RUNTIME
      NAMES java
      PATHS ${Java_BIN_PATH}
    )    
  ENDIF(CMAKE_Java_RUNTIME_INIT)

  # if no archive has been specified yet, then look for one
  IF(CMAKE_Java_ARCHIVE_INIT)
    SET(CMAKE_Java_ARCHIVE ${CMAKE_Java_ARCHIVE_INIT} CACHE PATH "Java Compiler")
  ELSE(CMAKE_Java_ARCHIVE_INIT)
    FIND_PROGRAM(CMAKE_Java_ARCHIVE
      NAMES jar
      PATHS ${Java_BIN_PATH}
    )    
  ENDIF(CMAKE_Java_ARCHIVE_INIT)
ENDIF(NOT CMAKE_Java_COMPILER)
MARK_AS_ADVANCED(CMAKE_Java_COMPILER)  

# configure variables set in this file for fast reload later on
CONFIGURE_FILE(${CMAKE_ROOT}/Modules/CMakeJavaCompiler.cmake.in 
  ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeJavaCompiler.cmake IMMEDIATE @ONLY)
SET(CMAKE_Java_COMPILER_ENV_VAR "JAVA_COMPILER")
