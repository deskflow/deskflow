# - Find wxWindows (wxWidgets) installation
# This module finds if wxWindows/wxWidgets is installed and determines where
# the include files and libraries are. It also determines what the name of
# the library is.
# Please note this file is DEPRECATED and replaced by FindwxWidgets.cmake.
# This code sets the following variables:
#
#  WXWINDOWS_FOUND     = system has WxWindows
#  WXWINDOWS_LIBRARIES = path to the wxWindows libraries
#                        on Unix/Linux with additional
#                        linker flags from
#                        "wx-config --libs"
#  CMAKE_WXWINDOWS_CXX_FLAGS  = Compiler flags for wxWindows,
#                               essentially "`wx-config --cxxflags`"
#                               on Linux
#  WXWINDOWS_INCLUDE_DIR      = where to find "wx/wx.h" and "wx/setup.h"
#  WXWINDOWS_LINK_DIRECTORIES = link directories, useful for rpath on
#                                Unix
#  WXWINDOWS_DEFINITIONS      = extra defines
#
# OPTIONS
# If you need OpenGL support please
#  SET(WXWINDOWS_USE_GL 1)
# in your CMakeLists.txt *before* you include this file.
#
#  HAVE_ISYSTEM      - true required to replace -I by -isystem on g++
#
# For convenience include Use_wxWindows.cmake in your project's
# CMakeLists.txt using INCLUDE(Use_wxWindows).
#
# USAGE
#  SET(WXWINDOWS_USE_GL 1)
#  FIND_PACKAGE(wxWindows)
#
# NOTES
# wxWidgets 2.6.x is supported for monolithic builds
# e.g. compiled  in wx/build/msw dir as:
#  nmake -f makefile.vc BUILD=debug SHARED=0 USE_OPENGL=1 MONOLITHIC=1
#
# DEPRECATED
#
#  CMAKE_WX_CAN_COMPILE
#  WXWINDOWS_LIBRARY
#  CMAKE_WX_CXX_FLAGS
#  WXWINDOWS_INCLUDE_PATH
#
# AUTHOR
# Jan Woetzel <http://www.mip.informatik.uni-kiel.de/~jw> (07/2003-01/2006)

#=============================================================================
# Copyright 2000-2009 Kitware, Inc.
# Copyright 2003-2006 Jan Woetzel
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

# ------------------------------------------------------------------
#
# -removed OPTION for CMAKE_WXWINDOWS_USE_GL. Force the developer to SET it before calling this.
# -major update for wx 2.6.2 and monolithic build option. (10/2005)
#
# STATUS
# tested with:
#  cmake 1.6.7, Linux (Suse 7.3), wxWindows 2.4.0, gcc 2.95
#  cmake 1.6.7, Linux (Suse 8.2), wxWindows 2.4.0, gcc 3.3
#  cmake 1.6.7, Linux (Suse 8.2), wxWindows 2.4.1-patch1,  gcc 3.3
#  cmake 1.6.7, MS Windows XP home, wxWindows 2.4.1, MS Visual Studio .net 7 2002 (static build)
#  cmake 2.0.5 on Windows XP and Suse Linux 9.2
#  cmake 2.0.6 on Windows XP and Suse Linux 9.2, wxWidgets 2.6.2 MONOLITHIC build
#  cmake 2.2.2 on Windows XP, MS Visual Studio .net 2003 7.1 wxWidgets 2.6.2 MONOLITHIC build
#
# TODO
#  -OPTION for unicode builds
#  -further testing of DLL linking under MS WIN32
#  -better support for non-monolithic builds
#


IF(WIN32)
  SET(WIN32_STYLE_FIND 1)
ENDIF(WIN32)
IF(MINGW)
  SET(WIN32_STYLE_FIND 0)
  SET(UNIX_STYLE_FIND 1)
ENDIF(MINGW)
IF(UNIX)
  SET(UNIX_STYLE_FIND 1)
ENDIF(UNIX)


IF(WIN32_STYLE_FIND)

  ## ######################################################################
  ##
  ## Windows specific:
  ##
  ## candidates for root/base directory of wxwindows
  ## should have subdirs include and lib containing include/wx/wx.h
  ## fix the root dir to avoid mixing of headers/libs from different
  ## versions/builds:

  SET (WXWINDOWS_POSSIBLE_ROOT_PATHS
    $ENV{WXWIN}
    "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\wxWidgets_is1;Inno Setup: App Path]"  ## WX 2.6.x
    "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\wxWindows_is1;Inno Setup: App Path]"  ## WX 2.4.x
    C:\\wxWidgets-2.6.2
    D:\\wxWidgets-2.6.2
    C:\\wxWidgets-2.6.1
    D:\\wxWidgets-2.6.1
    C:\\wxWindows-2.4.2
    D:\\wxWindows-2.4.2
    )

  ## WX supports monolithic and multiple smaller libs (since 2.5.x), we prefer monolithic for now.
  ## monolithic = WX is built as a single big library
  ## e.g. compile on WIN32 as  "nmake -f makefile.vc MONOLITHIC=1 BUILD=debug SHARED=0 USE_OPENGL=1" (JW)
  OPTION(WXWINDOWS_USE_MONOLITHIC "Use monolithic build of WX??" ON)
  MARK_AS_ADVANCED(WXWINDOWS_USE_MONOLITHIC)

  ## GL libs used?
  OPTION(WXWINDOWS_USE_GL "Use Wx with GL support(glcanvas)?" ON)
  MARK_AS_ADVANCED(WXWINDOWS_USE_GL)


  ## avoid mixing of headers and libs between multiple installed WX versions,
  ## select just one tree here:
  FIND_PATH(WXWINDOWS_ROOT_DIR  include/wx/wx.h
    ${WXWINDOWS_POSSIBLE_ROOT_PATHS} )
  # MESSAGE("DBG found WXWINDOWS_ROOT_DIR: ${WXWINDOWS_ROOT_DIR}")


  ## find libs for combination of static/shared with release/debug
  ## be careful if you add something here,
  ## avoid mixing of headers and libs of different wx versions,
  ## there may be multiple WX version s installed.
  SET (WXWINDOWS_POSSIBLE_LIB_PATHS
    "${WXWINDOWS_ROOT_DIR}/lib"
    )

  ## monolithic?
  IF (WXWINDOWS_USE_MONOLITHIC)

    FIND_LIBRARY(WXWINDOWS_STATIC_LIBRARY
      NAMES wx wxmsw wxmsw26
      PATHS
      "${WXWINDOWS_ROOT_DIR}/lib/vc_lib"
      ${WXWINDOWS_POSSIBLE_LIB_PATHS}
      DOC "wxWindows static release build library" )

    FIND_LIBRARY(WXWINDOWS_STATIC_DEBUG_LIBRARY
      NAMES wxd wxmswd wxmsw26d
      PATHS
      "${WXWINDOWS_ROOT_DIR}/lib/vc_lib"
      ${WXWINDOWS_POSSIBLE_LIB_PATHS}
      DOC "wxWindows static debug build library" )

    FIND_LIBRARY(WXWINDOWS_SHARED_LIBRARY
      NAMES wxmsw26 wxmsw262 wxmsw24 wxmsw242 wxmsw241 wxmsw240 wx23_2 wx22_9
      PATHS
      "${WXWINDOWS_ROOT_DIR}/lib/vc_dll"
      ${WXWINDOWS_POSSIBLE_LIB_PATHS}
      DOC "wxWindows shared release build library" )

    FIND_LIBRARY(WXWINDOWS_SHARED_DEBUG_LIBRARY
      NAMES wxmsw26d wxmsw262d wxmsw24d wxmsw241d wxmsw240d wx23_2d wx22_9d
      PATHS
      "${WXWINDOWS_ROOT_DIR}/lib/vc_dll"
      ${WXWINDOWS_POSSIBLE_LIB_PATHS}
      DOC "wxWindows shared debug build library " )


    ##
    ## required for WXWINDOWS_USE_GL
    ## gl lib is always build separate:
    ##
    FIND_LIBRARY(WXWINDOWS_STATIC_LIBRARY_GL
      NAMES wx_gl wxmsw_gl wxmsw26_gl
      PATHS
      "${WXWINDOWS_ROOT_DIR}/lib/vc_lib"
      ${WXWINDOWS_POSSIBLE_LIB_PATHS}
      DOC "wxWindows static release build GL library" )

    FIND_LIBRARY(WXWINDOWS_STATIC_DEBUG_LIBRARY_GL
      NAMES wxd_gl wxmswd_gl wxmsw26d_gl
      PATHS
      "${WXWINDOWS_ROOT_DIR}/lib/vc_lib"
      ${WXWINDOWS_POSSIBLE_LIB_PATHS}
      DOC "wxWindows static debug build GL library" )


    FIND_LIBRARY(WXWINDOWS_STATIC_DEBUG_LIBRARY_PNG
      NAMES wxpngd
      PATHS
      "${WXWINDOWS_ROOT_DIR}/lib/vc_lib"
      ${WXWINDOWS_POSSIBLE_LIB_PATHS}
      DOC "wxWindows static debug png library" )

    FIND_LIBRARY(WXWINDOWS_STATIC_LIBRARY_PNG
      NAMES wxpng
      PATHS
      "${WXWINDOWS_ROOT_DIR}/lib/vc_lib"
      ${WXWINDOWS_POSSIBLE_LIB_PATHS}
      DOC "wxWindows static png library" )

    FIND_LIBRARY(WXWINDOWS_STATIC_DEBUG_LIBRARY_TIFF
      NAMES wxtiffd
      PATHS
      "${WXWINDOWS_ROOT_DIR}/lib/vc_lib"
      ${WXWINDOWS_POSSIBLE_LIB_PATHS}
      DOC "wxWindows static debug tiff library" )

    FIND_LIBRARY(WXWINDOWS_STATIC_LIBRARY_TIFF
      NAMES wxtiff
      PATHS
      "${WXWINDOWS_ROOT_DIR}/lib/vc_lib"
      ${WXWINDOWS_POSSIBLE_LIB_PATHS}
      DOC "wxWindows static tiff library" )

    FIND_LIBRARY(WXWINDOWS_STATIC_DEBUG_LIBRARY_JPEG
      NAMES wxjpegd  wxjpgd
      PATHS
      "${WXWINDOWS_ROOT_DIR}/lib/vc_lib"
      ${WXWINDOWS_POSSIBLE_LIB_PATHS}
      DOC "wxWindows static debug jpeg library" )

    FIND_LIBRARY(WXWINDOWS_STATIC_LIBRARY_JPEG
      NAMES wxjpeg wxjpg
      PATHS
      "${WXWINDOWS_ROOT_DIR}/lib/vc_lib"
      ${WXWINDOWS_POSSIBLE_LIB_PATHS}
      DOC "wxWindows static jpeg library" )

    FIND_LIBRARY(WXWINDOWS_STATIC_DEBUG_LIBRARY_ZLIB
      NAMES wxzlibd
      PATHS
      "${WXWINDOWS_ROOT_DIR}/lib/vc_lib"
      ${WXWINDOWS_POSSIBLE_LIB_PATHS}
      DOC "wxWindows static debug zlib library" )

    FIND_LIBRARY(WXWINDOWS_STATIC_LIBRARY_ZLIB
      NAMES wxzlib
      PATHS
      "${WXWINDOWS_ROOT_DIR}/lib/vc_lib"
      ${WXWINDOWS_POSSIBLE_LIB_PATHS}
      DOC "wxWindows static zib library" )

    FIND_LIBRARY(WXWINDOWS_STATIC_DEBUG_LIBRARY_REGEX
      NAMES wxregexd
      PATHS
      "${WXWINDOWS_ROOT_DIR}/lib/vc_lib"
      ${WXWINDOWS_POSSIBLE_LIB_PATHS}
      DOC "wxWindows static debug regex library" )

    FIND_LIBRARY(WXWINDOWS_STATIC_LIBRARY_REGEX
      NAMES wxregex
      PATHS
      "${WXWINDOWS_ROOT_DIR}/lib/vc_lib"
      ${WXWINDOWS_POSSIBLE_LIB_PATHS}
      DOC "wxWindows static regex library" )



    ## untested:
    FIND_LIBRARY(WXWINDOWS_SHARED_LIBRARY_GL
      NAMES wx_gl wxmsw_gl wxmsw26_gl
      PATHS
      "${WXWINDOWS_ROOT_DIR}/lib/vc_dll"
      ${WXWINDOWS_POSSIBLE_LIB_PATHS}
      DOC "wxWindows shared release build GL library" )

    FIND_LIBRARY(WXWINDOWS_SHARED_DEBUG_LIBRARY_GL
      NAMES wxd_gl wxmswd_gl wxmsw26d_gl
      PATHS
      "${WXWINDOWS_ROOT_DIR}/lib/vc_dll"
      ${WXWINDOWS_POSSIBLE_LIB_PATHS}
      DOC "wxWindows shared debug build GL library" )


  ELSE (WXWINDOWS_USE_MONOLITHIC)
    ## WX is built as multiple small pieces libraries instead of monolithic

    ## DEPECATED (jw) replaced by more general WXWINDOWS_USE_MONOLITHIC ON/OFF
    # OPTION(WXWINDOWS_SEPARATE_LIBS_BUILD "Is wxWindows build with separate libs?" OFF)

    ## HACK: This is very dirty.
    ## because the libs of a particular version are explicitly listed
    ## and NOT searched/verified.
    ## TODO:  Really search for each lib, then decide for
    ## monolithic x debug x shared x GL (=16 combinations) for at least 18 libs
    ## -->  about 288 combinations
    ## thus we need a different approach so solve this correctly ...

    MESSAGE(STATUS "Warning: You are trying to use wxWidgets without monolithic build (WXWINDOWS_SEPARATE_LIBS_BUILD). This is a HACK, libraries are not verified! (JW).")

    SET(WXWINDOWS_STATIC_LIBS ${WXWINDOWS_STATIC_LIBS}
      wxbase26
      wxbase26_net
      wxbase26_odbc
      wxbase26_xml
      wxmsw26_adv
      wxmsw26_core
      wxmsw26_dbgrid
      wxmsw26_gl
      wxmsw26_html
      wxmsw26_media
      wxmsw26_qa
      wxmsw26_xrc
      wxexpat
      wxjpeg
      wxpng
      wxregex
      wxtiff
      wxzlib
      comctl32
      rpcrt4
      wsock32
      )
    ## HACK: feed in to optimized / debug libaries if both were FOUND.
    SET(WXWINDOWS_STATIC_DEBUG_LIBS ${WXWINDOWS_STATIC_DEBUG_LIBS}
      wxbase26d
      wxbase26d_net
      wxbase26d_odbc
      wxbase26d_xml
      wxmsw26d_adv
      wxmsw26d_core
      wxmsw26d_dbgrid
      wxmsw26d_gl
      wxmsw26d_html
      wxmsw26d_media
      wxmsw26d_qa
      wxmsw26d_xrc
      wxexpatd
      wxjpegd
      wxpngd
      wxregexd
      wxtiffd
      wxzlibd
      comctl32
      rpcrt4
      wsock32
      )
  ENDIF (WXWINDOWS_USE_MONOLITHIC)


  ##
  ## now we should have found all WX libs available on the system.
  ## let the user decide which of the available onse to use.
  ##

  ## if there is at least one shared lib available
  ## let user choose wether to use shared or static wxwindows libs
  IF(WXWINDOWS_SHARED_LIBRARY OR WXWINDOWS_SHARED_DEBUG_LIBRARY)
    ## default value OFF because wxWindows MSVS default build is static
    OPTION(WXWINDOWS_USE_SHARED_LIBS
      "Use shared versions (dll) of wxWindows libraries?" OFF)
    MARK_AS_ADVANCED(WXWINDOWS_USE_SHARED_LIBS)
  ENDIF(WXWINDOWS_SHARED_LIBRARY OR WXWINDOWS_SHARED_DEBUG_LIBRARY)

  ## add system libraries wxwindows always seems to depend on
  SET(WXWINDOWS_LIBRARIES ${WXWINDOWS_LIBRARIES}
    comctl32
    rpcrt4
    wsock32
    )

  IF (NOT WXWINDOWS_USE_SHARED_LIBS)
    SET(WXWINDOWS_LIBRARIES ${WXWINDOWS_LIBRARIES}
      ##  these ones dont seem required, in particular  ctl3d32 is not neccesary (Jan Woetzel 07/2003)
      #   ctl3d32
      debug ${WXWINDOWS_STATIC_DEBUG_LIBRARY_ZLIB}   optimized ${WXWINDOWS_STATIC_LIBRARY_ZLIB}
      debug ${WXWINDOWS_STATIC_DEBUG_LIBRARY_REGEX}  optimized ${WXWINDOWS_STATIC_LIBRARY_REGEX}
      debug ${WXWINDOWS_STATIC_DEBUG_LIBRARY_PNG}    optimized ${WXWINDOWS_STATIC_LIBRARY_PNG}
      debug ${WXWINDOWS_STATIC_DEBUG_LIBRARY_JPEG}   optimized ${WXWINDOWS_STATIC_LIBRARY_JPEG}
      debug ${WXWINDOWS_STATIC_DEBUG_LIBRARY_TIFF}   optimized ${WXWINDOWS_STATIC_LIBRARY_TIFF}
      )
  ENDIF (NOT WXWINDOWS_USE_SHARED_LIBS)

  ## opengl/glu: TODO/FIXME: better use FindOpenGL.cmake here
  ## assume release versions of glu an dopengl, here.
  IF (WXWINDOWS_USE_GL)
    SET(WXWINDOWS_LIBRARIES ${WXWINDOWS_LIBRARIES}
      opengl32
      glu32 )
  ENDIF (WXWINDOWS_USE_GL)

  ##
  ## select between use of  shared or static wxWindows lib then set libs to use
  ## for debug and optimized build.  so the user can switch between debug and
  ## release build e.g. within MS Visual Studio without running cmake with a
  ## different build directory again.
  ##
  ## then add the build specific include dir for wx/setup.h
  ##

  IF(WXWINDOWS_USE_SHARED_LIBS)
    ##MESSAGE("DBG wxWindows use shared lib selected.")
    ## assume that both builds use the same setup(.h) for simplicity

    ## shared: both wx (debug and release) found?
    ## assume that both builds use the same setup(.h) for simplicity
    IF(WXWINDOWS_SHARED_DEBUG_LIBRARY AND WXWINDOWS_SHARED_LIBRARY)
      ##MESSAGE("DBG wx shared: debug and optimized found.")
      FIND_PATH(WXWINDOWS_INCLUDE_DIR_SETUPH  wx/setup.h
        ${WXWINDOWS_ROOT_DIR}/lib/mswdlld
        ${WXWINDOWS_ROOT_DIR}/lib/mswdll
        ${WXWINDOWS_ROOT_DIR}/lib/vc_dll/mswd
        ${WXWINDOWS_ROOT_DIR}/lib/vc_dll/msw )
      SET(WXWINDOWS_LIBRARIES ${WXWINDOWS_LIBRARIES}
        debug     ${WXWINDOWS_SHARED_DEBUG_LIBRARY}
        optimized ${WXWINDOWS_SHARED_LIBRARY} )
      IF (WXWINDOWS_USE_GL)
        SET(WXWINDOWS_LIBRARIES ${WXWINDOWS_LIBRARIES}
          debug     ${WXWINDOWS_SHARED_DEBUG_LIBRARY_GL}
          optimized ${WXWINDOWS_SHARED_LIBRARY_GL} )
      ENDIF (WXWINDOWS_USE_GL)
    ENDIF(WXWINDOWS_SHARED_DEBUG_LIBRARY AND WXWINDOWS_SHARED_LIBRARY)

    ## shared: only debug wx lib found?
    IF(WXWINDOWS_SHARED_DEBUG_LIBRARY)
      IF(NOT WXWINDOWS_SHARED_LIBRARY)
        ##MESSAGE("DBG wx shared: debug (but no optimized) found.")
        FIND_PATH(WXWINDOWS_INCLUDE_DIR_SETUPH  wx/setup.h
          ${WXWINDOWS_ROOT_DIR}/lib/mswdlld
          ${WXWINDOWS_ROOT_DIR}/lib/vc_dll/mswd  )
        SET(WXWINDOWS_LIBRARIES ${WXWINDOWS_LIBRARIES}
          ${WXWINDOWS_SHARED_DEBUG_LIBRARY} )
        IF (WXWINDOWS_USE_GL)
          SET(WXWINDOWS_LIBRARIES ${WXWINDOWS_LIBRARIES}
            ${WXWINDOWS_SHARED_DEBUG_LIBRARY_GL} )
        ENDIF (WXWINDOWS_USE_GL)
      ENDIF(NOT WXWINDOWS_SHARED_LIBRARY)
    ENDIF(WXWINDOWS_SHARED_DEBUG_LIBRARY)

    ## shared: only release wx lib found?
    IF(NOT WXWINDOWS_SHARED_DEBUG_LIBRARY)
      IF(WXWINDOWS_SHARED_LIBRARY)
        ##MESSAGE("DBG wx shared: optimized (but no debug) found.")
        FIND_PATH(WXWINDOWS_INCLUDE_DIR_SETUPH  wx/setup.h
          ${WXWINDOWS_ROOT_DIR}/lib/mswdll
          ${WXWINDOWS_ROOT_DIR}/lib/vc_dll/msw  )
        SET(WXWINDOWS_LIBRARIES ${WXWINDOWS_LIBRARIES}
          ${WXWINDOWS_SHARED_DEBUG_LIBRARY} )
        IF (WXWINDOWS_USE_GL)
          SET(WXWINDOWS_LIBRARIES ${WXWINDOWS_LIBRARIES}
            ${WXWINDOWS_SHARED_DEBUG_LIBRARY_GL} )
        ENDIF (WXWINDOWS_USE_GL)
      ENDIF(WXWINDOWS_SHARED_LIBRARY)
    ENDIF(NOT WXWINDOWS_SHARED_DEBUG_LIBRARY)

    ## shared: none found?
    IF(NOT WXWINDOWS_SHARED_DEBUG_LIBRARY)
      IF(NOT WXWINDOWS_SHARED_LIBRARY)
        MESSAGE(STATUS
          "No shared wxWindows lib found, but WXWINDOWS_USE_SHARED_LIBS=${WXWINDOWS_USE_SHARED_LIBS}.")
      ENDIF(NOT WXWINDOWS_SHARED_LIBRARY)
    ENDIF(NOT WXWINDOWS_SHARED_DEBUG_LIBRARY)

    #########################################################################################
  ELSE(WXWINDOWS_USE_SHARED_LIBS)

    ##jw: DEPRECATED IF(NOT WXWINDOWS_SEPARATE_LIBS_BUILD)

    ## static: both wx (debug and release) found?
    ## assume that both builds use the same setup(.h) for simplicity
    IF(WXWINDOWS_STATIC_DEBUG_LIBRARY AND WXWINDOWS_STATIC_LIBRARY)
      ##MESSAGE("DBG wx static: debug and optimized found.")
      FIND_PATH(WXWINDOWS_INCLUDE_DIR_SETUPH  wx/setup.h
        ${WXWINDOWS_ROOT_DIR}/lib/mswd
        ${WXWINDOWS_ROOT_DIR}/lib/msw
        ${WXWINDOWS_ROOT_DIR}/lib/vc_lib/mswd
        ${WXWINDOWS_ROOT_DIR}/lib/vc_lib/msw )
      SET(WXWINDOWS_LIBRARIES ${WXWINDOWS_LIBRARIES}
        debug     ${WXWINDOWS_STATIC_DEBUG_LIBRARY}
        optimized ${WXWINDOWS_STATIC_LIBRARY} )
      IF (WXWINDOWS_USE_GL)
        SET(WXWINDOWS_LIBRARIES ${WXWINDOWS_LIBRARIES}
          debug     ${WXWINDOWS_STATIC_DEBUG_LIBRARY_GL}
          optimized ${WXWINDOWS_STATIC_LIBRARY_GL} )
      ENDIF (WXWINDOWS_USE_GL)
    ENDIF(WXWINDOWS_STATIC_DEBUG_LIBRARY AND WXWINDOWS_STATIC_LIBRARY)

    ## static: only debug wx lib found?
    IF(WXWINDOWS_STATIC_DEBUG_LIBRARY)
      IF(NOT WXWINDOWS_STATIC_LIBRARY)
        ##MESSAGE("DBG wx static: debug (but no optimized) found.")
        FIND_PATH(WXWINDOWS_INCLUDE_DIR_SETUPH  wx/setup.h
          ${WXWINDOWS_ROOT_DIR}/lib/mswd
          ${WXWINDOWS_ROOT_DIR}/lib/vc_lib/mswd  )
        SET(WXWINDOWS_LIBRARIES ${WXWINDOWS_LIBRARIES}
          ${WXWINDOWS_STATIC_DEBUG_LIBRARY} )
        IF (WXWINDOWS_USE_GL)
          SET(WXWINDOWS_LIBRARIES ${WXWINDOWS_LIBRARIES}
            ${WXWINDOWS_STATIC_DEBUG_LIBRARY_GL} )
        ENDIF (WXWINDOWS_USE_GL)
      ENDIF(NOT WXWINDOWS_STATIC_LIBRARY)
    ENDIF(WXWINDOWS_STATIC_DEBUG_LIBRARY)

    ## static: only release wx lib found?
    IF(NOT WXWINDOWS_STATIC_DEBUG_LIBRARY)
      IF(WXWINDOWS_STATIC_LIBRARY)
        ##MESSAGE("DBG wx static: optimized (but no debug) found.")
        FIND_PATH(WXWINDOWS_INCLUDE_DIR_SETUPH  wx/setup.h
          ${WXWINDOWS_ROOT_DIR}/lib/msw
          ${WXWINDOWS_ROOT_DIR}/lib/vc_lib/msw )
        SET(WXWINDOWS_LIBRARIES ${WXWINDOWS_LIBRARIES}
          ${WXWINDOWS_STATIC_LIBRARY} )
        IF (WXWINDOWS_USE_GL)
          SET(WXWINDOWS_LIBRARIES ${WXWINDOWS_LIBRARIES}
            ${WXWINDOWS_STATIC_LIBRARY_GL} )
        ENDIF (WXWINDOWS_USE_GL)
      ENDIF(WXWINDOWS_STATIC_LIBRARY)
    ENDIF(NOT WXWINDOWS_STATIC_DEBUG_LIBRARY)

    ## static: none found?
    IF(NOT WXWINDOWS_STATIC_DEBUG_LIBRARY AND NOT WXWINDOWS_SEPARATE_LIBS_BUILD)
      IF(NOT WXWINDOWS_STATIC_LIBRARY)
        MESSAGE(STATUS
          "No static wxWindows lib found, but WXWINDOWS_USE_SHARED_LIBS=${WXWINDOWS_USE_SHARED_LIBS}.")
      ENDIF(NOT WXWINDOWS_STATIC_LIBRARY)
    ENDIF(NOT WXWINDOWS_STATIC_DEBUG_LIBRARY AND NOT WXWINDOWS_SEPARATE_LIBS_BUILD)
  ENDIF(WXWINDOWS_USE_SHARED_LIBS)


  ## not neccessary in wxWindows 2.4.1 and 2.6.2
  ## but it may fix a previous bug, see
  ## http://lists.wxwindows.org/cgi-bin/ezmlm-cgi?8:mss:37574:200305:mpdioeneabobmgjenoap
  OPTION(WXWINDOWS_SET_DEFINITIONS "Set additional defines for wxWindows" OFF)
  MARK_AS_ADVANCED(WXWINDOWS_SET_DEFINITIONS)
  IF (WXWINDOWS_SET_DEFINITIONS)
    SET(WXWINDOWS_DEFINITIONS "-DWINVER=0x400")
  ELSE (WXWINDOWS_SET_DEFINITIONS)
    # clear:
    SET(WXWINDOWS_DEFINITIONS "")
  ENDIF (WXWINDOWS_SET_DEFINITIONS)



  ## Find the include directories for wxwindows
  ## the first, build specific for wx/setup.h was determined before.
  ## add inc dir for general for "wx/wx.h"
  FIND_PATH(WXWINDOWS_INCLUDE_DIR  wx/wx.h
    "${WXWINDOWS_ROOT_DIR}/include" )
  ## append the build specific include dir for wx/setup.h:
  IF (WXWINDOWS_INCLUDE_DIR_SETUPH)
    SET(WXWINDOWS_INCLUDE_DIR ${WXWINDOWS_INCLUDE_DIR} ${WXWINDOWS_INCLUDE_DIR_SETUPH} )
  ENDIF (WXWINDOWS_INCLUDE_DIR_SETUPH)



  MARK_AS_ADVANCED(
    WXWINDOWS_ROOT_DIR
    WXWINDOWS_INCLUDE_DIR
    WXWINDOWS_INCLUDE_DIR_SETUPH
    WXWINDOWS_STATIC_LIBRARY
    WXWINDOWS_STATIC_LIBRARY_GL
    WXWINDOWS_STATIC_DEBUG_LIBRARY
    WXWINDOWS_STATIC_DEBUG_LIBRARY_GL
    WXWINDOWS_STATIC_LIBRARY_ZLIB
    WXWINDOWS_STATIC_DEBUG_LIBRARY_ZLIB
    WXWINDOWS_STATIC_LIBRARY_REGEX
    WXWINDOWS_STATIC_DEBUG_LIBRARY_REGEX
    WXWINDOWS_STATIC_LIBRARY_PNG
    WXWINDOWS_STATIC_DEBUG_LIBRARY_PNG
    WXWINDOWS_STATIC_LIBRARY_JPEG
    WXWINDOWS_STATIC_DEBUG_LIBRARY_JPEG
    WXWINDOWS_STATIC_DEBUG_LIBRARY_TIFF
    WXWINDOWS_STATIC_LIBRARY_TIFF
    WXWINDOWS_SHARED_LIBRARY
    WXWINDOWS_SHARED_DEBUG_LIBRARY
    WXWINDOWS_SHARED_LIBRARY_GL
    WXWINDOWS_SHARED_DEBUG_LIBRARY_GL
    )


ELSE(WIN32_STYLE_FIND)

  IF (UNIX_STYLE_FIND)
    ## ######################################################################
    ##
    ## UNIX/Linux specific:
    ##
    ## use backquoted wx-config to query and set flags and libs:
    ## 06/2003 Jan Woetzel
    ##

    OPTION(WXWINDOWS_USE_SHARED_LIBS "Use shared versions (.so) of wxWindows libraries" ON)
    MARK_AS_ADVANCED(WXWINDOWS_USE_SHARED_LIBS)

    # JW removed option and force the develper th SET it.
    # OPTION(WXWINDOWS_USE_GL "use wxWindows with GL support (use additional
    # --gl-libs for wx-config)?" OFF)

    # wx-config should be in your path anyhow, usually no need to set WXWIN or
    # search in ../wx or ../../wx
    FIND_PROGRAM(CMAKE_WXWINDOWS_WXCONFIG_EXECUTABLE wx-config
      $ENV{WXWIN}
      $ENV{WXWIN}/bin
      ../wx/bin
      ../../wx/bin )

    # check wether wx-config was found:
    IF(CMAKE_WXWINDOWS_WXCONFIG_EXECUTABLE)

      # use shared/static wx lib?
      # remember: always link shared to use systems GL etc. libs (no static
      # linking, just link *against* static .a libs)
      IF(WXWINDOWS_USE_SHARED_LIBS)
        SET(WX_CONFIG_ARGS_LIBS "--libs")
      ELSE(WXWINDOWS_USE_SHARED_LIBS)
        SET(WX_CONFIG_ARGS_LIBS "--static --libs")
      ENDIF(WXWINDOWS_USE_SHARED_LIBS)

      # do we need additionial wx GL stuff like GLCanvas ?
      IF(WXWINDOWS_USE_GL)
        SET(WX_CONFIG_ARGS_LIBS "${WX_CONFIG_ARGS_LIBS} --gl-libs" )
      ENDIF(WXWINDOWS_USE_GL)
      ##MESSAGE("DBG: WX_CONFIG_ARGS_LIBS=${WX_CONFIG_ARGS_LIBS}===")

      # set CXXFLAGS to be fed into CMAKE_CXX_FLAGS by the user:
      IF (HAVE_ISYSTEM) # does the compiler support -isystem ?
              IF (NOT APPLE) # -isystem seem sto be unsuppored on Mac
                IF(CMAKE_COMPILER_IS_GNUCC AND CMAKE_COMPILER_IS_GNUCXX )
            IF (CMAKE_CXX_COMPILER MATCHES g\\+\\+)
              SET(CMAKE_WXWINDOWS_CXX_FLAGS "`${CMAKE_WXWINDOWS_WXCONFIG_EXECUTABLE} --cxxflags|sed -e s/-I/-isystem/g`")
            ELSE(CMAKE_CXX_COMPILER MATCHES g\\+\\+)
              SET(CMAKE_WXWINDOWS_CXX_FLAGS "`${CMAKE_WXWINDOWS_WXCONFIG_EXECUTABLE} --cxxflags`")
            ENDIF(CMAKE_CXX_COMPILER MATCHES g\\+\\+)
                ENDIF(CMAKE_COMPILER_IS_GNUCC AND CMAKE_COMPILER_IS_GNUCXX )
              ENDIF (NOT APPLE)
      ENDIF (HAVE_ISYSTEM)
      ##MESSAGE("DBG: for compilation:
      ##CMAKE_WXWINDOWS_CXX_FLAGS=${CMAKE_WXWINDOWS_CXX_FLAGS}===")

      # keep the back-quoted string for clarity
      SET(WXWINDOWS_LIBRARIES "`${CMAKE_WXWINDOWS_WXCONFIG_EXECUTABLE} ${WX_CONFIG_ARGS_LIBS}`")
      ##MESSAGE("DBG2: for linking:
      ##WXWINDOWS_LIBRARIES=${WXWINDOWS_LIBRARIES}===")

      # evaluate wx-config output to separate linker flags and linkdirs for
      # rpath:
      EXEC_PROGRAM(${CMAKE_WXWINDOWS_WXCONFIG_EXECUTABLE}
        ARGS ${WX_CONFIG_ARGS_LIBS}
        OUTPUT_VARIABLE WX_CONFIG_LIBS )

      ## extract linkdirs (-L) for rpath
      ## use regular expression to match wildcard equivalent "-L*<endchar>"
      ## with <endchar> is a space or a semicolon
      STRING(REGEX MATCHALL "[-][L]([^ ;])+" WXWINDOWS_LINK_DIRECTORIES_WITH_PREFIX "${WX_CONFIG_LIBS}" )
      # MESSAGE("DBG  WXWINDOWS_LINK_DIRECTORIES_WITH_PREFIX=${WXWINDOWS_LINK_DIRECTORIES_WITH_PREFIX}")

      ## remove prefix -L because we need the pure directory for LINK_DIRECTORIES
      ## replace -L by ; because the separator seems to be lost otherwise (bug or
      ## feature?)
      IF(WXWINDOWS_LINK_DIRECTORIES_WITH_PREFIX)
        STRING(REGEX REPLACE "[-][L]" ";" WXWINDOWS_LINK_DIRECTORIES ${WXWINDOWS_LINK_DIRECTORIES_WITH_PREFIX} )
        # MESSAGE("DBG  WXWINDOWS_LINK_DIRECTORIES=${WXWINDOWS_LINK_DIRECTORIES}")
      ENDIF(WXWINDOWS_LINK_DIRECTORIES_WITH_PREFIX)


      ## replace space separated string by semicolon separated vector to make it
      ## work with LINK_DIRECTORIES
      SEPARATE_ARGUMENTS(WXWINDOWS_LINK_DIRECTORIES)

      MARK_AS_ADVANCED(
        CMAKE_WXWINDOWS_CXX_FLAGS
        WXWINDOWS_INCLUDE_DIR
        WXWINDOWS_LIBRARIES
        CMAKE_WXWINDOWS_WXCONFIG_EXECUTABLE
        )


      ## we really need wx-config...
    ELSE(CMAKE_WXWINDOWS_WXCONFIG_EXECUTABLE)
      MESSAGE(STATUS "Cannot find wx-config anywhere on the system. Please put the file into your path or specify it in CMAKE_WXWINDOWS_WXCONFIG_EXECUTABLE.")
      MARK_AS_ADVANCED(CMAKE_WXWINDOWS_WXCONFIG_EXECUTABLE)
    ENDIF(CMAKE_WXWINDOWS_WXCONFIG_EXECUTABLE)



  ELSE(UNIX_STYLE_FIND)
    MESSAGE(STATUS "FindwxWindows.cmake:  Platform unknown/unsupported by FindwxWindows.cmake. It's neither WIN32 nor UNIX")
  ENDIF(UNIX_STYLE_FIND)
ENDIF(WIN32_STYLE_FIND)


IF(WXWINDOWS_LIBRARIES)
  IF(WXWINDOWS_INCLUDE_DIR OR CMAKE_WXWINDOWS_CXX_FLAGS)
    ## found all we need.
    SET(WXWINDOWS_FOUND 1)

    ## set deprecated variables for backward compatibility:
    SET(CMAKE_WX_CAN_COMPILE   ${WXWINDOWS_FOUND})
    SET(WXWINDOWS_LIBRARY     ${WXWINDOWS_LIBRARIES})
    SET(WXWINDOWS_INCLUDE_PATH ${WXWINDOWS_INCLUDE_DIR})
    SET(WXWINDOWS_LINK_DIRECTORIES ${WXWINDOWS_LINK_DIRECTORIES})
    SET(CMAKE_WX_CXX_FLAGS     ${CMAKE_WXWINDOWS_CXX_FLAGS})

  ENDIF(WXWINDOWS_INCLUDE_DIR OR CMAKE_WXWINDOWS_CXX_FLAGS)
ENDIF(WXWINDOWS_LIBRARIES)
