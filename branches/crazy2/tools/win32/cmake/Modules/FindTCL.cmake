# - Find Tcl includes and libraries.
# This module finds if Tcl is installed and determines where the
# include files and libraries are. It also determines what the name of
# the library is. This code sets the following variables:
#  TCL_FOUND              = Tcl was found
#  TK_FOUND               = Tk was found
#  TCLTK_FOUND            = Tcl and Tk were found
#  TCL_LIBRARY            = path to Tcl library (tcl tcl80)
#  TCL_INCLUDE_PATH       = path to where tcl.h can be found
#  TCL_TCLSH              = path to tclsh binary (tcl tcl80)
#  TK_LIBRARY             = path to Tk library (tk tk80 etc)
#  TK_INCLUDE_PATH        = path to where tk.h can be found
#  TK_WISH                = full path to the wish executable
#
# In an effort to remove some clutter and clear up some issues for people
# who are not necessarily Tcl/Tk gurus/developpers, some variables were
# moved or removed. Changes compared to CMake 2.4 are:
# - The stub libraries are now found in FindTclStub.cmake
#   => they were only useful for people writing Tcl/Tk extensions.
# - TCL_LIBRARY_DEBUG and TK_LIBRARY_DEBUG were removed.
#   => these libs are not packaged by default with Tcl/Tk distributions. 
#      Even when Tcl/Tk is built from source, several flavors of debug libs
#      are created and there is no real reason to pick a single one
#      specifically (say, amongst tcl84g, tcl84gs, or tcl84sgx). 
#      Let's leave that choice to the user by allowing him to assign 
#      TCL_LIBRARY to any Tcl library, debug or not.
# - TK_INTERNAL_PATH was removed.
#   => this ended up being only a Win32 variable, and there is a lot of
#      confusion regarding the location of this file in an installed Tcl/Tk
#      tree anyway (see 8.5 for example). If you need the internal path at
#      this point it is safer you ask directly where the *source* tree is
#      and dig from there.

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

INCLUDE(CMakeFindFrameworks)
INCLUDE(FindTclsh)
INCLUDE(FindWish)

GET_FILENAME_COMPONENT(TCL_TCLSH_PATH "${TCL_TCLSH}" PATH)
GET_FILENAME_COMPONENT(TCL_TCLSH_PATH_PARENT "${TCL_TCLSH_PATH}" PATH)
STRING(REGEX REPLACE 
  "^.*tclsh([0-9]\\.*[0-9]).*$" "\\1" TCL_TCLSH_VERSION "${TCL_TCLSH}")

GET_FILENAME_COMPONENT(TK_WISH_PATH "${TK_WISH}" PATH)
GET_FILENAME_COMPONENT(TK_WISH_PATH_PARENT "${TK_WISH_PATH}" PATH)
STRING(REGEX REPLACE 
  "^.*wish([0-9]\\.*[0-9]).*$" "\\1" TK_WISH_VERSION "${TK_WISH}")

GET_FILENAME_COMPONENT(TCL_INCLUDE_PATH_PARENT "${TCL_INCLUDE_PATH}" PATH)
GET_FILENAME_COMPONENT(TK_INCLUDE_PATH_PARENT "${TK_INCLUDE_PATH}" PATH)

GET_FILENAME_COMPONENT(TCL_LIBRARY_PATH "${TCL_LIBRARY}" PATH)
GET_FILENAME_COMPONENT(TCL_LIBRARY_PATH_PARENT "${TCL_LIBRARY_PATH}" PATH)
STRING(REGEX REPLACE 
  "^.*tcl([0-9]\\.*[0-9]).*$" "\\1" TCL_LIBRARY_VERSION "${TCL_LIBRARY}")

GET_FILENAME_COMPONENT(TK_LIBRARY_PATH "${TK_LIBRARY}" PATH)
GET_FILENAME_COMPONENT(TK_LIBRARY_PATH_PARENT "${TK_LIBRARY_PATH}" PATH)
STRING(REGEX REPLACE 
  "^.*tk([0-9]\\.*[0-9]).*$" "\\1" TK_LIBRARY_VERSION "${TK_LIBRARY}")

SET(TCLTK_POSSIBLE_LIB_PATHS
  "${TCL_INCLUDE_PATH_PARENT}/lib"
  "${TK_INCLUDE_PATH_PARENT}/lib"
  "${TCL_LIBRARY_PATH}"
  "${TK_LIBRARY_PATH}"
  "${TCL_TCLSH_PATH_PARENT}/lib"
  "${TK_WISH_PATH_PARENT}/lib"
  /usr/lib 
  /usr/local/lib
  )

IF(WIN32)
  GET_FILENAME_COMPONENT(
    ActiveTcl_CurrentVersion 
    "[HKEY_LOCAL_MACHINE\\SOFTWARE\\ActiveState\\ActiveTcl;CurrentVersion]" 
    NAME)
  SET(TCLTK_POSSIBLE_LIB_PATHS ${TCLTK_POSSIBLE_LIB_PATHS}
    "[HKEY_LOCAL_MACHINE\\SOFTWARE\\ActiveState\\ActiveTcl\\${ActiveTcl_CurrentVersion}]/lib"
    "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Scriptics\\Tcl\\8.6;Root]/lib"
    "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Scriptics\\Tcl\\8.5;Root]/lib"
    "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Scriptics\\Tcl\\8.4;Root]/lib"
    "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Scriptics\\Tcl\\8.3;Root]/lib"
    "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Scriptics\\Tcl\\8.2;Root]/lib"
    "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Scriptics\\Tcl\\8.0;Root]/lib"
    "$ENV{ProgramFiles}/Tcl/Lib"
    "C:/Program Files/Tcl/lib" 
    "C:/Tcl/lib" 
    )
ENDIF(WIN32)

FIND_LIBRARY(TCL_LIBRARY
  NAMES 
  tcl   
  tcl${TK_LIBRARY_VERSION} tcl${TCL_TCLSH_VERSION} tcl${TK_WISH_VERSION}
  tcl86 tcl8.6 
  tcl85 tcl8.5 
  tcl84 tcl8.4 
  tcl83 tcl8.3 
  tcl82 tcl8.2 
  tcl80 tcl8.0
  PATHS ${TCLTK_POSSIBLE_LIB_PATHS}
  )

FIND_LIBRARY(TK_LIBRARY 
  NAMES 
  tk
  tk${TCL_LIBRARY_VERSION} tk${TCL_TCLSH_VERSION} tk${TK_WISH_VERSION}
  tk86 tk8.6
  tk85 tk8.5 
  tk84 tk8.4 
  tk83 tk8.3 
  tk82 tk8.2 
  tk80 tk8.0
  PATHS ${TCLTK_POSSIBLE_LIB_PATHS}
  )

CMAKE_FIND_FRAMEWORKS(Tcl)
CMAKE_FIND_FRAMEWORKS(Tk)

SET(TCL_FRAMEWORK_INCLUDES)
IF(Tcl_FRAMEWORKS)
  IF(NOT TCL_INCLUDE_PATH)
    FOREACH(dir ${Tcl_FRAMEWORKS})
      SET(TCL_FRAMEWORK_INCLUDES ${TCL_FRAMEWORK_INCLUDES} ${dir}/Headers)
    ENDFOREACH(dir)
  ENDIF(NOT TCL_INCLUDE_PATH)
ENDIF(Tcl_FRAMEWORKS)

SET(TK_FRAMEWORK_INCLUDES)
IF(Tk_FRAMEWORKS)
  IF(NOT TK_INCLUDE_PATH)
    FOREACH(dir ${Tk_FRAMEWORKS})
      SET(TK_FRAMEWORK_INCLUDES ${TK_FRAMEWORK_INCLUDES}
        ${dir}/Headers ${dir}/PrivateHeaders)
    ENDFOREACH(dir)
  ENDIF(NOT TK_INCLUDE_PATH)
ENDIF(Tk_FRAMEWORKS)

SET(TCLTK_POSSIBLE_INCLUDE_PATHS
  "${TCL_LIBRARY_PATH_PARENT}/include"
  "${TK_LIBRARY_PATH_PARENT}/include"
  "${TCL_INCLUDE_PATH}"
  "${TK_INCLUDE_PATH}"
  ${TCL_FRAMEWORK_INCLUDES} 
  ${TK_FRAMEWORK_INCLUDES} 
  "${TCL_TCLSH_PATH_PARENT}/include"
  "${TK_WISH_PATH_PARENT}/include"
  /usr/include
  /usr/local/include
  /usr/include/tcl${TK_LIBRARY_VERSION}
  /usr/include/tcl${TCL_LIBRARY_VERSION}
  /usr/include/tcl8.6
  /usr/include/tcl8.5
  /usr/include/tcl8.4
  /usr/include/tcl8.3
  /usr/include/tcl8.2
  /usr/include/tcl8.0
  )

IF(WIN32)
  SET(TCLTK_POSSIBLE_INCLUDE_PATHS ${TCLTK_POSSIBLE_INCLUDE_PATHS}
    "[HKEY_LOCAL_MACHINE\\SOFTWARE\\ActiveState\\ActiveTcl\\${ActiveTcl_CurrentVersion}]/include"
    "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Scriptics\\Tcl\\8.6;Root]/include"
    "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Scriptics\\Tcl\\8.5;Root]/include"
    "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Scriptics\\Tcl\\8.4;Root]/include"
    "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Scriptics\\Tcl\\8.3;Root]/include"
    "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Scriptics\\Tcl\\8.2;Root]/include"
    "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Scriptics\\Tcl\\8.0;Root]/include"
    "$ENV{ProgramFiles}/Tcl/include"
    "C:/Program Files/Tcl/include"
    "C:/Tcl/include"
    )
ENDIF(WIN32)

FIND_PATH(TCL_INCLUDE_PATH 
  NAMES tcl.h
  HINTS ${TCLTK_POSSIBLE_INCLUDE_PATHS}
  )

FIND_PATH(TK_INCLUDE_PATH 
  NAMES tk.h
  HINTS ${TCLTK_POSSIBLE_INCLUDE_PATHS}
  )

# handle the QUIETLY and REQUIRED arguments and set TCL_FOUND to TRUE if 
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(TCL DEFAULT_MSG TCL_LIBRARY TCL_INCLUDE_PATH)
SET(TCLTK_FIND_REQUIRED ${TCL_FIND_REQUIRED})
SET(TCLTK_FIND_QUIETLY  ${TCL_FIND_QUIETLY})
FIND_PACKAGE_HANDLE_STANDARD_ARGS(TCLTK DEFAULT_MSG TCL_LIBRARY TCL_INCLUDE_PATH TK_LIBRARY TK_INCLUDE_PATH)
SET(TK_FIND_REQUIRED ${TCL_FIND_REQUIRED})
SET(TK_FIND_QUIETLY  ${TCL_FIND_QUIETLY})
FIND_PACKAGE_HANDLE_STANDARD_ARGS(TK DEFAULT_MSG TK_LIBRARY TK_INCLUDE_PATH)

MARK_AS_ADVANCED(
  TCL_INCLUDE_PATH
  TK_INCLUDE_PATH
  TCL_LIBRARY
  TK_LIBRARY
  )
