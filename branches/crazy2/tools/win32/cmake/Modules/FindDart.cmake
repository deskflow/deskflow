# - Find DART
# This module looks for the dart testing software and sets DART_ROOT
# to point to where it found it.
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

FIND_PATH(DART_ROOT README.INSTALL 
    $ENV{DART_ROOT}
    ${PROJECT_SOURCE_DIR}/Dart 
     /usr/share/Dart 
    C:/Dart  
    "$ENV{ProgramFiles}/Dart"
    "C:/Program Files/Dart" 
    ${PROJECT_SOURCE_DIR}/../Dart 
    [HKEY_LOCAL_MACHINE\\SOFTWARE\\Dart\\InstallPath]
    DOC "If you have Dart installed, where is it located?"
    )
