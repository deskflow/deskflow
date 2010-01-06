# - Find Tcl stub libraries.
# This module finds Tcl stub libraries. It first finds Tcl include files and
# libraries by calling FindTCL.cmake.
# How to Use the Tcl Stubs Library:
#   http://tcl.activestate.com/doc/howto/stubs.html
# Using Stub Libraries:
#   http://safari.oreilly.com/0130385603/ch48lev1sec3
# This code sets the following variables:
#  TCL_STUB_LIBRARY       = path to Tcl stub library
#  TK_STUB_LIBRARY        = path to Tk stub library
#  TTK_STUB_LIBRARY       = path to ttk stub library
#
# In an effort to remove some clutter and clear up some issues for people
# who are not necessarily Tcl/Tk gurus/developpers, some variables were
# moved or removed. Changes compared to CMake 2.4 are:
# - TCL_STUB_LIBRARY_DEBUG and TK_STUB_LIBRARY_DEBUG were removed.
#   => these libs are not packaged by default with Tcl/Tk distributions. 
#      Even when Tcl/Tk is built from source, several flavors of debug libs
#      are created and there is no real reason to pick a single one
#      specifically (say, amongst tclstub84g, tclstub84gs, or tclstub84sgx). 
#      Let's leave that choice to the user by allowing him to assign 
#      TCL_STUB_LIBRARY to any Tcl library, debug or not.

#=============================================================================
# Copyright 2008-2009 Kitware, Inc.
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

INCLUDE(FindTCL)

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

FIND_LIBRARY(TCL_STUB_LIBRARY
  NAMES 
  tclstub 
  tclstub${TK_LIBRARY_VERSION} tclstub${TCL_TCLSH_VERSION} tclstub${TK_WISH_VERSION}
  tclstub86 tclstub8.6
  tclstub85 tclstub8.5 
  tclstub84 tclstub8.4 
  tclstub83 tclstub8.3 
  tclstub82 tclstub8.2 
  tclstub80 tclstub8.0
  PATHS ${TCLTK_POSSIBLE_LIB_PATHS}
)

FIND_LIBRARY(TK_STUB_LIBRARY 
  NAMES 
  tkstub 
  tkstub${TCL_LIBRARY_VERSION} tkstub${TCL_TCLSH_VERSION} tkstub${TK_WISH_VERSION}
  tkstub86 tkstub8.6
  tkstub85 tkstub8.5 
  tkstub84 tkstub8.4 
  tkstub83 tkstub8.3 
  tkstub82 tkstub8.2 
  tkstub80 tkstub8.0
  PATHS ${TCLTK_POSSIBLE_LIB_PATHS}
)

FIND_LIBRARY(TTK_STUB_LIBRARY 
  NAMES 
  ttkstub 
  ttkstub${TCL_LIBRARY_VERSION} ttkstub${TCL_TCLSH_VERSION} ttkstub${TK_WISH_VERSION}
  ttkstub88 ttkstub8.8
  ttkstub87 ttkstub8.7
  ttkstub86 ttkstub8.6
  ttkstub85 ttkstub8.5 
  PATHS ${TCLTK_POSSIBLE_LIB_PATHS}
)

MARK_AS_ADVANCED(
  TCL_STUB_LIBRARY
  TK_STUB_LIBRARY
  )
