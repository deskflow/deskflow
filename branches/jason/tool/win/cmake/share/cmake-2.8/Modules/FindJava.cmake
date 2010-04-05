# - Find Java
# This module finds if Java is installed and determines where the
# include files and libraries are. This code sets the following
# variables:
#
#  JAVA_RUNTIME    = the full path to the Java runtime
#  JAVA_COMPILE    = the full path to the Java compiler
#  JAVA_ARCHIVE    = the full path to the Java archiver
#

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

SET(JAVA_BIN_PATH
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
FIND_PROGRAM(JAVA_RUNTIME
  NAMES java
  PATHS ${JAVA_BIN_PATH}
)

FIND_PROGRAM(JAVA_ARCHIVE
  NAMES jar
  PATHS ${JAVA_BIN_PATH}
)

FIND_PROGRAM(JAVA_COMPILE
  NAMES javac
  PATHS ${JAVA_BIN_PATH}
)

MARK_AS_ADVANCED(
JAVA_RUNTIME
JAVA_ARCHIVE
JAVA_COMPILE
)
